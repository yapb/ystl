// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/string.h>

// unsupported-platform shim
#if defined(YSTL_ARCH_NON_X86) || (defined(YSTL_MACOS) && defined(YSTL_ARCH_X64))

namespace ystl {

template <typename T> class Detour final : public NonCopyable {
public:
  explicit Detour () = default;
  ~Detour () = default;

  Detour (StringRef module, StringRef name, T *address) {
    initialize (module, name, address);
  }

  void initialize (StringRef, StringRef, T *) {}
  void install (void *, bool = false) {}

  bool valid () const {
    return false;
  }
  bool detoured () const {
    return false;
  }
  bool detour () {
    return false;
  }
  bool restore () {
    return false;
  }

  template <typename... Args> decltype (auto) operator() (Args &&...args) {
    auto fn = reinterpret_cast<T *> (trampoline_ ? trampoline_ : original_);
    return fn (ystl::forward<Args> (args)...);
  }

private:
  void *original_ { nullptr };
  void *trampoline_ { nullptr };
};

}

#else
  #if !defined(YSTL_WINDOWS)
    #include <sys/mman.h>
    #include <pthread.h>

    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
      #define MAP_ANONYMOUS MAP_ANON
    #endif
  #endif

  #include <cstdint>
  #include <cstring>
  #include <ystl/thread.h>

namespace ystl {

// x86instr - x86 / x64 instruction-length decoder and relocation helpers
class X86Instr final {
  X86Instr () = delete;

public:
  // decode the length of the instruction at code, returns 0 when the stream cannot be decoded (caller should fall back to a safe minimum)
  static size_t length (const uint8_t *code) noexcept {
    size_t len = 0;

    // legacy prefixes (up to 4)
    for (int i = 0; i < 4; ++i) {
      auto b = code[len];

      if (b == 0x26 || b == 0x2E || b == 0x36 || b == 0x3E || b == 0x64 || b == 0x65 || b == 0x66 || b == 0x67 || b == 0xF0 || b == 0xF2 ||
          b == 0xF3) {
        ++len;
      }
      else {
        break;
      }
    }

  #if defined(YSTL_ARCH_X64)
    // rex prefix
    if ((code[len] & 0xF0) == 0x40) {
      ++len;
    }
  #endif

    auto b = code[len];
    auto has_mod_rm = false;

    size_t imm_size = 0;
    int group_imm = 0;
    uint8_t modrm = 0;

    if (b == 0x0F) {
      // two-byte / three-byte opcode
      uint8_t b2 = code[++len];

      if (b2 == 0x38 || b2 == 0x3A) {
        ++len; // three-byte opcode (sse 3+)
        has_mod_rm = true;
      }
      else {
        has_mod_rm = true;
        for (auto x : kNoModRM2) {
          if (b2 == x) {
            has_mod_rm = false;
            break;
          }
        }
      }
      ++len;

      if (b2 == 0xA4 || b2 == 0xAC || b2 == 0xAA || b2 == 0xAB) {
        imm_size = 1; // shld / shrd / bsswap / bswap
      }
    }
    else {
      // one-byte opcode
      for (auto x : kModRM1) {
        if (b == x) {
          has_mod_rm = true;
          break;
        }
      }

      // immediate sizes
      if ((b >= 0x04 && b <= 0x3C && (b & 0x07) == 0x04) || b == 0x6A || b == 0xEB || b == 0xE0 || b == 0xE1 || b == 0xE2 || b == 0xE3 ||
          (b >= 0xB0 && b <= 0xB7) || b == 0xCD || b == 0xA8) {
        imm_size = 1;
      }
      else if ((b >= 0x05 && b <= 0x3D && (b & 0x07) == 0x05) || b == 0x68 || b == 0xC7 || b == 0xE8 || b == 0xE9 || b == 0xA9) {
        imm_size = 4;
      }
      else if (b >= 0xB8 && b <= 0xBF) {

  #if defined(YSTL_ARCH_X64)
        // mov r,imm : imm32 unless preceded by rex.w
        if (len > 0 && (code[len - 1] & 0xF0) == 0x40) {
          imm_size = 8;
        }
        else {
          imm_size = 4;
        }
  #else
        imm_size = 4;
  #endif
      }
      else if (b == 0xC2) {
        imm_size = 2; // ret near imm16
      }
      else if (b == 0xC6) {
        imm_size = 1; // mov r/m8, imm8
      }

      // opcodes whose immediate presence/size depends on the modrm.reg field
      if (b == 0x80 || b == 0x83 || b == 0xC0 || b == 0xC1 || b == 0x6B || b == 0xF6) {
        group_imm = 1;
      }
      else if (b == 0x81 || b == 0x69 || b == 0xF7) {
        group_imm = 4;
      }

      ++len;
    }

    if (has_mod_rm) {
      modrm = code[len];
      uint8_t mod = (modrm >> 6) & 3;
      uint8_t rm = modrm & 7;

      ++len;

      bool has_sib = (rm == 4 && mod != 3);

      if (has_sib) {
        ++len; // sib byte
      }

      if (mod == 1) {
        len += 1;
      }
      else if (mod == 2) {
        len += 4;
      }
      else if (mod == 0) {
        if (rm == 5) {
          len += 4; // disp32 absolute / rip-relative
        }
        else if (has_sib) {
          uint8_t base = code[len - 1] & 7;

          if (base == 5) {
            len += 4;
          }
        }
      }
    }

    if (group_imm != 0) {
      imm_size = static_cast<size_t> (group_imm);

      // test/not/neg/mul/... only /0 and /1 carry an immediate
      if ((b == 0xF6 || b == 0xF7) && (((modrm >> 3) & 7) > 1)) {
        imm_size = 0;
      }
    }

    // immediate
    len += imm_size;
    return len;
  }

  // smallest offset >= minbytes that falls on an instruction boundary
  static size_t align_boundary (const uint8_t *code, size_t min_bytes) noexcept {
    size_t off = 0;

    while (off < min_bytes) {
      auto ilen = length (code + off);

      if (ilen == 0) {
        return min_bytes; // undecodable – use the safe minimum
      }
      off += ilen;

      if (off > 64) {
        break;
      }
    }
    return off;
  }

  // fix up relative displacements; returns false when some relocated displacement cannot be represented
  static bool fixup_relatives (uint8_t *tramp, const uint8_t *orig, size_t patch_len, intptr_t delta) noexcept {
    size_t off = 0;

    while (off < patch_len) {
      auto ilen = length (orig + off);
      if (ilen == 0 || ilen > patch_len - off) {
        break;
      }
      bool handled = false;

      auto adjust_disp = [] (const uint8_t *orig_ptr, uint8_t *tramp_ptr, size_t offset, size_t total_len, int64_t delta_val,
                           size_t disp_size) -> bool {
        if (offset + disp_size + 1 > total_len) {
          return false;
        }

        if (disp_size == 1) {
          auto disp = *reinterpret_cast<const int8_t *> (orig_ptr + offset + 1);
          if (delta_val != static_cast<int8_t> (delta_val)) {
            return false;
          }
          disp = static_cast<int8_t> (disp - static_cast<int8_t> (delta_val));
          memcpy (tramp_ptr + offset + 1, &disp, disp_size);
        }
        else {
          auto disp = *reinterpret_cast<const int32_t *> (orig_ptr + offset + 1);
          const auto new_disp = static_cast<int64_t> (disp) - delta_val;

          if (new_disp != static_cast<int32_t> (new_disp)) {
            return false; // the target is out of rel32 reach from the trampoline
          }
          memcpy (tramp_ptr + offset + 1, &new_disp, disp_size);
        }

        return true;
      };

      if (off + 1 <= patch_len) {
        uint8_t b = orig[off];

        if (b == 0xE8 || b == 0xE9) { // rel32
          if (!adjust_disp (orig, tramp, off, patch_len, delta, sizeof (int32_t))) {
            return false;
          }
          handled = true;
        }
        else if ((b >= 0x70 && b <= 0x7F) || b == 0xEB || (b >= 0xE0 && b <= 0xE3)) { // rel8
          if (!adjust_disp (orig, tramp, off, patch_len, delta, sizeof (int8_t))) {
            return false;
          }
          handled = true;
        }
        else if (b == 0x0F && off + 2 <= patch_len) {
          uint8_t b2 = orig[off + 1];
          if (b2 >= 0x80 && b2 <= 0x8F) { // two-byte jcc rel32
            if (!adjust_disp (orig, tramp, off + 1, patch_len, delta, sizeof (int32_t))) {
              return false;
            }
            handled = true;
          }
          else if (b2 == 0xA4 || b2 == 0xAC) {
            handled = true; // shld/shrd – no fixup
          }
        }
      }

  #if !defined(YSTL_ARCH_X64)
      (void)handled; // rip-relative fixups are x64-only, the flag is consulted there
  #endif

  #if defined(YSTL_ARCH_X64)
      // rip-relative addressing
      if (!handled && ilen >= 3) {
        size_t pm = 0;

        while (off + pm < patch_len) {
          auto p = orig[off + pm];
          if (p == 0x26 || p == 0x2E || p == 0x36 || p == 0x3E || p == 0x64 || p == 0x65 || p == 0x66 || p == 0x67 || p == 0xF0 || p == 0xF2 ||
              p == 0xF3) {
            ++pm;
          }
          else if ((p & 0xF0) == 0x40) {
            ++pm; // rex
          }
          else {
            break;
          }
        }

        size_t op_off = off + pm;
        size_t modrm_off = op_off + 1;

        if (orig[op_off] == 0x0F) {
          modrm_off = op_off + 2;

          if (op_off + 1 < off + ilen) {
            uint8_t b2 = orig[op_off + 1];

            if ((b2 == 0x38 || b2 == 0x3A) && op_off + 2 < off + ilen) {
              modrm_off = op_off + 3;
            }
          }
        }

        if (modrm_off < off + ilen) {
          uint8_t modrm = orig[modrm_off];
          uint8_t mod = (modrm >> 6) & 3;
          uint8_t rm = modrm & 7;

          size_t disp_off = 0;
          bool rip_rel = false;

          if (mod == 0 && rm == 5) {
            disp_off = modrm_off + 1;
            rip_rel = true;
          }
          else if (mod == 0 && rm == 4) {
            size_t sib_off = modrm_off + 1;

            if (sib_off < off + ilen) {
              uint8_t base = orig[sib_off] & 7;

              if (base == 5) {
                disp_off = sib_off + 1;
                rip_rel = true;
              }
            }
          }

          if (rip_rel && disp_off + 4 <= off + ilen) {
            auto disp = *reinterpret_cast<const int32_t *> (orig + disp_off);
            const auto new_disp = static_cast<int64_t> (disp) - delta;

            if (new_disp != static_cast<int32_t> (new_disp)) {
              return false; // the data is out of rel32 reach from the trampoline
            }
            memcpy (tramp + disp_off, &new_disp, 4);
          }
        }
      }
  #endif // ystl_arch_x64
      off += ilen;
    }

    return true;
  }

private:
  // one-byte opcodes whose operands include a modrm byte
  static constexpr uint8_t kModRM1[] = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x10,
    0x11,
    0x12,
    0x13,
    0x18,
    0x19,
    0x1A,
    0x1B,
    0x20,
    0x21,
    0x22,
    0x23,
    0x28,
    0x29,
    0x2A,
    0x2B,
    0x30,
    0x31,
    0x32,
    0x33,
    0x38,
    0x39,
    0x3A,
    0x3B,
    0x80,
    0x81,
    0x82,
    0x83,
    0x88,
    0x89,
    0x8A,
    0x8B,
    0x8C,
    0x8D,
    0x8E,
    0x8F,
    0xC0,
    0xC1,
    0xC6,
    0xC7,
    0xD0,
    0xD1,
    0xD2,
    0xD3,
    0xF6,
    0xF7,
    0xFE,
    0xFF,
  };

  // two-byte opcodes (0f xx) that do not take a modrm byte
  static constexpr uint8_t kNoModRM2[] = {
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x0E,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x77,
    0xA2,
    0xC8,
    0xC9,
    0xCA,
    0xCB,
    0xCC,
    0xCD,
    0xCE,
    0xCF,
  };
};

// platform-level function hooking
template <typename T> class Detour final : public NonCopyable {
private:
  static constexpr size_t PtrSize = sizeof (void *);

  #if defined(YSTL_ARCH_X64)
  static constexpr size_t JmpSize = 12;
  static constexpr size_t AddrOff = 2;
  #else
  static constexpr size_t JmpSize = 5;
  static constexpr size_t AddrOff = 0;
  #endif

  using uintptr = ystl::conditional_t<sizeof (void *) == 8, uint64_t, uint32_t>;

  Mutex cs_;

  void *original_ { nullptr };
  void *detour_ { nullptr };
  void *trampoline_ { nullptr };

  Array<uint8_t> saved_bytes_ {};
  size_t actual_patch_ { 0 };
  bool patched_ { false };

  bool has_endbr_ { false };
  size_t endbr_len_ { 0 };

  #if !defined(YSTL_WINDOWS)
  unsigned long page_size_ { 0 };
  uintptr page_start_ { 0 };
  #endif

  size_t patch_size () const noexcept {
    return actual_patch_ ? actual_patch_ : (JmpSize + endbr_len_);
  }

  // rel32 instructions reach at most 2gb, so the trampoline must stay nearby
  static constexpr uintptr kTrampolineMaxDistance = 0x7FFF0000; // just under 2gb
  static constexpr uintptr kTrampolineStep = 0x10000; // windows allocation granularity

  // plain allocation without any proximity requirements (32-bit builds)
  void *allocate_raw (size_t size) const noexcept {
  #if defined(YSTL_WINDOWS)
    return VirtualAlloc (nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  #else
    auto *result = mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (result == MAP_FAILED) {
      return nullptr;
    }
    return result;
  #endif
  }

  // tries to map at the hinted address, callers must check the distance
  void *allocate_near (uintptr hint, size_t size) const noexcept {
    auto *addr = reinterpret_cast<void *> (hint & ~(kTrampolineStep - 1));

  #if defined(YSTL_WINDOWS)
    auto *result = VirtualAlloc (addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!result) {
      return nullptr;
    }
  #else
    auto *result = mmap (addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (result == MAP_FAILED) {
      return nullptr;
    }

    const auto got = reinterpret_cast<uintptr> (result);
    auto diff = static_cast<intptr_t> (got - reinterpret_cast<uintptr> (original_));

    if (diff > static_cast<intptr_t> (kTrampolineMaxDistance) || diff < -static_cast<intptr_t> (kTrampolineMaxDistance)) {
      munmap (result, size);
      return nullptr;
    }
  #endif
    return result;
  }

  // allocates the trampoline within int32 reach of the target on x64
  void *alloc_trampoline (size_t size) const noexcept {
  #if !defined(YSTL_ARCH_X64)
    return allocate_raw (size);
  #else
    const auto base = reinterpret_cast<uintptr> (original_);

    for (uintptr offset = 0; offset <= kTrampolineMaxDistance; offset += kTrampolineStep) {
      // below the target, skipping the first iteration (same as above)
      if (offset != 0 && offset <= base) {
        if (auto *ptr = allocate_near (base - offset, size)) {
          return ptr;
        }
      }
      if (auto *ptr = allocate_near (base + offset, size)) {
        return ptr;
      }
    }
    return nullptr;
  #endif
  }

  bool build_trampoline () noexcept {
    size_t size = patch_size () + JmpSize;

    trampoline_ = alloc_trampoline (size);

    if (!trampoline_) {
      return false;
    }

    memcpy (trampoline_, saved_bytes_.data (), patch_size ());

    // fix up relative displacements
    {
      auto delta = reinterpret_cast<intptr_t> (trampoline_) - reinterpret_cast<intptr_t> (original_);

      if (delta != 0 && !X86Instr::fixup_relatives (
                          reinterpret_cast<uint8_t *> (trampoline_), reinterpret_cast<const uint8_t *> (original_), patch_size (), delta)) {
        // some relocation is out of reach even after the fixup attempts
        free_trampoline ();
        return false;
      }
    }
    auto buf = reinterpret_cast<uint8_t *> (trampoline_) + patch_size ();

  #if defined(YSTL_ARCH_X64)
    {
      auto target = reinterpret_cast<void *> (reinterpret_cast<uintptr> (original_) + patch_size ());

      buf[0] = 0x48;
      buf[1] = 0xB8;

      memcpy (buf + AddrOff, &target, PtrSize);
      buf[AddrOff + PtrSize] = 0xFF;
      buf[AddrOff + PtrSize + 1] = 0xE0;
    }
  #else
    {
      auto jump_target = reinterpret_cast<uintptr> (original_) + patch_size ();
      auto rel32 = static_cast<int32_t> (jump_target - (reinterpret_cast<uintptr> (buf) + 5));

      buf[0] = 0xE9;
      memcpy (buf + 1, &rel32, 4);
    }
  #endif

  #if defined(YSTL_WINDOWS)
    {
      DWORD old_protect {};

      if (!VirtualProtect (trampoline_, size, PAGE_EXECUTE_READ, &old_protect)) {
        free_trampoline ();
        return false;
      }
    }
  #else
    if (mprotect (trampoline_, size, PROT_READ | PROT_EXEC) == -1) {
      free_trampoline ();
      return false;
    }
  #endif

    return true;
  }

  void free_trampoline () noexcept {
    if (!trampoline_) {
      return;
    }

  #if defined(YSTL_WINDOWS)
    VirtualFree (trampoline_, 0, MEM_RELEASE);
  #else
    munmap (trampoline_, patch_size () + JmpSize);
  #endif

    trampoline_ = nullptr;
  }

  bool patch_memory (const Array<uint8_t> &to, bool patched) noexcept {
    MutexScopedLock lock (cs_);
    patched_ = patched;

  #if defined(YSTL_WINDOWS)
    unsigned long old_protect {};

    if (!VirtualProtect (original_, to.size (), PAGE_EXECUTE_READWRITE, &old_protect)) {
      return false;
    }
    memcpy (original_, to.data (), to.size ());
    FlushInstructionCache (GetCurrentProcess (), original_, to.size ());

    return VirtualProtect (original_, to.size (), old_protect, &old_protect) != 0;

  #else
    auto page_addr = reinterpret_cast<void *> (page_start_);

    if (mprotect (page_addr, page_size_, PROT_READ | PROT_WRITE) == -1) {
      return false;
    }
    memcpy (original_, to.data (), to.size ());

    #if defined(YSTL_CXX_CLANG) || defined(YSTL_CXX_GCC)
    __builtin___clear_cache (reinterpret_cast<char *> (original_), reinterpret_cast<char *> (original_) + to.size ());
    #endif

    if (mprotect (page_addr, page_size_, PROT_READ | PROT_EXEC) == -1) {
      return false;
    }

    return true;
  #endif
  }

public:
  explicit Detour () = default;

  Detour (StringRef module, StringRef name, T *address) {
    initialize (module, name, address);
  }

  ~Detour () {
    restore ();
    free_trampoline ();
  }

  void initialize ([[maybe_unused]] StringRef module, [[maybe_unused]] StringRef name, T *address) noexcept {
  #if !defined(YSTL_WINDOWS)
    auto ptr = reinterpret_cast<uint8_t *> (address);

    #if defined(YSTL_ARCH_X64)
    while (*reinterpret_cast<uint16_t *> (ptr) == 0x25ff) {
      auto disp = *reinterpret_cast<int32_t *> (ptr + 2);
      auto iat = reinterpret_cast<uint8_t **> (ptr + 6 + disp);

      ptr = *iat;
    }
    #else
    while (*reinterpret_cast<uint16_t *> (ptr) == 0x25ff) {
      ptr = *reinterpret_cast<uint8_t **> (ptr + 2);
    }
    #endif

    original_ = ptr;
    page_size_ = static_cast<unsigned long> (sysconf (_SC_PAGE_SIZE));
  #else
    auto handle = GetModuleHandleA (module.chars ());

    if (!handle) {
      original_ = reinterpret_cast<void *> (address);
    }
    else {
      original_ = reinterpret_cast<void *> (GetProcAddress (handle, name.chars ()));

      if (!original_) {
        original_ = reinterpret_cast<void *> (address);
      }
    }
  #endif
    auto bytes = reinterpret_cast<const uint8_t *> (original_);

    if (bytes[0] == 0xF3 && bytes[1] == 0x0F && bytes[2] == 0x1E && (bytes[3] == 0xFA || bytes[3] == 0xFB)) {
      has_endbr_ = true;
      endbr_len_ = 4;
    }
  }

  void install (void *detour, bool enable = false) noexcept {
    if (!original_) {
      return;
    }
    detour_ = detour;

  #if !defined(YSTL_WINDOWS)
    page_start_ = reinterpret_cast<uintptr> (original_) & ~(page_size_ - 1);
  #endif

    // compute instruction-aligned patch size
    {
      auto base = reinterpret_cast<const uint8_t *> (original_);
      actual_patch_ = endbr_len_ + X86Instr::align_boundary (base + endbr_len_, JmpSize);
    }
    saved_bytes_.resize (patch_size ());
    memcpy (saved_bytes_.data (), original_, patch_size ());

    if (!build_trampoline ()) {
      return;
    }

    if (enable) {
      this->detour ();
    }
  }

  bool valid () const noexcept {
    return original_ && detour_;
  }

  bool detoured () const noexcept {
    return patched_;
  }

  bool detour () noexcept {
    if (!valid ()) {
      return false;
    }

    auto psz = patch_size ();
    Array<uint8_t> data {};

    data.resize (psz);
    size_t off = 0;

  #if defined(YSTL_ARCH_X64) || defined(YSTL_ARCH_X32)
    if (has_endbr_) {
      data[0] = 0xF3;
      data[1] = 0x0F;
      data[2] = 0x1E;

    #if defined(YSTL_ARCH_X64)
      data[3] = 0xFA;
    #else
      data[3] = 0xFB;
    #endif

      off = endbr_len_;
    }
  #endif

  #if defined(YSTL_ARCH_X64)
    data[off] = 0x48;
    data[off + 1] = 0xB8;

    memcpy (data.data () + off + AddrOff, &detour_, PtrSize);

    data[off + AddrOff + PtrSize] = 0xFF;
    data[off + AddrOff + PtrSize + 1] = 0xE0;
  #else
    {
      auto rel32 = static_cast<int32_t> (reinterpret_cast<uintptr_t> (detour_) - (reinterpret_cast<uintptr_t> (original_) + off + 5));

      data[off] = 0xE9;
      memcpy (data.data () + off + 1, &rel32, 4);
    }
  #endif

    // nop-padding beyond jmp size (when alignment extended the region)
    auto jmp_end = off + JmpSize;

    if (jmp_end < psz) {
      memset (data.data () + jmp_end, 0x90, psz - jmp_end);
    }

    return patch_memory (data, true);
  }

  bool restore () noexcept {
    if (!valid ()) {
      return false;
    }
    return patch_memory (saved_bytes_, false);
  }

  template <typename... Args> decltype (auto) operator() (Args &&...args) const noexcept {
    auto fn = reinterpret_cast<T *> (trampoline_ ? trampoline_ : original_);
    return fn (ystl::forward<Args> (args)...);
  }
};

}

#endif
