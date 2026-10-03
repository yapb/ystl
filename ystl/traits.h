// SPDX-License-Identifier: Unlicense

#pragma once

// freestanding STL traits; ystl names are thin aliases over std
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

namespace ystl {

using std::add_lvalue_reference;
using std::add_rvalue_reference;
using std::bool_constant;
using std::conditional;
using std::conditional_t;
using std::decay;
using std::declval;
using std::enable_if;
using std::enable_if_t;
using std::false_type;
using std::index_sequence;
using std::integral_constant;
using std::invoke_result;
using std::invoke_result_t;
using std::is_array;
using std::is_array_v;
using std::is_bounded_array;
using std::is_bounded_array_v;
using std::is_const;
using std::is_const_v;
using std::is_constant_evaluated;
using std::is_constructible;
using std::is_constructible_v;
using std::is_convertible;
using std::is_convertible_v;
using std::is_enum;
using std::is_enum_v;
using std::is_floating_point;
using std::is_floating_point_v;
using std::is_integral;
using std::is_integral_v;
using std::is_invocable;
using std::is_invocable_r;
using std::is_invocable_r_v;
using std::is_invocable_v;
using std::is_lvalue_reference;
using std::is_lvalue_reference_v;
using std::is_nothrow_move_assignable;
using std::is_nothrow_move_assignable_v;
using std::is_nothrow_move_constructible;
using std::is_nothrow_move_constructible_v;
using std::is_pointer;
using std::is_pointer_v;
using std::is_same;
using std::is_same_v;
using std::is_trivially_copyable;
using std::is_trivially_copyable_v;
using std::is_trivially_default_constructible;
using std::is_trivially_default_constructible_v;
using std::is_trivially_destructible;
using std::is_trivially_destructible_v;
using std::is_void;
using std::is_void_v;
using std::make_index_sequence;
using std::make_unsigned;
using std::make_unsigned_t;
using std::nullptr_t;
using std::numeric_limits;
using std::remove_const;
using std::remove_const_t;
using std::remove_cv;
using std::remove_cv_t;
using std::remove_cvref_t;
using std::remove_pointer;
using std::remove_pointer_t;
using std::remove_reference;
using std::remove_reference_t;
using std::true_type;
using std::void_t;

// single-level extent removal; maps directly to std::remove_extent
template <typename T> using clear_extent = std::remove_extent<T>;

template <typename T> constexpr bool always_false = false;

namespace detail {
template <typename T, bool IsEnum = ystl::is_enum_v<T>> struct underlying_type_base;

template <typename T> struct underlying_type_base<T, true> {
  using type = __underlying_type (T);
};

template <typename T> struct underlying_type_base<T, false> {};
}

// SFINAE-friendly underlying_type; to_underlying stays custom until C++23
template <typename T> struct underlying_type : detail::underlying_type_base<T> {};

template <typename T> using underlying_type_t = typename underlying_type<T>::type;

template <typename T> constexpr underlying_type_t<T> to_underlying (T e) noexcept {
  return static_cast<underlying_type_t<T>> (e);
}

namespace detail {
// get the i-th type from a parameter pack
template <size_t I, typename... Ts> struct TypeAt;
template <size_t I, typename T, typename... Rest> struct TypeAt<I, T, Rest...> : TypeAt<I - 1, Rest...> {};
template <typename T, typename... Rest> struct TypeAt<0, T, Rest...> {
  using type = T;
};

// find the index of the first type matching t
template <size_t I, typename T, typename... Ts> struct IndexOf;
template <size_t I, typename T, typename First, typename... Rest> struct IndexOf<I, T, First, Rest...> : IndexOf<I + 1, T, Rest...> {};
template <size_t I, typename T, typename... Rest> struct IndexOf<I, T, T, Rest...> : integral_constant<size_t, I> {};
}

}
