// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/singleton.h>
#include <ystl/utility.h>

#if !defined(YSTL_ARCH_NON_X86)
  #if defined(YSTL_WINDOWS) && (defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL))
    #include <intrin.h>
  #elif !defined(YSTL_EMSCRIPTEN)
    #include <cpuid.h>
  #endif
#elif (defined(YSTL_LINUX) || defined(YSTL_ANDROID)) && (defined(YSTL_ARCH_ARM) || defined(YSTL_ARCH_RISCV) || defined(YSTL_ARCH_S390X))
  #include <sys/auxv.h>
#endif

namespace ystl {

// cpu flags for current cpu
class CpuFlags final : public Singleton<CpuFlags> {
public:
  // x86 simd
  bool sse3 {}, ssse3 {}, sse41 {}, sse42 {};
  bool avx {}, avx2 {}, avx512f {};
  bool fma {}, f16c {}, popcnt {};
  bool bmi1 {}, bmi2 {};
  bool aesni {}, shani {};

  // arm
  bool neon {}, fp16 {}, dotprod {};
  bool sve {}, sve2 {};
  bool bf16 {}, i8mm {};
  bool aes {}, sha1 {}, sha2 {};
  bool pmull {}, crc32 {}, atomics {};

  // risc-v
  bool rvv {};

  // s390x
  bool vx {};

public:
  CpuFlags () {
    detect ();
  }

  ~CpuFlags () = default;

private:
  void detect () {
#if !defined(YSTL_ARCH_NON_X86) && !defined(YSTL_EMSCRIPTEN)
    detect_x86 ();
#elif defined(YSTL_ARCH_ARM)
    detect_arm ();
#elif defined(YSTL_ARCH_RISCV)
    detect_risc_v ();
#elif defined(YSTL_ARCH_S390X)
    detect_s390x ();
#endif
  }

#if !defined(YSTL_ARCH_NON_X86) && !defined(YSTL_EMSCRIPTEN)
  [[nodiscard]] static uint32_t get_xcr0 () noexcept {
  #if defined(YSTL_WINDOWS) && (defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL))
    return static_cast<uint32_t> (_xgetbv (0));
  #else
    uint32_t lo = 0, hi = 0;
    __asm__ volatile ("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
    return lo;
  #endif
  }

  void detect_x86 () {
    enum : int32_t {
      eax,
      ebx,
      ecx,
      edx,
      count
    };
    uint32_t data[count] {};

    // leaf 1
  #if defined(YSTL_WINDOWS) && (defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL))
    __cpuidex (reinterpret_cast<int32_t *> (data), 0, 0);
  #else
    __get_cpuid (0x0, &data[eax], &data[ebx], &data[ecx], &data[edx]);
  #endif

    const auto max_leaf = data[eax];

  #if defined(YSTL_WINDOWS) && (defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL))
    __cpuidex (reinterpret_cast<int32_t *> (data), 1, 0);
  #else
    __get_cpuid (0x1, &data[eax], &data[ebx], &data[ecx], &data[edx]);
  #endif

    // avx needs os save support (xcr0), bare hardware bit faults without it
    const auto os_xsave = !!(data[ecx] & ystl::bit (27));
    const auto xcr0 = os_xsave ? get_xcr0 () : 0u;
    const auto avx_usable = os_xsave && (xcr0 & 0x6) == 0x6;

    sse3 = !!(data[ecx] & ystl::bit (0));
    ssse3 = !!(data[ecx] & ystl::bit (9));
    fma = avx_usable && !!(data[ecx] & ystl::bit (12));
    sse41 = !!(data[ecx] & ystl::bit (19));
    sse42 = !!(data[ecx] & ystl::bit (20));
    popcnt = !!(data[ecx] & ystl::bit (23));
    aesni = !!(data[ecx] & ystl::bit (25));
    avx = avx_usable && !!(data[ecx] & ystl::bit (28));
    f16c = avx_usable && !!(data[ecx] & ystl::bit (29));

    // leaf 7, sub-leaf 0 (absent on pre-2013 cpus, leaves garbage without the guard)
    if (max_leaf >= 7) [[likely]] {
  #if defined(YSTL_WINDOWS) && (defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL))
      __cpuidex (reinterpret_cast<int32_t *> (data), 7, 0);
  #else
      __get_cpuid (0x7, &data[eax], &data[ebx], &data[ecx], &data[edx]);
  #endif

      const auto avx512_usable = avx_usable && (xcr0 & 0xE0) == 0xE0;

      bmi1 = !!(data[ebx] & ystl::bit (3));
      avx2 = avx_usable && !!(data[ebx] & ystl::bit (5));
      bmi2 = !!(data[ebx] & ystl::bit (8));
      avx512f = avx512_usable && !!(data[ebx] & ystl::bit (16));
      shani = !!(data[ebx] & ystl::bit (29));
    }
  }
#endif

#if defined(YSTL_ARCH_ARM)
  void detect_arm () {
  #if defined(YSTL_ARCH_ARM64)
    detect_arm64 ();
  #elif defined(YSTL_ARCH_ARM32)
    detect_arm32 ();
  #endif
  }

  #if defined(YSTL_ARCH_ARM64)
  void detect_arm64 () {
    neon = true;

    #if defined(YSTL_LINUX) || defined(YSTL_ANDROID)
    auto hwcap = getauxval (AT_HWCAP);
    auto hwcap2 = getauxval (AT_HWCAP2);

    aes = !!(hwcap & ystl::bit (3));
    pmull = !!(hwcap & ystl::bit (4));
    sha1 = !!(hwcap & ystl::bit (5));
    sha2 = !!(hwcap & ystl::bit (6));
    crc32 = !!(hwcap & ystl::bit (7));
    atomics = !!(hwcap & ystl::bit (8));
    fp16 = !!(hwcap & ystl::bit (10));
    dotprod = !!(hwcap & ystl::bit (20));
    sve = !!(hwcap & ystl::bit (22));

    sve2 = !!(hwcap2 & ystl::bit (1));
    i8mm = !!(hwcap2 & ystl::bit (13));
    bf16 = !!(hwcap2 & ystl::bit (14));
    #else
        // compile-time detection for non-linux platforms (macos, etc.)
      #if defined(__ARM_FEATURE_CRC32)
    crc32 = true;
      #endif
      #if defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO)
    aes = true;
    pmull = true;
      #endif
      #if defined(__ARM_FEATURE_SHA2) || defined(__ARM_FEATURE_CRYPTO)
    sha1 = true;
    sha2 = true;
      #endif
      #if defined(__ARM_FEATURE_ATOMICS)
    atomics = true;
      #endif
      #if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
    fp16 = true;
      #endif
      #if defined(__ARM_FEATURE_DOTPROD)
    dotprod = true;
      #endif
      #if defined(__ARM_FEATURE_SVE)
    sve = true;
      #endif
      #if defined(__ARM_FEATURE_SVE2)
    sve2 = true;
      #endif
      #if defined(__ARM_FEATURE_BF16)
    bf16 = true;
      #endif
      #if defined(__ARM_FEATURE_MATMUL_INT8)
    i8mm = true;
      #endif
    #endif
  }
  #endif

  #if defined(YSTL_ARCH_ARM32)
  void detect_arm32 () {
    #if defined(YSTL_LINUX) || defined(YSTL_ANDROID)
    auto hwcap = getauxval (AT_HWCAP);
    auto hwcap2 = getauxval (AT_HWCAP2);

    neon = !!(hwcap & ystl::bit (12));

    aes = !!(hwcap2 & ystl::bit (0));
    pmull = !!(hwcap2 & ystl::bit (1));
    sha1 = !!(hwcap2 & ystl::bit (2));
    sha2 = !!(hwcap2 & ystl::bit (3));
    crc32 = !!(hwcap2 & ystl::bit (4));
    #else
      #if defined(__ARM_NEON)
    neon = true;
      #endif
      #if defined(__ARM_FEATURE_CRC32)
    crc32 = true;
      #endif
      #if defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO)
    aes = true;
    pmull = true;
      #endif
      #if defined(__ARM_FEATURE_SHA2) || defined(__ARM_FEATURE_CRYPTO)
    sha1 = true;
    sha2 = true;
      #endif
    #endif
  }
  #endif
#endif

#if defined(YSTL_ARCH_RISCV)
  void detect_risc_v () {
  #if defined(YSTL_LINUX)
    enum : uint32_t {
      hwcap_v = ystl::bit ('V' - 'A')
    };
    rvv = !!(getauxval (AT_HWCAP) & hwcap_v);
  #elif defined(__riscv_vector)
    rvv = true;
  #endif
  }
#endif

#if defined(YSTL_ARCH_S390X)
  void detect_s390x () {
  #if defined(YSTL_LINUX)
    enum : uint32_t {
      hwcap_vx = ystl::bit (1)
    };
    vx = !!(getauxval (AT_HWCAP) & hwcap_vx);
  #endif
  }
#endif
};

// expose platform singleton
YSTL_EXPOSE_GLOBAL_SINGLETON (CpuFlags, cpuflags);

}
