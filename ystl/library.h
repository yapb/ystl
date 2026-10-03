// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/string.h>
#include <ystl/utility.h>

#if !defined(YSTL_WINDOWS)
  #if defined(YSTL_PSVITA)
    #define VRTLD_LIBDL_COMPAT
    #include <vrtld.h>
  #else
    #include <dlfcn.h>
  #endif
#endif

namespace ystl {

// shared library handle with owned or borrowed ownership model
class SharedLibrary final : public NonCopyable {
public:
#if defined(YSTL_WINDOWS)
  using Handle = HMODULE;
  using Func = FARPROC;
#else
  using Handle = void *;
  using Func = void *;
#endif

private:
  Handle handle_ = nullptr;
  bool unloadable_ = true;

public:
  SharedLibrary () = default;

  SharedLibrary (const SharedLibrary &) = delete;
  SharedLibrary &operator= (const SharedLibrary &) = delete;

  explicit SharedLibrary (StringRef file) {
    if (file.empty ()) {
      return;
    }
    load (file);
  }

  ~SharedLibrary () {
    unload ();
  }

  SharedLibrary (SharedLibrary &&rhs) noexcept {
    handle_ = rhs.handle_;
    unloadable_ = rhs.unloadable_;
    rhs.handle_ = nullptr;
    rhs.unloadable_ = true;
  }

public:
  // note: file must point to a null-terminated string; a non-terminated stringref view (e.g
  bool load (StringRef file, bool unloadable = true) noexcept {
    if (file.empty ()) {
      return false;
    }

    if (handle_ && unloadable_) {
      unload ();
    }
    handle_ = nullptr;
    unloadable_ = unloadable;

#if defined(YSTL_WINDOWS)
    handle_ = LoadLibraryA (file.chars ());
#else
    auto load_flags = RTLD_NOW | RTLD_LOCAL;

    // rtld_deepbind breaks under asan (interceptors live in the main executable), so skip it there
  #if defined(YSTL_POSIX) && !defined(YSTL_ANDROID) && defined(__GLIBC__)
    #if defined(__SANITIZE_ADDRESS__)
        // sanitized: keep loadflags as is
    #elif defined(__has_feature)
      #if !__has_feature(address_sanitizer)
    load_flags |= RTLD_DEEPBIND;
      #endif
    #else
    load_flags |= RTLD_DEEPBIND;
    #endif
  #endif
    handle_ = dlopen (file.chars (), load_flags);
#endif
    return handle_ != nullptr;
  }

  [[nodiscard]] static String path (void *address) {
#if defined(YSTL_WINDOWS)
    MEMORY_BASIC_INFORMATION mbi {};

    if (!VirtualQuery (address, &mbi, sizeof (mbi))) {
      return "";
    }

    if (mbi.State != MEM_COMMIT) {
      return "";
    }

    char dllpath[MAX_PATH] = { 0 };
    const auto length = GetModuleFileNameA (reinterpret_cast<Handle> (mbi.AllocationBase), dllpath, static_cast<DWORD> (sizeof (dllpath)));

    if (length == 0) {
      return "";
    }
    // truncated path still returned when the buffer proves too small
    return dllpath;
#else
    Dl_info dli;
    ystl::memzero (&dli, sizeof (dli));

    if (dladdr (address, &dli)) {
      return dli.dli_fname;
    }
    return "";
#endif
  }

  // borrows the module containing address; the handle is never unloaded
  bool locate (void *address) {
#if defined(YSTL_WINDOWS)
    MEMORY_BASIC_INFORMATION mbi {};

    if (!VirtualQuery (address, &mbi, sizeof (mbi))) {
      return false;
    }

    if (mbi.State != MEM_COMMIT) {
      return false;
    }

    if (handle_ && unloadable_) {
      unload ();
    }
    handle_ = reinterpret_cast<Handle> (mbi.AllocationBase);
    unloadable_ = false;
    return handle_ != nullptr;
#else
    Dl_info dli;
    ystl::memzero (&dli, sizeof (dli));

    if (!dladdr (address, &dli) || !dli.dli_fname) {
      return false;
    }
    return load (dli.dli_fname, false);
#endif
  }

  // releases the handle only when owned, borrowed handles stay valid
  void unload () noexcept {
    if (!handle_ || !unloadable_) {
      return;
    }

#if defined(YSTL_WINDOWS)
    FreeLibrary (static_cast<HMODULE> (handle_));
#else
    dlclose (handle_);
#endif
    handle_ = nullptr;
  }

  // note: a null symbol address is indistinguishable from not found
  template <typename R> [[nodiscard]] R resolve (StringRef fn) const {
    static_assert (ystl::is_pointer_v<R>, "resolve<R>: R must be a pointer type");

    if (!*this) {
      return nullptr;
    }
    return SharedLibrary::get_symbol<R> (handle (), fn);
  }

  Handle handle () const {
    return handle_;
  }

  // error text for the last failed load or resolve call
  [[nodiscard]] static String last_error () {
#if defined(YSTL_WINDOWS)
    const auto code = GetLastError ();

    if (code == 0) {
      return "";
    }
    char buffer[512] = { 0 };

    const auto length = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
      MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, static_cast<DWORD> (sizeof (buffer) - 1), nullptr);

    if (length == 0) {
      return "";
    }
    return buffer;
#else
    const char *error = dlerror ();
    return error ? error : "";
#endif
  }

public:
  explicit operator bool () const {
    return handle_ != nullptr;
  }

  bool valid () const {
    return operator bool ();
  }

  SharedLibrary &operator= (SharedLibrary &&rhs) noexcept {
    if (this != &rhs) {
      unload ();

      handle_ = rhs.handle_;
      unloadable_ = rhs.unloadable_;

      rhs.handle_ = nullptr;
      rhs.unloadable_ = true;
    }
    return *this;
  }

public:
  // note: fn must point to a null-terminated string
  template <typename R> static R YSTL_STDCALL get_symbol (Handle module, StringRef fn) {
    static_assert (ystl::is_pointer_v<R>, "getSymbol<R>: R must be a pointer type");

    if (!module) {
      return nullptr;
    }
#if defined(YSTL_WINDOWS)
    return reinterpret_cast<R> (GetProcAddress (static_cast<HMODULE> (module), fn.chars ()));
#else
    return reinterpret_cast<R> (dlsym (module, fn.chars ()));
#endif
  }

  // note: mod must point to a null-terminated string
  [[nodiscard]] static bool has_module (StringRef mod) {
    if (mod.empty ()) {
      return false;
    }
#if defined(YSTL_WINDOWS)
    return GetModuleHandleA (mod.chars ()) != nullptr;
#else
    // probe module presence with noload without triggering a load
  #ifdef RTLD_NOLOAD
    void *handle = dlopen (mod.chars (), RTLD_LAZY | RTLD_NOLOAD);
  #else
    void *handle = dlopen (mod.chars (), RTLD_LAZY);
  #endif
    if (handle) {
      dlclose (handle);
    }
    return handle != nullptr;
#endif
  }
};

// helper for dylink hooking across windows and posix signatures
struct PlatformDynlink {
#if defined(YSTL_WINDOWS)
  static constexpr StringRef DlopenName = "LoadLibraryA";
  static constexpr StringRef DlcloseName = "FreeLibrary";
  static constexpr StringRef DlsymName = "GetProcAddress";

  static inline auto Dlopen = LoadLibraryA;
  static inline auto Dlclose = FreeLibrary;
  static inline auto Dlsym = GetProcAddress;

  using DlopenType = decltype (LoadLibraryA);
  using DlcloseType = decltype (FreeLibrary);
  using DlsymType = decltype (GetProcAddress);

#else // posix (linux, macos, psvita, etc.)
  static constexpr StringRef DlopenName = "dlopen";
  static constexpr StringRef DlcloseName = "dlclose";
  static constexpr StringRef DlsymName = "dlsym";

  #if defined(YSTL_PSVITA) // just a shim
  static constexpr auto Dlopen = vrtld_dlopen;
  static constexpr auto Dlclose = vrtld_dlclose;
  static constexpr auto Dlsym = vrtld_dlsym;

  // decltype of the shim keeps the alias matched to vrtld signature
  using DlopenType = decltype (vrtld_dlopen);
  using DlcloseType = decltype (vrtld_dlclose);
  using DlsymType = decltype (vrtld_dlsym);
  #else
  static constexpr auto Dlopen = dlopen;
  static constexpr auto Dlclose = dlclose;
  static constexpr auto Dlsym = dlsym;

  using DlopenType = decltype (dlopen);
  using DlcloseType = decltype (dlclose);
  using DlsymType = decltype (dlsym);
  #endif
#endif
};

}
