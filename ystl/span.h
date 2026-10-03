// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdint.h>

#include <ystl/platform.h>
#include <ystl/traits.h>
#include <ystl/utility.h>

namespace ystl {

// lightweight non-owning view over a contiguous sequence of objects (std::span analogue)
template <typename T> class Span final {
private:
  T *ptr_ = nullptr;
  size_t len_ = 0;

public:
  constexpr Span () noexcept = default;

  constexpr Span (T *ptr, const size_t len) noexcept : ptr_ (ptr), len_ (len) {}

  // fixed-size arrays
  template <size_t N> constexpr Span (T (&arr)[N]) noexcept : ptr_ (arr), len_ (N) {}

  // enabled only when container data assigns to t pointer
  template <typename U>
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, Span> && ystl::is_lvalue_reference_v<U> &&
               requires (U &&cont, T *dst, size_t len) {
                 dst = cont.data ();
                 len = cont.size ();
               })
  constexpr Span (U &&cont) noexcept : ptr_ (cont.data ()), len_ (cont.size ()) {}

  // const-propagation conversion (span<t> -> span<const t>), same as std::span
  template <typename U>
    requires (ystl::is_same_v<T, const U> && !ystl::is_same_v<U, T>)
  constexpr Span (const Span<U> &other) noexcept : ptr_ (other.data ()), len_ (other.size ()) {}

public:
  constexpr T *data () const noexcept {
    return ptr_;
  }

  constexpr size_t size () const noexcept {
    return len_;
  }

  constexpr bool empty () const noexcept {
    return len_ == 0;
  }

  constexpr size_t length_bytes () const noexcept {
    size_t out = 0;

    if (mul_overflow (len_, sizeof (T), out)) [[unlikely]] {
      plat.abort ("Span::length_bytes() size overflows");
    }
    return out;
  }

  [[nodiscard]] constexpr size_t size_bytes () const noexcept {
    return length_bytes ();
  }

  constexpr T &operator[] (const size_t index) const noexcept {
    return ptr_[index];
  }

  constexpr T &at (const size_t index) const noexcept {
    if (index >= len_) [[unlikely]] {
      plat.abort ("Span::at() index out of range");
    }
    return ptr_[index];
  }

  constexpr T &front () const noexcept {
    if (len_ == 0) [[unlikely]] {
      plat.abort ("Span::front() called on empty span");
    }
    return *ptr_;
  }

  constexpr T &back () const noexcept {
    if (len_ == 0) [[unlikely]] {
      plat.abort ("Span::back() called on empty span");
    }
    return ptr_[len_ - 1];
  }

  constexpr Span first (const size_t count) const noexcept {
    return { ptr_, count < len_ ? count : len_ };
  }

  constexpr Span last (const size_t count) const noexcept {
    return count >= len_ ? *this : Span { ptr_ + (len_ - count), count };
  }

  constexpr Span subspan (const size_t offset) const noexcept {
    return offset >= len_ ? Span {} : Span { ptr_ + offset, len_ - offset };
  }

  constexpr Span subspan (const size_t offset, const size_t count) const noexcept {
    if (offset >= len_) {
      return {};
    }
    return { ptr_ + offset, count < len_ - offset ? count : len_ - offset };
  }

  // reinterprets the span as a read-only byte view
  constexpr Span<const uint8_t> bytes () const noexcept {
    return { reinterpret_cast<const uint8_t *> (ptr_), len_ * sizeof (T) };
  }

  // for range-based loops
public:
  constexpr T *begin () const noexcept {
    return ptr_;
  }

  constexpr T *end () const noexcept {
    return ptr_ + len_;
  }
};

template <typename T, size_t N> Span (T (&arr)[N]) -> Span<T>;

// trait to detect span instantiations (used to disambiguate generic overloads)
template <typename T> inline constexpr bool is_span_v = false;

template <typename U> inline constexpr bool is_span_v<Span<U>> = true;

// ctad for containers; element type follows the const-ness of data()
// mutable containers deduce mutable spans, const containers const spans
template <typename U>
  requires (!ystl::is_const_v<ystl::remove_reference_t<U>> &&
            requires (U &cont) {
              cont.data ();
              cont.size ();
            })
Span (U &cont) -> Span<ystl::remove_pointer_t<decltype (ystl::declval<U &> ().data ())>>;

template <typename U>
  requires requires (const U &cont) {
    cont.data ();
    cont.size ();
  }
Span (const U &cont) -> Span<ystl::remove_pointer_t<decltype (ystl::declval<const U &> ().data ())>>;

}
