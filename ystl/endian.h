// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <cstring>

#include <ystl/platform.h>
#include <ystl/span.h>
#include <ystl/traits.h>

namespace ystl {

// byteorder is a constexpr byte swapping utility for endian conversions
class ByteOrder final {
public:
  // detect system endianness at compile time using ystl platform macros
  static constexpr bool is_little_endian () {
#if defined(YSTL_ARCH_CPU_BIG_ENDIAN)
    return false;
#else
    return true;
#endif
  }

private:
  // manual byte swap implementations (fallback)
  static constexpr uint16_t swap_bytes16 (uint16_t value) noexcept {
    return static_cast<uint16_t> ((value >> 8) | (value << 8));
  }

  static constexpr uint32_t swap_bytes32 (uint32_t value) noexcept {
    return ((value & 0x000000ffu) << 24) | ((value & 0x0000ff00u) << 8) | ((value & 0x00ff0000u) >> 8) | ((value & 0xff000000u) >> 24);
  }

  static constexpr uint64_t swap_bytes64 (uint64_t value) noexcept {
    return ((value & 0x00000000000000ffull) << 56) | ((value & 0x000000000000ff00ull) << 40) | ((value & 0x0000000000ff0000ull) << 24) |
           ((value & 0x00000000ff000000ull) << 8) | ((value & 0x000000ff00000000ull) >> 8) | ((value & 0x0000ff0000000000ull) >> 24) |
           ((value & 0x00ff000000000000ull) >> 40) | ((value & 0xff00000000000000ull) >> 56);
  }

public:
  // compiler intrinsic wrappers for optimal performance
  static YSTL_FORCE_INLINE uint16_t bswap16 (uint16_t value) noexcept {
#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    return _byteswap_ushort (value);
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
    return __builtin_bswap16 (value);
#else
    return swap_bytes16 (value);
#endif
  }

  static YSTL_FORCE_INLINE uint32_t bswap32 (uint32_t value) noexcept {
#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    return _byteswap_ulong (value);
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
    return __builtin_bswap32 (value);
#else
    return swap_bytes32 (value);
#endif
  }

  static YSTL_FORCE_INLINE uint64_t bswap64 (uint64_t value) noexcept {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
    return __builtin_bswap64 (value);
#elif defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    return _byteswap_uint64 (value);
#else
    return swap_bytes64 (value);
#endif
  }

  // generic byte swap for any scalar, floats go through integer bit casts
  template <typename T> static constexpr YSTL_FORCE_INLINE T swap_scalar (T value) noexcept {
    if constexpr (is_same_v<T, float>) {
      return bit_cast<float> (bswap32 (bit_cast<uint32_t> (value)));
    }
    else if constexpr (is_same_v<T, double>) {
      return bit_cast<double> (bswap64 (bit_cast<uint64_t> (value)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (bswap16 (static_cast<uint16_t> (value)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (bswap32 (static_cast<uint32_t> (value)));
    }
    else if constexpr (sizeof (T) == 8) {
      return static_cast<T> (bswap64 (static_cast<uint64_t> (value)));
    }
    else {
      return value; // unsupported size
    }
  }

  // converts any-endian value to native byte order, noop on little endian
  template <typename T> static constexpr YSTL_FORCE_INLINE T to_native (T value) noexcept {
    if constexpr (is_little_endian ()) {
      return value;
    }
    else {
      return swap_scalar (value);
    }
  }

  // directional conversion functions
  template <typename T> static constexpr YSTL_FORCE_INLINE T from_le (T value) noexcept {
    // from le to native: on le it's noop, on be we swap
    return to_native (value);
  }

  template <typename T> static constexpr YSTL_FORCE_INLINE T to_le (T value) noexcept {
    // converts native to little endian, noop on little endian systems
    return to_native (value);
  }

  template <typename T> static constexpr YSTL_FORCE_INLINE T from_be (T value) noexcept {
    // from be to native: on be it's noop, on le we swap
    if constexpr (is_little_endian ()) {
      return swap_scalar (value);
    }
    else {
      return value;
    }
  }

  template <typename T> static constexpr YSTL_FORCE_INLINE T to_be (T value) noexcept {
    // to be from native: on be it's noop, on le we swap
    return from_be (value);
  }

  template <typename T> static YSTL_FORCE_INLINE T read_le (const void *ptr) noexcept {
    T value {};
    memcpy (&value, ptr, sizeof (T));

    return from_le<T> (value);
  }

  template <typename T> static YSTL_FORCE_INLINE T read_be (const void *ptr) noexcept {
    T value {};
    memcpy (&value, ptr, sizeof (T));
    return from_be<T> (value);
  }

  template <typename T> static YSTL_FORCE_INLINE void write_le (void *ptr, T value) noexcept {
    T converted = to_le<T> (value);
    memcpy (ptr, &converted, sizeof (T));
  }

  template <typename T> static YSTL_FORCE_INLINE void write_be (void *ptr, T value) noexcept {
    T converted = to_be<T> (value);
    memcpy (ptr, &converted, sizeof (T));
  }

  // 16-bit
  static YSTL_FORCE_INLINE uint16_t swap16 (uint16_t value) noexcept {
    if constexpr (is_little_endian ()) {
      return value;
    }
    else {
      return bswap16 (value);
    }
  }

  // 32-bit
  static YSTL_FORCE_INLINE uint32_t swap32 (uint32_t value) noexcept {
    if constexpr (is_little_endian ()) {
      return value;
    }
    else {
      return bswap32 (value);
    }
  }

  // 64-bit
  static YSTL_FORCE_INLINE uint64_t swap64 (uint64_t value) noexcept {
    if constexpr (is_little_endian ()) {
      return value;
    }
    else {
      return bswap64 (value);
    }
  }
};

// leio - little-endian field walker for on-disk serializable structures
namespace leio {

// field descriptor dispatches to adl-found each_fields overloads
template <typename T> struct Fields {
  template <typename V, typename F> static constexpr void each (V &object, F &&visit) {
    each_fields (object, visit);
  }
};

namespace detail {
// constexpr visitor summing sizeof() of every declared field
template <typename T> struct CoverCounter {
  size_t total {};

  template <typename M> constexpr void operator() (M &&) {
    total += sizeof (M);
  }
};

template <typename F, typename... Ms> constexpr void visit_each (F &&visit, Ms &&...ms) {
  (visit (ms), ...);
}
}

// swaps multi-byte fields of value in place to or from little endian
template <typename T>
  requires (!ystl::is_span_v<T>)
void le_swap (T &value) {
  if constexpr (ystl::is_integral_v<T> || ystl::is_enum_v<T>) {
    if constexpr (sizeof (T) > 1) {
      if constexpr (ystl::is_enum_v<T>) {
        value = static_cast<T> (ystl::ByteOrder::from_le (ystl::to_underlying (value)));
      }
      else {
        value = ystl::ByteOrder::from_le (value);
      }
    }
  }
  else if constexpr (ystl::is_floating_point_v<T>) {
    value = ystl::ByteOrder::from_le (value);
  }
  else if constexpr (ystl::is_array_v<T>) {
    for (auto &element : value) {
      le_swap (element);
    }
  }
  else {
    Fields<T>::each (value, [&] (auto &member) {
      le_swap (member);
    });
  }
}

// bulk overload: walks count elements of an array
template <typename U> void le_swap (U *data, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    le_swap (data[i]);
  }
}

// bulk overload: walks the elements of a span
template <typename U> void le_swap (const Span<U> &data) {
  le_swap (data.data (), data.size ());
}

// true when the fields declared via YSTL_LE_FIELDS cover sizeof(T) fully
template <typename T> constexpr bool fully_covered () {
  T value {};
  detail::CoverCounter<T> counter {};
  Fields<T>::each (value, counter);

  return counter.total == sizeof (T);
}
}

}

// declares adl-found each_fields overloads for type and verifies full coverage
#define YSTL_LE_FIELDS(Type, ...)                                                  \
  constexpr void each_fields (Type &object, auto &&visit) {                        \
    auto &&[__VA_ARGS__] = object;                                                 \
    ystl::leio::detail::visit_each (visit, __VA_ARGS__);                           \
  }                                                                                \
  constexpr void each_fields (const Type &object, auto &&visit) {                  \
    auto &&[__VA_ARGS__] = object;                                                 \
    ystl::leio::detail::visit_each (visit, __VA_ARGS__);                           \
  }                                                                                \
  static_assert (ystl::leio::fully_covered<Type> (), #Type " must be fully covered by YSTL_LE_FIELDS")
