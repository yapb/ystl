// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/traits.h>

// opt-in operators & traits for enum class types

namespace ystl {

// true when enable_enum_flags (E) is declared in E's namespace (adl opt-in)
template <typename E> inline constexpr bool is_flags_v = requires { enable_enum_flags (ystl::declval<E> ()); };

// true when enable_enum_arithmetic (E) is declared in E's namespace (adl opt-in)
template <typename E> inline constexpr bool is_flags_arithmetic_v = requires { enable_enum_arithmetic (ystl::declval<E> ()); };

// true for flags and arithmetic enums, or when enable_enum_hash (E) is declared (adl opt-in)
template <typename E>
inline constexpr bool is_flags_hashable_v = is_flags_v<E> || is_flags_arithmetic_v<E> || requires { enable_enum_hash (ystl::declval<E> ()); };

// substitution-friendly underlying type alias, yields no type for non-enum types
template <typename E> using flags_underlying_t = ystl::underlying_type_t<E>;

}

/// declare an enum as a flags type (enables bitwise operators)
#define YSTL_ENABLE_ENUM_FLAGS(E) void enable_enum_flags (E)

// declare an enum as an arithmetic-only type
#define YSTL_ENABLE_ENUM_ARITHMETIC(E) void enable_enum_arithmetic (E)

// declare a plain enum as hashable-only type (enables hashing for hashmap keys, no operators)
#define YSTL_ENABLE_ENUM_HASH(E) void enable_enum_hash (E)

// bitwise operators (requires is_flags_v)

// core bitwise operators
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator| (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) | ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator& (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) & ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator^ (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) ^ ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator~(E value) noexcept {
  return static_cast<E> (~ystl::to_underlying (value));
}

// compound assignment operators
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E &operator|= (E &lhs, E rhs) noexcept {
  return lhs = lhs | rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E &operator&= (E &lhs, E rhs) noexcept {
  return lhs = lhs & rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E &operator^= (E &lhs, E rhs) noexcept {
  return lhs = lhs ^ rhs;
}

// shift operators take the count via underlying type to avoid ambiguity
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator<< (E lhs, ystl::underlying_type_t<E> shift) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) << shift);
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator>> (E lhs, ystl::underlying_type_t<E> shift) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) >> shift);
}

// compound shift assignment operators
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E &operator<<= (E &lhs, ystl::underlying_type_t<E> shift) noexcept {
  return lhs = static_cast<E> (ystl::to_underlying (lhs) << shift);
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E &operator>>= (E &lhs, ystl::underlying_type_t<E> shift) noexcept {
  return lhs = static_cast<E> (ystl::to_underlying (lhs) >> shift);
}

// mixed-type operators (enum with underlying type)

// bitwise or
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator| (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) | rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator| (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs | static_cast<E> (rhs);
}

// bitwise and
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator& (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) & rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator& (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs & static_cast<E> (rhs);
}

// bitwise xor
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator^ (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) ^ rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr E operator^ (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs ^ static_cast<E> (rhs);
}

// mixed-type compound assignment (underlying type)
template <typename E>
  requires ystl::is_flags_v<E>
constexpr ystl::underlying_type_t<E> &operator&= (ystl::underlying_type_t<E> &lhs, E rhs) noexcept {
  return lhs = static_cast<ystl::underlying_type_t<E>> (lhs & rhs);
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr ystl::underlying_type_t<E> &operator|= (ystl::underlying_type_t<E> &lhs, E rhs) noexcept {
  return lhs = static_cast<ystl::underlying_type_t<E>> (lhs | rhs);
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr ystl::underlying_type_t<E> &operator^= (ystl::underlying_type_t<E> &lhs, E rhs) noexcept {
  return lhs = static_cast<ystl::underlying_type_t<E>> (lhs ^ rhs);
}

// mixed-type compound assignment (types other than underlying type)
template <typename T, typename E>
  requires (ystl::is_flags_v<E> && !ystl::is_same_v<T, ystl::flags_underlying_t<E>>)
constexpr T &operator&= (T &lhs, E rhs) noexcept {
  return lhs = static_cast<T> (lhs & ystl::to_underlying (rhs));
}

template <typename T, typename E>
  requires (ystl::is_flags_v<E> && !ystl::is_same_v<T, ystl::flags_underlying_t<E>>)
constexpr T &operator|= (T &lhs, E rhs) noexcept {
  return lhs = static_cast<T> (lhs | ystl::to_underlying (rhs));
}

template <typename T, typename E>
  requires (ystl::is_flags_v<E> && !ystl::is_same_v<T, ystl::flags_underlying_t<E>>)
constexpr T &operator^= (T &lhs, E rhs) noexcept {
  return lhs = static_cast<T> (lhs ^ ystl::to_underlying (rhs));
}

// boolean context operators (requires is_flags_v)

// logical not enables bool checks like if since enum values lack conversion
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool operator!(E value) noexcept {
  return ystl::to_underlying (value) == 0;
}

// equality operators
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool operator== (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return ystl::to_underlying (lhs) == rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool operator== (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return lhs == ystl::to_underlying (rhs);
}

// inequality operators
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool operator!= (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return ystl::to_underlying (lhs) != rhs;
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool operator!= (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return lhs != ystl::to_underlying (rhs);
}

// flag checking/manipulation helpers (requires is_flags_v)

// hasflag has "any of" semantics - returns true if at least one bit of flag is set
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool has_flag (E flags, E flag) noexcept {
  return (flags & flag) != static_cast<E> (0);
}

// allflags checks that all bits of mask are set
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool all_flags (E flags, E mask) noexcept {
  return (flags & mask) == mask;
}

// mixed-type hasflag
template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool has_flag (E flags, ystl::underlying_type_t<E> flag) noexcept {
  return (flags & static_cast<E> (flag)) != static_cast<E> (0);
}

template <typename E>
  requires ystl::is_flags_v<E>
constexpr bool has_flag (ystl::underlying_type_t<E> flags, E flag) noexcept {
  return (static_cast<E> (flags) & flag) != static_cast<E> (0);
}

// hasflag for arbitrary integral flags value (types other than underlying type)
template <typename E, typename T>
  requires (ystl::is_flags_v<E> && ystl::is_integral_v<T> && !ystl::is_same_v<T, ystl::flags_underlying_t<E>>)
constexpr bool has_flag (T flags, E flag) noexcept {
  return (static_cast<E> (flags) & flag) != static_cast<E> (0);
}

// flag setting functions
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E set_flag (E flags, E flag) noexcept {
  return flags | flag;
}

// mixed-type setflag
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E set_flag (E flags, ystl::underlying_type_t<E> flag) noexcept {
  return flags | static_cast<E> (flag);
}

template <typename E, typename T>
  requires (ystl::is_flags_v<E> && ystl::is_integral_v<T> && !ystl::is_same_v<T, E>)
constexpr E set_flag (T flags, E flag) noexcept {
  return static_cast<E> (flags) | flag;
}

// flag clearing functions
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E clear_flag (E flags, E flag) noexcept {
  return flags & ~flag;
}

// mixed-type clearflag
template <typename E>
  requires ystl::is_flags_v<E>
constexpr E clear_flag (E flags, ystl::underlying_type_t<E> flag) noexcept {
  return flags & ~static_cast<E> (flag);
}

template <typename E, typename T>
  requires (ystl::is_flags_v<E> && ystl::is_integral_v<T> && !ystl::is_same_v<T, E>)
constexpr E clear_flag (T flags, E flag) noexcept {
  return static_cast<E> (flags) & ~flag;
}

// arithmetic operators (requires is_flags_arithmetic_v)

// core arithmetic operators
template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator+ (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) + ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator- (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) - ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator* (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) * ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator/ (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) / ystl::to_underlying (rhs));
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator% (E lhs, E rhs) noexcept {
  return static_cast<E> (ystl::to_underlying (lhs) % ystl::to_underlying (rhs));
}

// compound assignment operators
template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator+= (E &lhs, E rhs) noexcept {
  return lhs = lhs + rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator-= (E &lhs, E rhs) noexcept {
  return lhs = lhs - rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator*= (E &lhs, E rhs) noexcept {
  return lhs = lhs * rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator/= (E &lhs, E rhs) noexcept {
  return lhs = lhs / rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator%= (E &lhs, E rhs) noexcept {
  return lhs = lhs % rhs;
}

// mixed-type arithmetic operators
template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator+ (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) + rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator+ (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs + static_cast<E> (rhs);
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator- (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) - rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator- (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs - static_cast<E> (rhs);
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator* (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) * rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator* (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs * static_cast<E> (rhs);
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator/ (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) / rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator/ (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs / static_cast<E> (rhs);
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator% (ystl::underlying_type_t<E> lhs, E rhs) noexcept {
  return static_cast<E> (lhs) % rhs;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator% (E lhs, ystl::underlying_type_t<E> rhs) noexcept {
  return lhs % static_cast<E> (rhs);
}

// increment/decrement operators

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator++ (E &value, int) noexcept {
  E result = value;
  value = static_cast<E> (ystl::to_underlying (value) + 1);
  return result;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator++ (E &value) noexcept {
  value = static_cast<E> (ystl::to_underlying (value) + 1);
  return value;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E operator-- (E &value, int) noexcept {
  E result = value;
  value = static_cast<E> (ystl::to_underlying (value) - 1);
  return result;
}

template <typename E>
  requires ystl::is_flags_arithmetic_v<E>
constexpr E &operator-- (E &value) noexcept {
  value = static_cast<E> (ystl::to_underlying (value) - 1);
  return value;
}
