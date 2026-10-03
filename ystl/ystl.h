// SPDX-License-Identifier: Unlicense

#pragma once

// ystl relies on c++20 language features (concepts, designated initializers)
#if defined(_MSVC_LANG)
  #define YSTL_CXX_STD _MSVC_LANG
#else
  #define YSTL_CXX_STD __cplusplus
#endif

#if YSTL_CXX_STD < 202002L
  // note gcc 10 reports old cplusplus, detect real mode via cpp constexpr check
  #if !(defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 10 && defined(__cpp_constexpr) && __cpp_constexpr >= 201907L)
    #error "ystl requires c++20 or newer. set cpp_std=c++20 in meson.build or CMAKE_CXX_STANDARD 20 in cmake."
  #endif
#endif

#include <stddef.h>
#include <stdio.h>
#include <limits.h>

// provide placement new to avoid the new header when it is unavailable
#if defined(YSTL_COMPAT_STL)
  #include <new>
#elif defined(__has_include) && __has_include(<new>)
  #include <new>
#elif !defined(__PLACEMENT_NEW_INLINE)
  #define __PLACEMENT_NEW_INLINE
inline void *operator new (const size_t, void *ptr) noexcept {
  return ptr;
}
#endif

#include <ystl/traits.h>
#include <ystl/utility.h>
#include <ystl/atomic.h>
#include <ystl/endian.h>
#include <ystl/memory.h>
#include <ystl/array.h>
#include <ystl/fixedarray.h>
#include <ystl/span.h>
#include <ystl/optional.h>
#include <ystl/tuple.h>
#include <ystl/deque.h>
#include <ystl/sort.h>
#include <ystl/flags.h>
#include <ystl/binheap.h>
#include <ystl/inlinelist.h>
#include <ystl/files.h>
#include <ystl/lambda.h>
#include <ystl/http.h>
#include <ystl/library.h>
#include <ystl/hashmap.h>
#include <ystl/hashset.h>
#include <ystl/optparse.h>
#include <ystl/tokenizer.h>
#include <ystl/confparse.h>
#include <ystl/logger.h>
#include <ystl/cpuflags.h>
#include <ystl/mathlib.h>
#include <ystl/vector.h>
#include <ystl/random.h>
#include <ystl/ulz.h>
#include <ystl/color.h>
#include <ystl/detour.h>
#include <ystl/thread.h>
#include <ystl/timers.h>
#include <ystl/wavehelper.h>
#include <ystl/utf8tools.h>
