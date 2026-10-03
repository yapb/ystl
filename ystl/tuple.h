// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/traits.h>

namespace ystl {

// forward declaration, so tuple_size is usable inside detail helpers
template <typename... Ts> class Tuple;

template <typename T> struct tuple_size;
template <typename... Ts> struct tuple_size<Tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};
template <typename T> struct tuple_size<const T> : tuple_size<T> {};
template <typename T> struct tuple_size<volatile T> : tuple_size<T> {};
template <typename T> struct tuple_size<const volatile T> : tuple_size<T> {};
template <typename T> inline constexpr size_t tuple_size_v = tuple_size<T>::value;

namespace detail {

// one indexed leaf per element; tuple inherits all of them, no recursion
template <size_t I, typename T> struct TupleLeaf {
  T value_;

  TupleLeaf () = default;

  template <typename U>
    requires (ystl::is_constructible_v<T, U &&>)
  constexpr TupleLeaf (U &&u) : value_ (ystl::forward<U> (u)) {}

  TupleLeaf (const TupleLeaf &) = default;
  TupleLeaf (TupleLeaf &&) = default;
  TupleLeaf &operator= (const TupleLeaf &) = default;
  TupleLeaf &operator= (TupleLeaf &&) = default;

  template <size_t J, typename U>
    requires (ystl::is_constructible_v<T, const U &>)
  constexpr TupleLeaf (const TupleLeaf<J, U> &rhs) : value_ (rhs.value_) {}

  template <size_t J, typename U>
    requires (ystl::is_constructible_v<T, U &&>)
  constexpr TupleLeaf (TupleLeaf<J, U> &&rhs) : value_ (ystl::move (rhs.value_)) {}
};

// element access through the matching leaf
template <size_t I, typename T> [[nodiscard]] YSTL_FORCE_INLINE constexpr T &leaf_get (TupleLeaf<I, T> &leaf) noexcept {
  return leaf.value_;
}

template <size_t I, typename T> [[nodiscard]] YSTL_FORCE_INLINE constexpr const T &leaf_get (const TupleLeaf<I, T> &leaf) noexcept {
  return leaf.value_;
}

template <size_t I, typename T> [[nodiscard]] YSTL_FORCE_INLINE constexpr T &&leaf_get (TupleLeaf<I, T> &&leaf) noexcept {
  return ystl::move (leaf.value_);
}

template <size_t I, typename T> [[nodiscard]] YSTL_FORCE_INLINE constexpr const T &&leaf_get (const TupleLeaf<I, T> &&leaf) noexcept {
  return ystl::move (leaf.value_);
}

// flat storage over all elements
template <typename Seq, typename... Ts> struct TupleBase;

template <size_t... Is, typename... Ts> struct TupleBase<index_sequence<Is...>, Ts...> : TupleLeaf<Is, Ts>... {
  TupleBase () = default;

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && sizeof...(Us) != 0 && (ystl::is_constructible_v<Ts, Us &&> && ...))
  constexpr TupleBase (Us &&...us) : TupleLeaf<Is, Ts> (ystl::forward<Us> (us))... {}

  TupleBase (const TupleBase &) = default;
  TupleBase (TupleBase &&) = default;
  TupleBase &operator= (const TupleBase &) = default;
  TupleBase &operator= (TupleBase &&) = default;

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && (ystl::is_constructible_v<Ts, const Us &> && ...))
  constexpr TupleBase (const TupleBase<make_index_sequence<sizeof...(Us)>, Us...> &rhs) : TupleLeaf<Is, Ts> (leaf_get<Is> (rhs))... {}

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && (ystl::is_constructible_v<Ts, Us &&> && ...))
  constexpr TupleBase (TupleBase<make_index_sequence<sizeof...(Us)>, Us...> &&rhs) : TupleLeaf<Is, Ts> (ystl::move (leaf_get<Is> (rhs)))... {}
};

// number of occurrences of t in ts... (for get<t>)
template <typename T, typename... Ts> struct TypeCount;
template <typename T> struct TypeCount<T> : integral_constant<size_t, 0> {};
template <typename T, typename H, typename... Rs>
struct TypeCount<T, H, Rs...> : integral_constant<size_t, (ystl::is_same_v<T, H> ? 1 : 0) + TypeCount<T, Rs...>::value> {};

// equality as a single fold; an empty pack yields true
template <typename A, typename B, size_t... Is> [[nodiscard]] constexpr bool tuple_equal (const A &a, const B &b, index_sequence<Is...>) {
  return ((leaf_get<Is> (a) == leaf_get<Is> (b)) && ...);
}

// lexicographic less, the first differing element decides
template <size_t I, size_t N, typename A, typename B> [[nodiscard]] constexpr bool tuple_less (const A &a, const B &b) {
  if constexpr (I >= N) {
    return false;
  }
  else {
    if (leaf_get<I> (a) < leaf_get<I> (b)) {
      return true;
    }
    if (leaf_get<I> (b) < leaf_get<I> (a)) {
      return false;
    }
    return tuple_less<I + 1, N> (a, b);
  }
}
}

// heterogeneous container of ts
template <typename... Ts> class Tuple : public detail::TupleBase<make_index_sequence<sizeof...(Ts)>, Ts...> {
private:
  using Base = detail::TupleBase<make_index_sequence<sizeof...(Ts)>, Ts...>;

public:
  constexpr Tuple ()
    requires ((ystl::is_constructible_v<Ts> && ...))
    : Base () {}

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && sizeof...(Us) != 0 && (ystl::is_constructible_v<Ts, Us &&> && ...))
  constexpr Tuple (Us &&...us) : Base (ystl::forward<Us> (us)...) {}

  Tuple (const Tuple &) = default;
  Tuple (Tuple &&) = default;

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && (ystl::is_constructible_v<Ts, const Us &> && ...))
  constexpr Tuple (const Tuple<Us...> &rhs) : Base (static_cast<const detail::TupleBase<make_index_sequence<sizeof...(Us)>, Us...> &> (rhs)) {}

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts) && (ystl::is_constructible_v<Ts, Us &&> && ...))
  constexpr Tuple (Tuple<Us...> &&rhs) : Base (static_cast<detail::TupleBase<make_index_sequence<sizeof...(Us)>, Us...> &&> (rhs)) {}

  Tuple &operator= (const Tuple &) = default;
  Tuple &operator= (Tuple &&) = default;

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  Tuple &operator= (const Tuple<Us...> &rhs) {
    assign_impl (rhs, make_index_sequence<sizeof...(Ts)> {});
    return *this;
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  Tuple &operator= (Tuple<Us...> &&rhs) {
    assign_impl (ystl::move (rhs), make_index_sequence<sizeof...(Ts)> {});
    return *this;
  }

public:
  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator== (const Tuple<Us...> &rhs) const {
    return detail::tuple_equal (*this, rhs, make_index_sequence<sizeof...(Ts)> {});
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator!= (const Tuple<Us...> &rhs) const {
    return !(*this == rhs);
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator< (const Tuple<Us...> &rhs) const {
    return detail::tuple_less<0, sizeof...(Ts)> (*this, rhs);
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator> (const Tuple<Us...> &rhs) const {
    return rhs < *this;
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator<= (const Tuple<Us...> &rhs) const {
    return !(rhs < *this);
  }

  template <typename... Us>
    requires (sizeof...(Us) == sizeof...(Ts))
  [[nodiscard]] bool operator>= (const Tuple<Us...> &rhs) const {
    return !(*this < rhs);
  }

public:
  void swap (Tuple &rhs) noexcept {
    if (this == &rhs) [[unlikely]] {
      return;
    }
    swap_impl (rhs, make_index_sequence<sizeof...(Ts)> {});
  }

private:
  template <typename U, size_t... Is> void assign_impl (U &&rhs, index_sequence<Is...>) {
    ((detail::leaf_get<Is> (*this) = detail::leaf_get<Is> (ystl::forward<U> (rhs))), ...);
  }

  template <size_t... Is> void swap_impl (Tuple &rhs, index_sequence<Is...>) noexcept {
    (ystl::swap (detail::leaf_get<Is> (*this), detail::leaf_get<Is> (rhs)), ...);
  }
};

template <typename... Us> Tuple (Us &&...) -> Tuple<typename decay<Us>::type...>;

// get by index
template <size_t I, typename... Ts>
[[nodiscard]] YSTL_FORCE_INLINE constexpr typename detail::TypeAt<I, Ts...>::type &get (Tuple<Ts...> &t) noexcept {
  static_assert (I < sizeof...(Ts), "ystl::get<I>: index out of range");
  return detail::leaf_get<I> (t);
}

template <size_t I, typename... Ts>
[[nodiscard]] YSTL_FORCE_INLINE constexpr const typename detail::TypeAt<I, Ts...>::type &get (const Tuple<Ts...> &t) noexcept {
  static_assert (I < sizeof...(Ts), "ystl::get<I>: index out of range");
  return detail::leaf_get<I> (t);
}

template <size_t I, typename... Ts>
[[nodiscard]] YSTL_FORCE_INLINE constexpr typename detail::TypeAt<I, Ts...>::type &&get (Tuple<Ts...> &&t) noexcept {
  static_assert (I < sizeof...(Ts), "ystl::get<I>: index out of range");
  return detail::leaf_get<I> (ystl::move (t));
}

template <size_t I, typename... Ts>
[[nodiscard]] YSTL_FORCE_INLINE constexpr const typename detail::TypeAt<I, Ts...>::type &&get (const Tuple<Ts...> &&t) noexcept {
  static_assert (I < sizeof...(Ts), "ystl::get<I>: index out of range");
  return detail::leaf_get<I> (ystl::move (t));
}

// get by type, enabled only when t appears exactly once
template <typename T, typename... Ts> [[nodiscard]] YSTL_FORCE_INLINE constexpr T &get (Tuple<Ts...> &t) noexcept {
  static_assert (detail::TypeCount<T, Ts...>::value == 1, "ystl::get<T>: T must appear exactly once in Tuple");
  return get<detail::IndexOf<0, T, Ts...>::value> (t);
}

template <typename T, typename... Ts> [[nodiscard]] YSTL_FORCE_INLINE constexpr const T &get (const Tuple<Ts...> &t) noexcept {
  static_assert (detail::TypeCount<T, Ts...>::value == 1, "ystl::get<T>: T must appear exactly once in Tuple");
  return get<detail::IndexOf<0, T, Ts...>::value> (t);
}

template <typename T, typename... Ts> [[nodiscard]] YSTL_FORCE_INLINE constexpr T &&get (Tuple<Ts...> &&t) noexcept {
  static_assert (detail::TypeCount<T, Ts...>::value == 1, "ystl::get<T>: T must appear exactly once in Tuple");
  return get<detail::IndexOf<0, T, Ts...>::value> (ystl::move (t));
}

template <typename T, typename... Ts> [[nodiscard]] YSTL_FORCE_INLINE constexpr const T &&get (const Tuple<Ts...> &&t) noexcept {
  static_assert (detail::TypeCount<T, Ts...>::value == 1, "ystl::get<T>: T must appear exactly once in Tuple");
  return get<detail::IndexOf<0, T, Ts...>::value> (ystl::move (t));
}

// maketuple decays its arguments
template <typename... Ts> [[nodiscard]] constexpr Tuple<typename decay<Ts>::type...> make_tuple (Ts &&...args) {
  return Tuple<typename decay<Ts>::type...> (ystl::forward<Ts> (args)...);
}

// tie packs lvalue references, forwardastuple packs forwarding references
template <typename... Ts> [[nodiscard]] constexpr Tuple<Ts &...> tie (Ts &...args) {
  return Tuple<Ts &...> (args...);
}

template <typename... Ts> [[nodiscard]] constexpr Tuple<Ts &&...> forward_as_tuple (Ts &&...args) noexcept {
  return Tuple<Ts &&...> (ystl::forward<Ts> (args)...);
}

// tuple_element
template <size_t I, typename T> struct tuple_element;
template <size_t I, typename... Ts> struct tuple_element<I, Tuple<Ts...>> {
  using type = typename detail::TypeAt<I, Ts...>::type;
};
template <size_t I, typename T> struct tuple_element<I, const T> : tuple_element<I, T> {};
template <size_t I, typename T> struct tuple_element<I, volatile T> : tuple_element<I, T> {};
template <size_t I, typename T> struct tuple_element<I, const volatile T> : tuple_element<I, T> {};
template <size_t I, typename T> using tuple_element_t = typename tuple_element<I, T>::type;

namespace detail {

// concatenate two tuples, preserving value categories of the sources
template <typename A, typename B, size_t... Is, size_t... Js>
[[nodiscard]] constexpr auto tuple_cat_two (A &&a, B &&b, index_sequence<Is...>, index_sequence<Js...>) {
  using Result = Tuple<typename tuple_element<Is, remove_cvref_t<A>>::type..., typename tuple_element<Js, remove_cvref_t<B>>::type...>;

  return Result (get<Is> (ystl::forward<A> (a))..., get<Js> (ystl::forward<B> (b))...);
}
}

// tuplecat concatenates any number of tuples
[[nodiscard]] inline constexpr Tuple<> tuple_cat () {
  return Tuple<> {};
}

template <typename A, typename B> [[nodiscard]] constexpr auto tuple_cat (A &&a, B &&b) {
  return detail::tuple_cat_two (ystl::forward<A> (a), ystl::forward<B> (b), make_index_sequence<tuple_size_v<remove_cvref_t<A>>> {},
    make_index_sequence<tuple_size_v<remove_cvref_t<B>>> {});
}

template <typename A, typename B, typename... Rs> [[nodiscard]] constexpr auto tuple_cat (A &&a, B &&b, Rs &&...rest) {
  return tuple_cat (tuple_cat (ystl::forward<A> (a), ystl::forward<B> (b)), ystl::forward<Rs> (rest)...);
}

// swap
template <typename... Ts> constexpr void swap (Tuple<Ts...> &lhs, Tuple<Ts...> &rhs) noexcept {
  lhs.swap (rhs);
}

}
