// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/traits.h>

#if defined(YSTL_CXX_MSVC) && !defined(YSTL_CXX_CLANG) && !defined(YSTL_CXX_GCC)
  #include <intrin.h>
#endif

namespace ystl {

// memory ordering constants, mirrors the semantics of the standard ones
enum class MemoryOrder : int {
  relaxed = 0,
  consume = 1,
  acquire = 2,
  release = 3,
  acq_rel = 4,
  seq_cst = 5,
};

namespace detail {
#if defined(YSTL_CXX_MSVC) && !defined(YSTL_CXX_CLANG) && !defined(YSTL_CXX_GCC)
// vanilla msvc: interlocked intrinsics are always sequentially consistent, the memory ordering is ignored
template <typename T> struct AtomicIntrinsics {
private:
  // 64-bit compare-exchange is available on every supported msvc architecture (cmpxchg8b / lock cmpxchg)
  static YSTL_FORCE_INLINE int64_t cas64 (volatile int64_t *ptr, int64_t exchange, int64_t comparand) noexcept {
    return _InterlockedCompareExchange64 (ptr, exchange, comparand);
  }

  static YSTL_FORCE_INLINE int64_t load64 (volatile int64_t *ptr) noexcept {
    return cas64 (ptr, 0, 0);
  }

  #if defined(_M_X64) || defined(_M_ARM64) || defined(_M_ARM64EC)
  // 64-bit interlocked arithmetic is natively available here
  static YSTL_FORCE_INLINE int64_t exchange64 (volatile int64_t *ptr, int64_t desired) noexcept {
    return _InterlockedExchange64 (ptr, desired);
  }

  static YSTL_FORCE_INLINE int64_t add64 (volatile int64_t *ptr, int64_t operand) noexcept {
    return _InterlockedExchangeAdd64 (ptr, operand);
  }

  static YSTL_FORCE_INLINE int64_t and64 (volatile int64_t *ptr, int64_t operand) noexcept {
    return _InterlockedAnd64 (ptr, operand);
  }

  static YSTL_FORCE_INLINE int64_t or64 (volatile int64_t *ptr, int64_t operand) noexcept {
    return _InterlockedOr64 (ptr, operand);
  }

  static YSTL_FORCE_INLINE int64_t xor64 (volatile int64_t *ptr, int64_t operand) noexcept {
    return _InterlockedXor64 (ptr, operand);
  }
  #else
  // 32-bit x86 has no 64-bit interlocked arithmetic, emulate with a compare-exchange loop (cmpxchg8b)
  static YSTL_FORCE_INLINE int64_t exchange64 (volatile int64_t *ptr, int64_t desired) noexcept {
    auto old = load64 (ptr);

    while (cas64 (ptr, desired, old) != old) {
      old = load64 (ptr);
    }
    return old;
  }

  static YSTL_FORCE_INLINE int64_t add64 (volatile int64_t *ptr, int64_t operand) noexcept {
    auto old = load64 (ptr);

    for (;;) {
      auto swapped = cas64 (ptr, old + operand, old);

      if (swapped == old) {
        return old;
      }
      old = swapped;
    }
  }

  static YSTL_FORCE_INLINE int64_t and64 (volatile int64_t *ptr, int64_t operand) noexcept {
    auto old = load64 (ptr);

    for (;;) {
      auto swapped = cas64 (ptr, old & operand, old);

      if (swapped == old) {
        return old;
      }
      old = swapped;
    }
  }

  static YSTL_FORCE_INLINE int64_t or64 (volatile int64_t *ptr, int64_t operand) noexcept {
    auto old = load64 (ptr);

    for (;;) {
      auto swapped = cas64 (ptr, old | operand, old);

      if (swapped == old) {
        return old;
      }
      old = swapped;
    }
  }

  static YSTL_FORCE_INLINE int64_t xor64 (volatile int64_t *ptr, int64_t operand) noexcept {
    auto old = load64 (ptr);

    for (;;) {
      auto swapped = cas64 (ptr, old ^ operand, old);

      if (swapped == old) {
        return old;
      }
      old = swapped;
    }
  }
  #endif

public:
  static YSTL_FORCE_INLINE T load (volatile T *ptr, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (load64 (reinterpret_cast<volatile int64_t *> (ptr)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedCompareExchange (reinterpret_cast<volatile long *> (ptr), 0, 0));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedCompareExchange16 (reinterpret_cast<volatile short *> (ptr), 0, 0));
    }
    else {
      return static_cast<T> (_InterlockedCompareExchange8 (reinterpret_cast<volatile char *> (ptr), 0, 0));
    }
  }

  static YSTL_FORCE_INLINE void store (volatile T *ptr, T desired, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      exchange64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (desired));
    }
    else if constexpr (sizeof (T) == 4) {
      _InterlockedExchange (reinterpret_cast<volatile long *> (ptr), static_cast<long> (desired));
    }
    else if constexpr (sizeof (T) == 2) {
      _InterlockedExchange16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (desired));
    }
    else {
      _InterlockedExchange8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (desired));
    }
  }

  static YSTL_FORCE_INLINE T exchange (volatile T *ptr, T desired, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (exchange64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (desired)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedExchange (reinterpret_cast<volatile long *> (ptr), static_cast<long> (desired)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedExchange16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (desired)));
    }
    else {
      return static_cast<T> (_InterlockedExchange8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (desired)));
    }
  }

  static YSTL_FORCE_INLINE T fetch_add (volatile T *ptr, T operand, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (add64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (operand)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedExchangeAdd (reinterpret_cast<volatile long *> (ptr), static_cast<long> (operand)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedExchangeAdd16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (operand)));
    }
    else {
      return static_cast<T> (_InterlockedExchangeAdd8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (operand)));
    }
  }

  static YSTL_FORCE_INLINE T fetch_sub (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    // negate in unsigned domain for well-defined modular arithmetic
    using Unsigned = ystl::make_unsigned_t<T>;
    const auto negated = static_cast<T> (Unsigned (0) - static_cast<Unsigned> (operand));
    return fetch_add (ptr, negated, order);
  }

  static YSTL_FORCE_INLINE T fetch_and (volatile T *ptr, T operand, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (and64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (operand)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedAnd (reinterpret_cast<volatile long *> (ptr), static_cast<long> (operand)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedAnd16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (operand)));
    }
    else {
      return static_cast<T> (_InterlockedAnd8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (operand)));
    }
  }

  static YSTL_FORCE_INLINE T fetch_or (volatile T *ptr, T operand, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (or64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (operand)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedOr (reinterpret_cast<volatile long *> (ptr), static_cast<long> (operand)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedOr16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (operand)));
    }
    else {
      return static_cast<T> (_InterlockedOr8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (operand)));
    }
  }

  static YSTL_FORCE_INLINE T fetch_xor (volatile T *ptr, T operand, MemoryOrder) noexcept {
    if constexpr (sizeof (T) == 8) {
      return static_cast<T> (xor64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (operand)));
    }
    else if constexpr (sizeof (T) == 4) {
      return static_cast<T> (_InterlockedXor (reinterpret_cast<volatile long *> (ptr), static_cast<long> (operand)));
    }
    else if constexpr (sizeof (T) == 2) {
      return static_cast<T> (_InterlockedXor16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (operand)));
    }
    else {
      return static_cast<T> (_InterlockedXor8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (operand)));
    }
  }

  static YSTL_FORCE_INLINE bool compare_exchange (volatile T *ptr, T &expected, T desired, MemoryOrder, MemoryOrder) noexcept {
    T old {};

    if constexpr (sizeof (T) == 8) {
      old = static_cast<T> (cas64 (reinterpret_cast<volatile int64_t *> (ptr), static_cast<int64_t> (desired), static_cast<int64_t> (expected)));
    }
    else if constexpr (sizeof (T) == 4) {
      old = static_cast<T> (
        _InterlockedCompareExchange (reinterpret_cast<volatile long *> (ptr), static_cast<long> (desired), static_cast<long> (expected)));
    }
    else if constexpr (sizeof (T) == 2) {
      old = static_cast<T> (
        _InterlockedCompareExchange16 (reinterpret_cast<volatile short *> (ptr), static_cast<short> (desired), static_cast<short> (expected)));
    }
    else {
      old = static_cast<T> (
        _InterlockedCompareExchange8 (reinterpret_cast<volatile char *> (ptr), static_cast<char> (desired), static_cast<char> (expected)));
    }

    if (old == expected) {
      return true;
    }
    expected = old;
    return false;
  }
};
#else
// gcc, clang and clang-cl: __atomic builtins with full memory ordering support
template <typename T> struct AtomicIntrinsics {
  static YSTL_FORCE_INLINE int atomic_order (MemoryOrder order) noexcept {
    switch (order) {
    case MemoryOrder::relaxed:
      return __ATOMIC_RELAXED;
    case MemoryOrder::consume:
      return __ATOMIC_CONSUME;
    case MemoryOrder::acquire:
      return __ATOMIC_ACQUIRE;
    case MemoryOrder::release:
      return __ATOMIC_RELEASE;
    case MemoryOrder::acq_rel:
      return __ATOMIC_ACQ_REL;
    default:
      return __ATOMIC_SEQ_CST;
    }
  }

  static YSTL_FORCE_INLINE T load (volatile T *ptr, MemoryOrder order) noexcept {
    return __atomic_load_n (ptr, atomic_order (order));
  }

  static YSTL_FORCE_INLINE void store (volatile T *ptr, T desired, MemoryOrder order) noexcept {
    __atomic_store_n (ptr, desired, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T exchange (volatile T *ptr, T desired, MemoryOrder order) noexcept {
    return __atomic_exchange_n (ptr, desired, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T fetch_add (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    return __atomic_fetch_add (ptr, operand, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T fetch_sub (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    return __atomic_fetch_sub (ptr, operand, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T fetch_and (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    return __atomic_fetch_and (ptr, operand, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T fetch_or (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    return __atomic_fetch_or (ptr, operand, atomic_order (order));
  }

  static YSTL_FORCE_INLINE T fetch_xor (volatile T *ptr, T operand, MemoryOrder order) noexcept {
    return __atomic_fetch_xor (ptr, operand, atomic_order (order));
  }

  static YSTL_FORCE_INLINE bool compare_exchange (volatile T *ptr, T &expected, T desired, MemoryOrder success, MemoryOrder failure) noexcept {
    return __atomic_compare_exchange_n (ptr, &expected, desired, false, atomic_order (success), atomic_order (failure));
  }
};
#endif
}

// sequential atomic variable, a lightweight replacement for std::atomic (integral types only)
template <typename T> class Atomic final : public NonCopyable, NonMovable {
  static_assert (ystl::is_integral_v<T>, "ystl::Atomic requires an integral type");
  static_assert (
    sizeof (T) == 1 || sizeof (T) == 2 || sizeof (T) == 4 || sizeof (T) == 8, "ystl::Atomic requires a 1, 2, 4 or 8 byte sized type");

private:
  alignas (T) volatile T value_ {};

public:
  Atomic () noexcept = default;

  Atomic (T desired) noexcept : value_ (desired) {}

  // 8-byte atomics need os locks on some 32-bit targets, ask the compiler
  bool is_lock_free () const noexcept {
#if defined(YSTL_CXX_MSVC) || defined(YSTL_CXX_CLANG_CL)
    return true;
#else
    return __atomic_is_lock_free (sizeof (T), &value_) != 0;
#endif
  }

  T load (MemoryOrder order = MemoryOrder::seq_cst) const noexcept {
    return detail::AtomicIntrinsics<T>::load (const_cast<volatile T *> (&value_), order);
  }

  void store (T desired, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    detail::AtomicIntrinsics<T>::store (&value_, desired, order);
  }

  T exchange (T desired, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::exchange (&value_, desired, order);
  }

  // compares the value with expected, on success writes desired, on failure writes the actual value back into expected
  bool compare_exchange (
    T &expected, T desired, MemoryOrder success = MemoryOrder::seq_cst, MemoryOrder failure = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::compare_exchange (&value_, expected, desired, success, failure);
  }

  T fetch_add (T operand, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::fetch_add (&value_, operand, order);
  }

  T fetch_sub (T operand, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::fetch_sub (&value_, operand, order);
  }

  T fetch_and (T operand, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::fetch_and (&value_, operand, order);
  }

  T fetch_or (T operand, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::fetch_or (&value_, operand, order);
  }

  T fetch_xor (T operand, MemoryOrder order = MemoryOrder::seq_cst) noexcept {
    return detail::AtomicIntrinsics<T>::fetch_xor (&value_, operand, order);
  }

  operator T () const noexcept {
    return load ();
  }

  T operator= (T desired) noexcept {
    store (desired);
    return desired;
  }

  T operator++ () noexcept {
    return fetch_add (1) + 1;
  }
  T operator++ (int) noexcept {
    return fetch_add (1);
  }

  T operator-- () noexcept {
    return fetch_sub (1) - 1;
  }
  T operator-- (int) noexcept {
    return fetch_sub (1);
  }

  T operator+= (T operand) noexcept {
    return fetch_add (operand) + operand;
  }
  T operator-= (T operand) noexcept {
    return fetch_sub (operand) - operand;
  }

  T operator&= (T operand) noexcept {
    return fetch_and (operand) & operand;
  }
  T operator|= (T operand) noexcept {
    return fetch_or (operand) | operand;
  }
  T operator^= (T operand) noexcept {
    return fetch_xor (operand) ^ operand;
  }
};

}
