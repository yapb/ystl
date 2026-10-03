// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/traits.h>

#define _USE_MATH_DEFINES
#include <math.h>

namespace ystl {

constexpr float kFloatOnEpsilon = 0.01f;
constexpr float kFloatEqualEpsilon = 0.001f;
constexpr float kRelativeFloatEqualEpsilon = 1e-6f; // relative term for fequal, ~8 ulps at map scale
constexpr float kFloatEpsilon = 1.192092896e-07f;
constexpr float kMathPi = 3.14159265358979323846f;
constexpr float kDegreeToRadians = kMathPi / 180.0f;
constexpr float kRadiansToDegree = 180.0f / kMathPi;

// design policy: scalar calls route to the standard library builtins

template <typename T> constexpr T min (const T &a, const T &b) {
  return a < b ? a : b;
}

template <typename T> constexpr T max (const T &a, const T &b) {
  return a > b ? a : b;
}

template <typename T> constexpr T clamp (const T &x, const T &a, const T &b) {
  return min (max (x, a), b);
}

template <typename T> constexpr T abs (const T &a) {
  if constexpr (is_same_v<T, double>) {
    return fabs (a);
  }
  else {
    return a < T {} ? -a : a;
  }
}

template <> YSTL_FORCE_INLINE float abs (const float &a) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_fabsf (a);
#else
  return ::fabsf (a);
#endif
}

template <typename T> constexpr T sqrf (const T &value) {
  return value * value;
}

YSTL_FORCE_INLINE float sinf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_sinf (value);
#else
  return ::sinf (value);
#endif
}

YSTL_FORCE_INLINE float cosf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_cosf (value);
#else
  return ::cosf (value);
#endif
}

YSTL_FORCE_INLINE float atan2f (const float y, const float x) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_atan2f (y, x);
#else
  return ::atan2f (y, x);
#endif
}

YSTL_FORCE_INLINE float powf (const float x, const float y) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_powf (x, y);
#else
  return ::powf (x, y);
#endif
}

YSTL_FORCE_INLINE float sqrtf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_sqrtf (value);
#else
  return ::sqrtf (value);
#endif
}

// exact 1.0f / sqrt (value)
YSTL_FORCE_INLINE float rsqrtf (const float value) {
  return 1.0f / sqrtf (value);
}

YSTL_FORCE_INLINE float tanf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_tanf (value);
#else
  return ::tanf (value);
#endif
}

YSTL_FORCE_INLINE float log10f (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_log10f (value);
#else
  return ::log10f (value);
#endif
}

YSTL_FORCE_INLINE float logf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_logf (value);
#else
  return ::logf (value);
#endif
}

YSTL_FORCE_INLINE float expf (const float value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_expf (value);
#else
  return ::expf (value);
#endif
}

#if (defined(YSTL_ARCH_X32) || defined(YSTL_ARCH_X64)) && !defined(__SSE4_1__) && (defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG))
  #define YSTL_ROUND_ARITHMETIC 1
#endif

YSTL_FORCE_INLINE float floorf (const float value) {
#if defined(YSTL_ROUND_ARITHMETIC)
  if (value >= 8388608.0f || value <= -8388608.0f) {
    return value;
  }
  // truncate toward zero, then step down when the input was not integral.
  // the old add-2^23 magic returned half-integers for negative inputs.
  const float result = static_cast<float> (static_cast<int32_t> (value));

  return result > value ? result - 1.0f : result;
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_floorf (value);
#else
  return ::floorf (value);
#endif
}

YSTL_FORCE_INLINE float ceilf (const float value) {
#if defined(YSTL_ROUND_ARITHMETIC)
  if (value >= 8388608.0f || value <= -8388608.0f) {
    return value;
  }
  // truncate toward zero, then step up when the input was not integral
  const float result = static_cast<float> (static_cast<int32_t> (value));

  return result < value ? result + 1.0f : result;
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_ceilf (value);
#else
  return ::ceilf (value);
#endif
}

YSTL_FORCE_INLINE float roundf (const float value) {
#if defined(YSTL_ROUND_ARITHMETIC)
  if (value >= 8388608.0f || value <= -8388608.0f) {
    return value;
  }
  // halfway rounds away from zero, exactly like ::roundf
  return value >= 0.0f ? ystl::floorf (value + 0.5f) : ystl::ceilf (value - 0.5f);
#elif defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  return __builtin_roundf (value);
#else
  return ::roundf (value);
#endif
}

YSTL_FORCE_INLINE void sincosf (const float &x, float &s, float &c) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  s = __builtin_sinf (x);
  c = __builtin_cosf (x);
#else
  s = ::sinf (x);
  c = ::cosf (x);
#endif
}

YSTL_FORCE_INLINE bool fzero (const float e) {
  return ystl::abs (e) < kFloatOnEpsilon;
}

// combined epsilon with absolute floor for comparisons against zero
YSTL_FORCE_INLINE bool fequal (const float a, const float b) {
  return ystl::abs (a - b) <= ystl::max (kFloatEqualEpsilon, kRelativeFloatEqualEpsilon * ystl::max (ystl::abs (a), ystl::abs (b)));
}

constexpr float rad2deg (const float r) {
  return r * kRadiansToDegree;
}

constexpr float deg2rad (const float d) {
  return d * kDegreeToRadians;
}

namespace detail {
template <int D> YSTL_FORCE_INLINE float wrap_angle_slow_fn (const float x) {
  return x - 2.0f * static_cast<float> (D) * ystl::floorf (x / (2.0f * static_cast<float> (D)) + 0.5f);
}
}

// wraps the angle into the [-d, d) range
template <int D> YSTL_FORCE_INLINE float wrap_angle_impl (const float a) {
  constexpr float kRange = static_cast<float> (D);
  constexpr float kFull = 2.0f * kRange;
  constexpr float kSafe = kFull * 8388608.0f;

  if (a >= -kRange && a < kRange) {
    return a;
  }

  if (a > -kSafe && a < kSafe) [[likely]] {
    const float t = a / kFull + 0.5f;
    float n = static_cast<float> (static_cast<int32_t> (t));

    if (t < 0.0f && n != t) {
      n -= 1.0f;
    }
    return a - kFull * n;
  }
  return detail::wrap_angle_slow_fn<D> (a);
}

// wraps the angle into the range [-180.0f, 180.0f)
YSTL_FORCE_INLINE float wrap_angle (const float a) {
  return wrap_angle_impl<180> (a);
}

// wraps the angle into the range [-360.0f, 360.0f)
YSTL_FORCE_INLINE float wrap_angle360 (const float a) {
  return wrap_angle_impl<360> (a);
}

YSTL_FORCE_INLINE float angles_difference (const float a, const float b) {
  return wrap_angle (a - b);
}

}
