// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/traits.h>

#include <cstdint>
#include <string.h>

namespace ystl {
// byte-wise zero fill, direct analogue of memset (ptr, 0, bytes)
template <typename T> void memzero (T *ptr, size_t bytes) noexcept {
  memset (reinterpret_cast<void *> (ptr), 0, bytes);
}

template <typename T, size_t N> constexpr size_t bufsize (const T (&)[N]) {
  return N - 1;
}

template <typename T> constexpr T bit (const T &a) {
  if constexpr (is_enum_v<T>) {
    using U = underlying_type_t<T>;
    return static_cast<T> (static_cast<U> (1) << static_cast<U> (a));
  }
  else {
    return static_cast<T> (1ULL << a);
  }
}

constexpr size_t bit_ceil (size_t n) noexcept {
  if (n <= 1) {
    return 1;
  }
#if defined(__has_builtin)
  #if __has_builtin(__builtin_clzll)
  // width of n - 1 in bits is the shift of the next power of two above it
  const auto width = static_cast<size_t> (64 - __builtin_clzll (static_cast<unsigned long long> (n - 1)));

  if (width >= numeric_limits<size_t>::digits) [[unlikely]] {
    return numeric_limits<size_t>::max (); // true ceiling is unrepresentable, saturate like below
  }
  return static_cast<size_t> (1ULL << width);
  #endif
#endif
  // fallback for compilers without count-leading-zeros (older msvc)
  n--;

  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;

// note: SIZE_MAX stays here, the preprocessor cannot evaluate numeric_limits
#if (SIZE_MAX > 0xffffffffu)
  n |= n >> 32;
#endif

  if (n == numeric_limits<size_t>::max ()) {
    return n;
  }
  return n + 1;
}

// checked size_t arithmetic for allocation sizes, true on wraparound
[[nodiscard]] constexpr bool add_overflow (size_t a, size_t b, size_t &out) noexcept {
#if defined(__has_builtin)
  #if __has_builtin(__builtin_add_overflow)
  return __builtin_add_overflow (a, b, &out);
  #endif
#endif
  out = a + b;
  return out < a;
}

[[nodiscard]] constexpr bool mul_overflow (size_t a, size_t b, size_t &out) noexcept {
#if defined(__has_builtin)
  #if __has_builtin(__builtin_mul_overflow)
  return __builtin_mul_overflow (a, b, &out);
  #endif
#endif
  out = a * b;
  return a != 0 && out / a != b;
}

// bit reinterpretation without union type punning prefers the compiler builtin
template <typename To, typename From> constexpr To bit_cast (const From &value) noexcept {
#if defined(__has_builtin)
  #if __has_builtin(__builtin_bit_cast)
  return __builtin_bit_cast (To, value);
  #endif
#endif
  // fallback for compilers without the builtin (older msvc)
  union {
    From from;
    To to;
  } punning {};

  punning.from = value;
  return punning.to;
}

}
