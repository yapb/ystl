// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/traits.h>
#include <ystl/movable.h>

namespace ystl {

template <typename T> struct Less {
  constexpr bool operator() (const T &a, const T &b) const {
    return a < b;
  }
};

template <typename T> struct Greater {
  constexpr bool operator() (const T &a, const T &b) const {
    return a > b;
  }
};

namespace detail {

// memset fast-path for fill: valid only when every byte of `value` is identical
template <typename T> YSTL_FORCE_INLINE bool uniform_byte_fill (T *first, size_t count, const T &value) {
  const auto bytes = reinterpret_cast<const uint8_t *> (&value);

  for (size_t i = 1; i < sizeof (T); ++i) {
    if (bytes[i] != bytes[0]) {
      return false;
    }
  }
  memset (reinterpret_cast<uint8_t *> (first), bytes[0], count * sizeof (T));

  return true;
}

// range helpers unify containers and c arrays for range overloads
template <typename Container> YSTL_FORCE_INLINE auto range_begin (Container &container) {
  return container.begin ();
}

template <typename Container> YSTL_FORCE_INLINE auto range_end (Container &container) {
  return container.end ();
}

template <typename T, size_t N> YSTL_FORCE_INLINE T *range_begin (T (&array)[N]) {
  return array;
}

template <typename T, size_t N> YSTL_FORCE_INLINE T *range_end (T (&array)[N]) {
  return array + N;
}

template <typename T, size_t N> YSTL_FORCE_INLINE const T *range_begin (const T (&array)[N]) {
  return array;
}

template <typename T, size_t N> YSTL_FORCE_INLINE const T *range_end (const T (&array)[N]) {
  return array + N;
}

// number of elements in a container or c array
template <typename Container> YSTL_FORCE_INLINE size_t range_count (Container &container) {
  return static_cast<size_t> (range_end (container) - range_begin (container));
}
}

// fill assigns value to each element, using memset for uniform bytes
template <typename T, typename U> YSTL_FORCE_INLINE void fill (T *first, size_t count, const U &value) {
  if (!first || count == 0) [[unlikely]] {
    return;
  }
  if constexpr (ystl::is_trivially_copyable_v<T>) {
    const T converted = static_cast<T> (value);

    // memset handles uniform byte patterns; otherwise assign element-wise
    if (!detail::uniform_byte_fill (first, count, converted)) {
      for (size_t i = 0; i < count; ++i) {
        first[i] = converted;
      }
    }
  }
  else {
    for (size_t i = 0; i < count; ++i) {
      first[i] = value;
    }
  }
}

// fill for containers and c arrays
template <typename Container, typename U> YSTL_FORCE_INLINE void fill (Container &container, const U &value) {
  fill (detail::range_begin (container), detail::range_count (container), value);
}

// filln: alias to fill, kept for call-site readability
template <typename T, typename U> YSTL_FORCE_INLINE void fill_n (T *first, size_t count, const U &value) {
  fill (first, count, value);
}

// copy copies count elements from src to dest, safe for overlapping ranges
template <typename T> YSTL_FORCE_INLINE T *copy (T *dest, const T *src, size_t count) {
  if (!dest || !src || count == 0) [[unlikely]] {
    return dest;
  }
  if constexpr (ystl::is_trivially_copyable_v<T>) {
    memmove (dest, src, count * sizeof (T));
  }
  else {
    for (size_t i = 0; i < count; ++i) {
      dest[i] = src[i];
    }
  }
  return dest + count;
}

// copyn: alias to copy, kept for call-site readability
template <typename T> YSTL_FORCE_INLINE T *copy_n (T *dest, const T *src, size_t count) {
  return copy (dest, src, count);
}

// foreach: applies `fn` to each element, returns the functor
template <typename T, typename UnaryFn> YSTL_FORCE_INLINE UnaryFn for_each (T *first, size_t num, UnaryFn fn) {
  for (size_t i = 0; i < num; ++i) {
    fn (first[i]);
  }
  return fn;
}

template <typename Container, typename UnaryFn> YSTL_FORCE_INLINE UnaryFn for_each (Container &container, UnaryFn fn) {
  return for_each (detail::range_begin (container), detail::range_count (container), fn);
}

// find: returns pointer to the first element equal to `value`, or nullptr
template <typename T, typename U> YSTL_FORCE_INLINE T *find (T *first, size_t num, const U &value) {
  for (size_t i = 0; i < num; ++i) {
    if (first[i] == value) {
      return &first[i];
    }
  }
  return nullptr;
}

template <typename T, typename U> YSTL_FORCE_INLINE const T *find (const T *first, size_t num, const U &value) {
  for (size_t i = 0; i < num; ++i) {
    if (first[i] == value) {
      return &first[i];
    }
  }
  return nullptr;
}

template <typename Container, typename U> YSTL_FORCE_INLINE auto find (Container &container, const U &value) {
  return find (detail::range_begin (container), detail::range_count (container), value);
}

// findif: returns pointer to the first element matching `pred`, or nullptr
template <typename T, typename Predicate> YSTL_FORCE_INLINE T *find_if (T *first, size_t num, Predicate pred) {
  for (size_t i = 0; i < num; ++i) {
    if (pred (first[i])) {
      return &first[i];
    }
  }
  return nullptr;
}

template <typename T, typename Predicate> YSTL_FORCE_INLINE const T *find_if (const T *first, size_t num, Predicate pred) {
  for (size_t i = 0; i < num; ++i) {
    if (pred (first[i])) {
      return &first[i];
    }
  }
  return nullptr;
}

template <typename Container, typename Predicate> YSTL_FORCE_INLINE auto find_if (Container &container, Predicate pred) {
  return find_if (detail::range_begin (container), detail::range_count (container), pred);
}

// count: returns the number of elements equal to `value`
template <typename T, typename U> YSTL_FORCE_INLINE size_t count (const T *first, size_t num, const U &value) {
  size_t result = 0;

  for (size_t i = 0; i < num; ++i) {
    if (first[i] == value) {
      ++result;
    }
  }
  return result;
}

template <typename Container, typename U> YSTL_FORCE_INLINE size_t count (Container &container, const U &value) {
  return count (detail::range_begin (container), detail::range_count (container), value);
}

// countif: returns the number of elements matching `pred`
template <typename T, typename Predicate> YSTL_FORCE_INLINE size_t count_if (const T *first, size_t num, Predicate pred) {
  size_t result = 0;

  for (size_t i = 0; i < num; ++i) {
    if (pred (first[i])) {
      ++result;
    }
  }
  return result;
}

template <typename Container, typename Predicate> YSTL_FORCE_INLINE size_t count_if (Container &container, Predicate pred) {
  return count_if (detail::range_begin (container), detail::range_count (container), pred);
}

// reverse: reverses the order of `num` elements in place
template <typename T> YSTL_FORCE_INLINE void reverse (T *first, size_t num) {
  if (!first || num < 2) [[unlikely]] {
    return;
  }
  size_t i = 0;
  size_t j = num - 1;

  while (i < j) {
    ystl::swap (first[i], first[j]);
    ++i;
    --j;
  }
}

template <typename Container> YSTL_FORCE_INLINE void reverse (Container &container) {
  reverse (detail::range_begin (container), detail::range_count (container));
}

// transform: assigns op (src[i]) to dest[i], dest may alias src
template <typename T, typename UnaryOp> YSTL_FORCE_INLINE void transform (T *dest, const T *src, size_t num, UnaryOp op) {
  for (size_t i = 0; i < num; ++i) {
    dest[i] = op (src[i]);
  }
}

// transform: assigns op (first[i], second[i]) to dest[i], dest may alias either input
template <typename T, typename BinaryOp> YSTL_FORCE_INLINE void transform (T *dest, const T *first, const T *second, size_t num, BinaryOp op) {
  for (size_t i = 0; i < num; ++i) {
    dest[i] = op (first[i], second[i]);
  }
}

// minelement: returns pointer to the smallest element, or nullptr for an empty range
template <typename T, typename Compare> YSTL_FORCE_INLINE T *min_element (T *first, size_t num, Compare comp) {
  if (num == 0) [[unlikely]] {
    return nullptr;
  }
  T *best = first;

  for (size_t i = 1; i < num; ++i) {
    if (comp (first[i], *best)) {
      best = &first[i];
    }
  }
  return best;
}

template <typename T> YSTL_FORCE_INLINE T *min_element (T *first, size_t num) {
  return min_element (first, num, Less<T> {});
}

template <typename T, typename Compare> YSTL_FORCE_INLINE const T *min_element (const T *first, size_t num, Compare comp) {
  if (num == 0) [[unlikely]] {
    return nullptr;
  }
  const T *best = first;

  for (size_t i = 1; i < num; ++i) {
    if (comp (first[i], *best)) {
      best = &first[i];
    }
  }
  return best;
}

template <typename T> YSTL_FORCE_INLINE const T *min_element (const T *first, size_t num) {
  return min_element (first, num, Less<T> {});
}

template <typename Container> YSTL_FORCE_INLINE auto min_element (Container &container) {
  using T = remove_reference_t<decltype (*detail::range_begin (container))>;
  return min_element (detail::range_begin (container), detail::range_count (container), Less<T> {});
}

// maxelement: returns pointer to the largest element, or nullptr for an empty range
template <typename T, typename Compare> YSTL_FORCE_INLINE T *max_element (T *first, size_t num, Compare comp) {
  if (num == 0) [[unlikely]] {
    return nullptr;
  }
  T *best = first;

  for (size_t i = 1; i < num; ++i) {
    if (comp (*best, first[i])) {
      best = &first[i];
    }
  }
  return best;
}

template <typename T> YSTL_FORCE_INLINE T *max_element (T *first, size_t num) {
  return max_element (first, num, Less<T> {});
}

template <typename T, typename Compare> YSTL_FORCE_INLINE const T *max_element (const T *first, size_t num, Compare comp) {
  if (num == 0) [[unlikely]] {
    return nullptr;
  }
  const T *best = first;

  for (size_t i = 1; i < num; ++i) {
    if (comp (*best, first[i])) {
      best = &first[i];
    }
  }
  return best;
}

template <typename T> YSTL_FORCE_INLINE const T *max_element (const T *first, size_t num) {
  return max_element (first, num, Less<T> {});
}

template <typename Container> YSTL_FORCE_INLINE auto max_element (Container &container) {
  using T = remove_reference_t<decltype (*detail::range_begin (container))>;
  return max_element (detail::range_begin (container), detail::range_count (container), Less<T> {});
}

// lowerbound returns first element not ordered before value, range must be sorted
template <typename T, typename U, typename Compare> YSTL_FORCE_INLINE T *lower_bound (T *first, size_t num, const U &value, Compare comp) {
  size_t base = 0;

  while (num > 0) {
    const size_t half = num / 2;

    if (comp (first[base + half], value)) {
      base += half + 1;
      num -= half + 1;
    }
    else {
      num = half;
    }
  }
  return first + base;
}

template <typename T, typename U> YSTL_FORCE_INLINE T *lower_bound (T *first, size_t num, const U &value) {
  return lower_bound (first, num, value, Less<T> {});
}

template <typename T, typename U, typename Compare>
YSTL_FORCE_INLINE const T *lower_bound (const T *first, size_t num, const U &value, Compare comp) {
  size_t base = 0;

  while (num > 0) {
    const size_t half = num / 2;

    if (comp (first[base + half], value)) {
      base += half + 1;
      num -= half + 1;
    }
    else {
      num = half;
    }
  }
  return first + base;
}

template <typename T, typename U> YSTL_FORCE_INLINE const T *lower_bound (const T *first, size_t num, const U &value) {
  return lower_bound (first, num, value, Less<T> {});
}

template <typename Container, typename U> YSTL_FORCE_INLINE auto lower_bound (Container &container, const U &value) {
  return lower_bound (detail::range_begin (container), detail::range_count (container), value);
}

// upperbound returns first element ordered after value, range must be sorted
template <typename T, typename U, typename Compare> YSTL_FORCE_INLINE T *upper_bound (T *first, size_t num, const U &value, Compare comp) {
  size_t base = 0;

  while (num > 0) {
    const size_t half = num / 2;

    if (!comp (value, first[base + half])) {
      base += half + 1;
      num -= half + 1;
    }
    else {
      num = half;
    }
  }
  return first + base;
}

template <typename T, typename U> YSTL_FORCE_INLINE T *upper_bound (T *first, size_t num, const U &value) {
  return upper_bound (first, num, value, Less<T> {});
}

template <typename T, typename U, typename Compare>
YSTL_FORCE_INLINE const T *upper_bound (const T *first, size_t num, const U &value, Compare comp) {
  size_t base = 0;

  while (num > 0) {
    const size_t half = num / 2;

    if (!comp (value, first[base + half])) {
      base += half + 1;
      num -= half + 1;
    }
    else {
      num = half;
    }
  }
  return first + base;
}

template <typename T, typename U> YSTL_FORCE_INLINE const T *upper_bound (const T *first, size_t num, const U &value) {
  return upper_bound (first, num, value, Less<T> {});
}

template <typename Container, typename U> YSTL_FORCE_INLINE auto upper_bound (Container &container, const U &value) {
  return upper_bound (detail::range_begin (container), detail::range_count (container), value);
}

// equal is true when num elements compare equal, using memcmp for integers
template <typename T> YSTL_FORCE_INLINE bool equal (const T *first1, const T *first2, size_t num) {
  if (num == 0) [[unlikely]] {
    return true;
  }
  if constexpr (ystl::is_integral_v<T> || ystl::is_enum_v<T>) {
    return memcmp (first1, first2, num * sizeof (T)) == 0;
  }
  else {
    for (size_t i = 0; i < num; ++i) {
      if (!(first1[i] == first2[i])) {
        return false;
      }
    }
    return true;
  }
}

// equal for two containers or c arrays
template <typename ContainerL, typename ContainerR> YSTL_FORCE_INLINE bool equal (ContainerL &lhs, ContainerR &rhs) {
  const auto num = detail::range_count (lhs);

  if (num != detail::range_count (rhs)) {
    return false;
  }
  return equal (detail::range_begin (lhs), detail::range_begin (rhs), num);
}

}
