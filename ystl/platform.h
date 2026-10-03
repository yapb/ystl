// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/fixedarray.h>
#include <ystl/singleton.h>
#include <ystl/utility.h>

namespace ystl {

// detects the build platform
#if defined(__linux__)
  #define YSTL_LINUX
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  #define YSTL_FREEBSD
#elif defined(__GNU__) || defined(__gnu_hurd__)
  #define YSTL_HURD
#elif defined(__OpenBSD__)
  #define YSTL_OPENBSD
#elif defined(__NetBSD__)
  #define YSTL_NETBSD
#elif defined(__DragonFly__)
  #define YSTL_DRAGONFLY
#elif defined(__APPLE__)
  #define YSTL_MACOS
#elif defined(_WIN32)
  #define YSTL_WINDOWS
#elif defined(__HAIKU__)
  #define YSTL_HAIKU
#elif defined(__sun) && defined(__SVR4)
  #define YSTL_SOLARIS
#endif

// bsd family umbrella
#if defined(YSTL_FREEBSD) || defined(YSTL_OPENBSD) || defined(YSTL_NETBSD) || defined(YSTL_DRAGONFLY)
  #define YSTL_BSD
#endif

// posix umbrella for all unix-like platforms
#if defined(YSTL_LINUX) || defined(YSTL_BSD) || defined(YSTL_MACOS) || defined(YSTL_HURD) || defined(YSTL_HAIKU) || defined(YSTL_SOLARIS)
  #define YSTL_POSIX
#endif

#if defined(__EMSCRIPTEN__)
  #define YSTL_EMSCRIPTEN
#endif

#if defined(__ANDROID__)
  #define YSTL_ANDROID
#endif

#if defined(__vita__)
  #define YSTL_PSVITA
#endif

#if !defined(YSTL_DEBUG) && (defined(DEBUG) || defined(_DEBUG))
  #define YSTL_DEBUG
#endif

// detects the compiler
#if defined(_MSC_VER)
  #define YSTL_CXX_MSVC _MSC_VER
#endif

#if defined(__clang__)
  #define YSTL_CXX_CLANG __clang__
#endif

#if defined(_CLANG_CL)
  #define YSTL_CXX_CLANG_CL _CLANG_CL
#endif

#if defined(__GNUC__)
  #define YSTL_CXX_GCC __GNUC__
#endif

// configure macroses
#define YSTL_C_LINKAGE extern "C"

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(__aarch64__) ||                               \
  (defined(_MSC_VER) && defined(_M_X64)) || defined(__powerpc64__) || (__riscv_xlen == 64)
  #define YSTL_ARCH_X64
#elif defined(__i686) || defined(__i686__) || defined(__i386) || defined(__i386__) || defined(i386) ||                                          \
  (defined(_MSC_VER) && defined(_M_IX86)) || defined(__powerpc__) || (__riscv_xlen == 32)
  #define YSTL_ARCH_X32
#endif

#if defined(__arm__)
  #define YSTL_ARCH_ARM32
#elif defined(__aarch64__)
  #define YSTL_ARCH_ARM64
#endif
#if defined(YSTL_ARCH_ARM32) || defined(YSTL_ARCH_ARM64)
  #define YSTL_ARCH_ARM
#endif

#if defined(__powerpc64__)
  #define YSTL_ARCH_PPC64
#elif defined(__powerpc__)
  #define YSTL_ARCH_PPC32
#endif

#if defined(__riscv)
  #define YSTL_ARCH_RISCV
#endif

#if defined(__s390x__)
  #define YSTL_ARCH_S390X
#endif

#if defined(YSTL_ARCH_PPC32) || defined(YSTL_ARCH_PPC64)
  #define YSTL_ARCH_PPC
#endif

#if defined(YSTL_ARCH_ARM) || defined(YSTL_ARCH_PPC) || defined(YSTL_ARCH_RISCV) || defined(YSTL_ARCH_S390X)
  #define YSTL_ARCH_NON_X86
#endif

#if !defined(YSTL_DISABLE_SIMD)
  #if !defined(YSTL_ARCH_NON_X86)
    #if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
      #define YSTL_HAS_SIMD_SSE
    #endif
  #elif defined(__ARM_NEON)
    #define YSTL_HAS_SIMD_NEON
  #elif defined(__riscv_vector)
    #define YSTL_HAS_SIMD_RVV
  #endif
#endif

#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
  #define YSTL_FORCE_INLINE __forceinline
  #define YSTL_NOINLINE __declspec(noinline)
#else
  #define YSTL_FORCE_INLINE __attribute__((__always_inline__)) inline
  #define YSTL_NOINLINE __attribute__((__noinline__))
#endif

#if defined(YSTL_HAS_SIMD_SSE) || defined(YSTL_HAS_SIMD_NEON) || defined(YSTL_HAS_SIMD_RVV)
  #define YSTL_HAS_SIMD
#endif

#if defined(YSTL_HAS_SIMD)
  #if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    #define YSTL_SIMD_ALIGNED __declspec(align(16))
  #else
    #define YSTL_SIMD_ALIGNED __attribute__((aligned(16)))
  #endif
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define YSTL_ARCH_CPU_BIG_ENDIAN
#endif

// define export macros
#if !defined(__GNUC__) || defined(YSTL_WINDOWS)
  #define YSTL_FORCE_STACK_ALIGN
  #define YSTL_EXPORT YSTL_C_LINKAGE __declspec(dllexport)
  #define YSTL_STDCALL __stdcall
#else
  #if defined(__i386__)
    #define YSTL_FORCE_STACK_ALIGN __attribute__((force_align_arg_pointer,noinline))
  #else
    #define YSTL_FORCE_STACK_ALIGN
  #endif
  #define YSTL_EXPORT YSTL_C_LINKAGE YSTL_FORCE_STACK_ALIGN __attribute__((visibility("default"),used))
  #define YSTL_STDCALL
#endif

// msvc and clang-cl provide us placement new by default
#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
  #define __PLACEMENT_NEW_INLINE
#endif

#if (defined(YSTL_CXX_MSVC) && !defined(YSTL_CXX_CLANG)) || defined(YSTL_ARCH_NON_X86)
  #define YSTL_SIMD_TARGET(dest) YSTL_FORCE_INLINE
  #define YSTL_SIMD_TARGET_AIL(dest) YSTL_FORCE_INLINE
  #define YSTL_SIMD_TARGET_TIL(dest) inline
#else
  #define YSTL_SIMD_TARGET(dest) __attribute__((target(dest)))
  #define YSTL_SIMD_TARGET_AIL(dest) __attribute__((__always_inline__,target(dest))) inline
  #define YSTL_SIMD_TARGET_TIL(dest) __attribute__((target(dest))) inline
#endif

// fake asm op to break gcc ssa propagation, silences false-positives
#if defined(YSTL_CXX_GCC)
  #define YSTL_SATISFY_GCC_ANALYZER(x) __asm__ ("" : "+r"(x))
#else
  #define YSTL_SATISFY_GCC_ANALYZER(x)
#endif

#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
  #define YSTL_DISABLE_ANONYMOUS_UNION_WARNING __pragma (warning (push)) __pragma (warning (disable : 4201))
  #define YSTL_RESTORE_ANONYMOUS_UNION_WARNING __pragma (warning (pop))
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  #define YSTL_DISABLE_ANONYMOUS_UNION_WARNING _Pragma ("GCC diagnostic push") _Pragma ("GCC diagnostic ignored \"-Wpedantic\"")
  #define YSTL_RESTORE_ANONYMOUS_UNION_WARNING _Pragma ("GCC diagnostic pop")
#else
  #define YSTL_DISABLE_ANONYMOUS_UNION_WARNING
  #define YSTL_RESTORE_ANONYMOUS_UNION_WARNING
#endif

#if defined(YSTL_CXX_GCC) && !defined(YSTL_CXX_CLANG)
  #define YSTL_IGNORE_RESTRICT _Pragma ("GCC diagnostic ignored \"-Wrestrict\"")
#else
  #define YSTL_IGNORE_RESTRICT
#endif

#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  #define YSTL_DISABLE_RUNTIME_FORMAT_WARNING _Pragma ("GCC diagnostic push") _Pragma ("GCC diagnostic ignored \"-Wformat-security\"") _Pragma ("GCC diagnostic ignored \"-Wformat-nonliteral\"") YSTL_IGNORE_RESTRICT
  #define YSTL_RESTORE_RUNTIME_FORMAT_WARNING _Pragma ("GCC diagnostic pop")
#else
  #define YSTL_DISABLE_RUNTIME_FORMAT_WARNING
  #define YSTL_RESTORE_RUNTIME_FORMAT_WARNING
#endif

}

#if defined(YSTL_WINDOWS)
constexpr auto kPathSeparator = "\\";
constexpr auto kLibrarySuffix = ".dll";

  // raise windows api version if doesn't build for xp
  #if !defined(YSTL_HAS_WINXP_SUPPORT) && (!defined(YSTL_CXX_MSVC) && !defined(YSTL_CXX_CLANG_CL))
    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
    #define WINVER 0x0600
  #endif

  #if !defined(NOMINMAX)
    #define NOMINMAX
  #endif

  #define WIN32_LEAN_AND_MEAN
  #define NOGDICAPMASKS
  #define NOVIRTUALKEYCODES
  #define NOWINMESSAGES
  #define NOWINSTYLES
  #define NOSYSMETRICS
  #define NOMENUS
  #define NOICONS
  #define NOKEYSTATES
  #define NOSYSCOMMANDS
  #define NORASTEROPS
  #define NOSHOWWINDOW
  #define OEMRESOURCE
  #define NOATOM
  #define NOCLIPBOARD
  #define NOCOLOR
  #define NOCTLMGR
  #define NODRAWTEXT
  #define NOGDI
  #define NOKERNEL
  #define NONLS
  #define NOMEMMGR
  #define NOMETAFILE
  #define NOMSG
  #define NOOPENFILE
  #define NOSCROLL
  #define NOSERVICE
  #define NOSOUND
  #define NOTEXTMETRIC
  #define NOWH
  #define NOWINOFFSETS
  #define NOCOMM
  #define NOKANJI
  #define NOHELP
  #define NOPROFILER
  #define NODEFERWINDOWPOS
  #define NOMCX
  #define NOWINRES
  #define NOIME

  #include <windows.h>
  #include <direct.h>
  #include <process.h>
  #include <io.h>
#else
constexpr auto kPathSeparator = "/";

  #if defined(YSTL_MACOS)
constexpr auto kLibrarySuffix = ".dylib";
  #else
constexpr auto kLibrarySuffix = ".so";
  #endif

  #if defined(YSTL_PSVITA)
    #define FNM_CASEFOLD 0x10
    #include <sys/syslimits.h>
  #endif

  #include <unistd.h>
  #include <dirent.h>
  #include <fnmatch.h>
  #include <strings.h>
  #include <sys/time.h>
  #include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(YSTL_ANDROID)
  #include <android/log.h>
#endif

#include <time.h>

namespace ystl {

// helper struct for platform detection
struct Platform : public Singleton<Platform> {
  bool win = false;
  bool nix = false;
  bool linux_ = false;
  bool macos = false;
  bool bsd = false;
  bool hurd = false;
  bool android = false;
  bool x64 = false;
  bool arm = false;
  bool ppc = false;
  bool riscv = false;
  bool s390x = false;
  bool simd = false;
  bool psvita = false;
  bool emscripten = false;

  FixedArray<char, 64> app_name {};

  Platform () {
#if defined(YSTL_WINDOWS)
    win = true;
#endif

#if defined(YSTL_ANDROID)
    android = true;
#endif

#if defined(YSTL_POSIX)
    nix = true;
#endif

#if defined(YSTL_LINUX)
    linux_ = true;
#endif

#if defined(YSTL_MACOS)
    macos = true;
#endif

#if defined(YSTL_BSD)
    bsd = true;
#endif

#if defined(YSTL_HURD)
    hurd = true;
#endif

#if defined(YSTL_ARCH_X64) || defined(YSTL_ARCH_ARM64)
    x64 = true;
#endif

#if defined(YSTL_ARCH_ARM)
    arm = true;
#endif

#if defined(YSTL_ARCH_RISCV)
    riscv = true;
#endif

#if defined(YSTL_ARCH_S390X)
    s390x = true;
#endif

#if defined(YSTL_PSVITA)
    psvita = true;
#endif

#if defined(YSTL_ARCH_PPC)
    ppc = true;
#endif

#if defined(YSTL_EMSCRIPTEN)
    emscripten = true;
#endif

#if !defined(YSTL_DISABLE_SIMD)
    simd = true;
#endif
  }

  // set the app name
  void set_app_name (const char *name) {
    snprintf (app_name.data (), app_name.capacity (), "%s", name);
  }

  // running on no-x86 platform ?
  bool is_non_x86 () const {
    return arm || ppc || riscv || s390x;
  }

  // helper platform-dependant functions
  template <typename U> bool is_valid_ptr ([[maybe_unused]] U *ptr) {
#if defined(YSTL_WINDOWS)
  #if defined(YSTL_HAS_WINXP_SUPPORT)
    if (IsBadCodePtr (reinterpret_cast<FARPROC> (ptr))) {
      return false;
    }
  #else
    MEMORY_BASIC_INFORMATION mbi {};

    if (VirtualQuery (reinterpret_cast<LPVOID> (ptr), &mbi, sizeof (mbi))) {
      auto result = !!(
        mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));

      if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
        result = false;
      }
      return result;
    }
  #endif
#endif
    return true;
  }

  bool create_directory (const char *dir, [[maybe_unused]] int mode = -1) {
    int result = 1;
#if defined(YSTL_WINDOWS)
    result = _mkdir (dir);
#else
    mode_t actual_mode = static_cast<mode_t> (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (mode != -1) {
      actual_mode = static_cast<mode_t> (mode);
    }
    result = mkdir (dir, actual_mode);
#endif
    return result == 0;
  }

  bool remove_file (const char *dir) {
#if defined(YSTL_WINDOWS)
    return _unlink (dir) == 0;
#else
    return unlink (dir) == 0;
#endif
  }

  bool remove_directory (const char *dir) {
#if defined(YSTL_WINDOWS)
    return _rmdir (dir) == 0;
#else
    return rmdir (dir) == 0;
#endif
  }

  bool is_directory (const char *path) {
#if defined(YSTL_WINDOWS)
    const auto attributes = GetFileAttributesA (path);

    return attributes != INVALID_FILE_ATTRIBUTES && !!(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st {};

    return stat (path, &st) == 0 && S_ISDIR (st.st_mode);
#endif
  }

  size_t working_directory (char *buffer, size_t size) {
    if (!buffer || !size) {
      return 0;
    }
#if defined(YSTL_WINDOWS)
    const auto result = GetCurrentDirectoryA (static_cast<DWORD> (size), buffer);
#else
    const auto result = getcwd (buffer, size) ? strlen (buffer) : 0;
#endif
    if (!result) {
      buffer[0] = '\0';
    }
    return result;
  }

  bool set_working_directory (const char *dir) {
#if defined(YSTL_WINDOWS)
    return !!SetCurrentDirectoryA (dir);
#else
    return chdir (dir) == 0;
#endif
  }

  [[nodiscard]] float seconds () {
#if defined(YSTL_WINDOWS)
    LARGE_INTEGER count {}, freq {};

    count.QuadPart = 0;
    freq.QuadPart = 0;

    QueryPerformanceFrequency (&freq);
    QueryPerformanceCounter (&count);

    return static_cast<float> (count.QuadPart) / static_cast<float> (freq.QuadPart);
#else
    timeval tv;
    gettimeofday (&tv, nullptr);

    static auto start_time = tv.tv_sec;

    return static_cast<float> (tv.tv_sec - start_time);
#endif
  }

  [[noreturn]] void abort (const char *msg = "OUT OF MEMORY!") noexcept {
    fprintf (stderr, "%s\n", msg);

#if defined(YSTL_ANDROID)
    __android_log_write (ANDROID_LOG_ERROR, app_name.data (), msg);
#endif

#if defined(YSTL_WINDOWS)
    // dynamically loaded to avoid user32.lib linkage; only needed on crash
    if (HMODULE user32 = LoadLibraryA ("user32.dll")) {
      using DestroyWindowFn = BOOL (WINAPI *) (HWND);
      using GetForegroundWindowFn = HWND (WINAPI *) ();
      using GetActiveWindowFn = HWND (WINAPI *) ();
      using MessageBoxAFn = int (WINAPI *) (HWND, LPCSTR, LPCSTR, UINT);
      const auto destroy_window = reinterpret_cast<DestroyWindowFn> (GetProcAddress (user32, "DestroyWindow"));
      const auto get_foreground_window = reinterpret_cast<GetForegroundWindowFn> (GetProcAddress (user32, "GetForegroundWindow"));
      if (destroy_window && get_foreground_window) {
        destroy_window (get_foreground_window ());
      }
      const auto get_active_window = reinterpret_cast<GetActiveWindowFn> (GetProcAddress (user32, "GetActiveWindow"));
      const auto message_box = reinterpret_cast<MessageBoxAFn> (GetProcAddress (user32, "MessageBoxA"));
      if (message_box) {
        message_box (get_active_window ? get_active_window () : nullptr, msg, app_name.data (), MB_ICONSTOP);
      }
    } // no FreeLibrary: avoids DETACH on a possibly corrupt heap; OS reclaims on abort
#endif

#if defined(YSTL_DEBUG) && defined(YSTL_CXX_MSVC)
    DebugBreak ();
    ::abort (); // DebugBreak returns when continued without a debugger
#else
    ::abort ();
#endif
  }

  void loctime (tm *_tm, const time_t *_time) {
#if defined(YSTL_WINDOWS)
    localtime_s (_tm, _time);
#else
    localtime_r (_time, _tm);
#endif
  }

  [[nodiscard]] const char *env (const char *var) {
    static char result[384];
    ystl::memzero (result, ystl::bufsize (result));

#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    char *buffer = nullptr;
    size_t size = 0;

    if (_dupenv_s (&buffer, &size, var) == 0 && buffer != nullptr) {
      strncpy_s (result, buffer, sizeof (result));
      free (buffer);
    }
#else
    auto data = getenv (var);

    if (data) {
      strncpy (result, data, ystl::bufsize (result));
    }
#endif
    return result;
  }

  [[nodiscard]] const char *tmpfname () noexcept {
#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    static char name[L_tmpnam_s];
    tmpnam_s (name);

    if (name[0] == '\\') {
      for (auto i = 0; name[i] != '\0'; i++) {
        name[i] = name[i + 1];
      }
    }
#else
    char templ[PATH_MAX] { "/tmp/crtmp-XXXXXX" };
    static char name[PATH_MAX] {};

    strncpy (name, templ, ystl::bufsize (name));

    if (auto fd = mkstemp (name); fd != -1) {
      close (fd);
      unlink (name);
    }
#endif
    return name;
  }

  [[nodiscard]] int32_t hardware_concurrency () {
#if defined(YSTL_WINDOWS)
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);

    return static_cast<int32_t> (sysinfo.dwNumberOfProcessors);
#else
    return static_cast<int32_t> (sysconf (_SC_NPROCESSORS_ONLN));
#endif
  }

  [[nodiscard]] bool file_exists (const char *path) {
#if defined(YSTL_WINDOWS)
    return _access (path, 0) == 0;
#else
    return access (path, F_OK) == 0;
#endif
  }

  [[nodiscard]] FILE *open_stdio_file (const char *path, const char *mode) {
    FILE *handle = nullptr;

#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    fopen_s (&handle, path, mode);
#else
    handle = fopen (path, mode);
#endif
    return handle;
  }

  [[nodiscard]] int pid () noexcept {
#if defined(YSTL_WINDOWS)
    return _getpid ();
#else
    return getpid ();
#endif
  }
};

// expose platform singleton
YSTL_EXPOSE_GLOBAL_SINGLETON (Platform, plat);

}
