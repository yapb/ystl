// SPDX-License-Identifier: Unlicense

#pragma once

#include <assert.h>

#include <compare>

#include <ystl/movable.h>
#include <ystl/traits.h>

namespace ystl {

template <typename T, size_t N> struct FixedArray {
  T elems_[N];

  constexpr size_t size () const {
    return N;
  }

  constexpr size_t capacity () const {
    return N;
  }

  constexpr bool empty () const {
    return false;
  }

  template <typename U> const T &operator[] (U index) const {
    if constexpr (ystl::is_enum_v<U>) {
      return elems_[ystl::to_underlying (index)];
    }
    else {
      return elems_[index];
    }
  }

  template <typename U> T &operator[] (U index) {
    if constexpr (ystl::is_enum_v<U>) {
      return elems_[ystl::to_underlying (index)];
    }
    else {
      return elems_[index];
    }
  }

  template <typename U> const T &at (U index) const {
    size_t i = 0;

    if constexpr (ystl::is_enum_v<U>) {
      i = static_cast<size_t> (ystl::to_underlying (index));
    }
    else {
      i = static_cast<size_t> (index);
    }

    assert (i < N);
    return operator[] (index);
  }

  template <typename U> T &at (U index) {
    size_t i = 0;

    if constexpr (ystl::is_enum_v<U>) {
      i = static_cast<size_t> (ystl::to_underlying (index));
    }
    else {
      i = static_cast<size_t> (index);
    }

    assert (i < N);
    return operator[] (index);
  }

  constexpr T &front () {
    return elems_[0];
  }

  constexpr const T &front () const {
    return elems_[0];
  }

  constexpr T &back () {
    return elems_[N - 1];
  }

  constexpr const T &back () const {
    return elems_[N - 1];
  }

  constexpr T *data () {
    return elems_;
  }

  constexpr const T *data () const {
    return elems_;
  }

  constexpr T *begin () {
    return elems_;
  }

  constexpr const T *begin () const {
    return elems_;
  }

  constexpr const T *cbegin () const {
    return elems_;
  }

  constexpr T *end () {
    return elems_ + N;
  }

  constexpr const T *end () const {
    return elems_ + N;
  }

  constexpr const T *cend () const {
    return elems_ + N;
  }

  constexpr void fill (const T &value) {
    for (size_t i = 0; i < N; ++i) {
      elems_[i] = value;
    }
  }

  constexpr void swap (FixedArray &rhs) noexcept {
    for (size_t i = 0; i < N; ++i) {
      ystl::swap (elems_[i], rhs.elems_[i]);
    }
  }

  // deliberately not constexpr since defaulted operators may be ill-formed
  bool operator== (const FixedArray &) const = default;
  auto operator<=> (const FixedArray &) const = default;
};

// specialization for zero-length array
template <typename T> struct FixedArray<T, 0> {
  constexpr size_t size () const {
    return 0;
  }
  constexpr size_t capacity () const {
    return 0;
  }

  constexpr bool empty () const {
    return true;
  }

  constexpr T *data () {
    return nullptr;
  }

  constexpr const T *data () const {
    return nullptr;
  }

  constexpr T *begin () {
    return nullptr;
  }

  constexpr const T *begin () const {
    return nullptr;
  }

  constexpr const T *cbegin () const {
    return nullptr;
  }

  constexpr T *end () {
    return nullptr;
  }

  constexpr const T *end () const {
    return nullptr;
  }

  constexpr const T *cend () const {
    return nullptr;
  }

  constexpr void fill (const T &) {}

  constexpr void swap (FixedArray &) noexcept {}

  // note: not constexpr, since member types may provide runtime-only comparison
  bool operator== (const FixedArray &) const = default;
  auto operator<=> (const FixedArray &) const = default;
};

}
