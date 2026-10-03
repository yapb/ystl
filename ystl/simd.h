// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/cpuflags.h>
#include <ystl/platform.h>

// simd kernels need math constants, so define _use_math_defines first
#ifndef _USE_MATH_DEFINES
  #define _USE_MATH_DEFINES
#endif
#include <math.h>

#if defined(YSTL_HAS_SIMD_RVV)
  #include <cmath>
#endif

#if defined(YSTL_HAS_SIMD_SSE)
  #include <smmintrin.h>
#elif defined(YSTL_HAS_SIMD_NEON)
  #include <arm_neon.h>
#elif defined(YSTL_HAS_SIMD_RVV)
  #include <riscv_vector.h>
#endif

namespace ystl::simd {
#if defined(YSTL_HAS_SIMD_SSE)
  #include <ystl/simd/sse2.h>
#elif defined(YSTL_HAS_SIMD_NEON)
  #include <ystl/simd/neon.h>
#elif defined(YSTL_HAS_SIMD_RVV)
  #include <ystl/simd/rvv.h>
#endif
}

// mathlib comes after simd kernels since it references simd batch
#include <ystl/mathlib.h>

namespace ystl {

#if defined(YSTL_HAS_SIMD)

// forward declaration of vector
template <typename T> class Vec3D;

// simple wrapper for vector
class YSTL_SIMD_ALIGNED SimdVec3Wrap final {
private:
  #if defined(YSTL_HAS_SIMD_SSE)
  static YSTL_FORCE_INLINE __m128 _simd_load_mask (const uint32_t *mask) {
    return _mm_loadu_ps (reinterpret_cast<const float *> (mask));
  }

  template <int XmmMask> static YSTL_SIMD_TARGET_AIL ("sse4.1") __m128 _simd_dpps (__m128 v0, __m128 v1) {
    if (cpuflags.sse41) [[likely]] {
      return _mm_dp_ps (v0, v1, XmmMask);
    }
    else if (cpuflags.sse3) [[unlikely]] {
      const auto mul0 = _mm_mul_ps (v0, v1);
      const auto had0 = _mm_hadd_ps (mul0, mul0);
      const auto had1 = _mm_hadd_ps (had0, had0);

      return had1;
    }

    const auto mul0 = _mm_mul_ps (v0, v1);
    const auto shf0 = _mm_shuffle_ps (mul0, mul0, _MM_SHUFFLE (2, 3, 0, 1));
    const auto sum0 = _mm_add_ps (mul0, shf0);
    const auto shf1 = _mm_shuffle_ps (sum0, sum0, _MM_SHUFFLE (0, 1, 2, 3));

    // horizontal sum broadcast into all lanes, matching _mm_dp_ps semantics
    return _mm_add_ps (sum0, shf1);
  }

  static YSTL_FORCE_INLINE __m128 _simd_div (__m128 v0, __m128 v1) {
    return _mm_andnot_ps (_mm_cmpeq_ps (v1, _mm_setzero_ps ()), _mm_div_ps (v0, v1));
  }
  #endif

public:
  YSTL_DISABLE_ANONYMOUS_UNION_WARNING

  #if defined(YSTL_HAS_SIMD_RVV)

  float YSTL_SIMD_ALIGNED data[4] {};
  float &x = data[0];
  float &y = data[1];
  float &z = data[2];
  float &w = data[3];

  #else
  union {
    #if defined(YSTL_HAS_SIMD_SSE)

    __m128 m { _mm_setzero_ps () };

    #else

    float32x4_t m { vdupq_n_f32 (0) };

    #endif
    struct {
      float x, y, z, w;
    };
  };
  #endif
  YSTL_RESTORE_ANONYMOUS_UNION_WARNING

  SimdVec3Wrap () = default;
  ~SimdVec3Wrap () = default;

  SimdVec3Wrap (const float &x_in, const float &y_in, const float &z_in) {
  #if defined(YSTL_HAS_SIMD_SSE)
    m = _mm_set_ps (0.0f, z_in, y_in, x_in);
  #elif defined(YSTL_HAS_SIMD_RVV)
    data[0] = x_in;
    data[1] = y_in;
    data[2] = z_in;
    data[3] = 0.0f;
  #else
    float YSTL_SIMD_ALIGNED arr[4] = { x_in, y_in, z_in, 0.0f };
    m = vld1q_f32 (arr);
  #endif
  }

  SimdVec3Wrap (const float &x_in, const float &y_in) {
  #if defined(YSTL_HAS_SIMD_SSE)
    m = _mm_set_ps (0.0f, 0.0f, y_in, x_in);
  #elif defined(YSTL_HAS_SIMD_RVV)
    data[0] = x_in;
    data[1] = y_in;
    data[2] = 0.0f;
    data[3] = 0.0f;
  #else
    float YSTL_SIMD_ALIGNED arr[4] = { x_in, y_in, 0.0f, 0.0f };
    m = vld1q_f32 (arr);
  #endif
  }

  #if defined(YSTL_HAS_SIMD_SSE)
  SimdVec3Wrap (__m128 in) : m (in) {}
  #elif defined(YSTL_HAS_SIMD_RVV)
  SimdVec3Wrap (vfloat32m1_t v) {
    simd::rvv_store4f (data, v);
  }
  #else
  SimdVec3Wrap (float32x4_t in) : m (in) {}
  #endif

public:
  #if defined(YSTL_HAS_SIMD_RVV)
  SimdVec3Wrap normalize () const {
    vfloat32m1_t v = simd::rvv_load4f (data);
    float dot = simd::rvv_reduce_sum4f (simd::rvv_mul4f (v, v));
    vfloat32m1_t len = simd::rvv_splat4f (::sqrtf (dot));
    vfloat32m1_t result = simd::div_ps (v, len);
    return SimdVec3Wrap (result);
  }

  YSTL_FORCE_INLINE void angle_vectors (SimdVec3Wrap &sines, SimdVec3Wrap &cosines) {
    static constexpr YSTL_SIMD_ALIGNED float kDegToRad[] = { kDegreeToRadians, kDegreeToRadians, kDegreeToRadians, kDegreeToRadians };

    vfloat32m1_t v = simd::rvv_load4f (data);
    vfloat32m1_t rad = simd::rvv_load4f (kDegToRad);
    vfloat32m1_t angles = simd::rvv_mul4f (v, rad);
    vfloat32m1_t s, c;

    simd::sincos_ps (angles, s, c);
    simd::rvv_store4f (sines.data, s);
    simd::rvv_store4f (cosines.data, c);
  }
  #elif defined(YSTL_HAS_SIMD_SSE)

  YSTL_SIMD_TARGET ("sse4.1")
  SimdVec3Wrap normalize () const {
    return _simd_div (m, _mm_sqrt_ps (_simd_dpps<0xff> (m, m)));
  }

  // t must expose a data[3] member the results are stored into (see vec3d)
  template <typename T>
    requires requires (T *t) { t->data; }
  YSTL_FORCE_INLINE void angle_vectors (T *forward, T *right, T *upward) {
    static constexpr YSTL_SIMD_ALIGNED float kDegToRad[] = { kDegreeToRadians, kDegreeToRadians, kDegreeToRadians, kDegreeToRadians };
    static constexpr YSTL_SIMD_ALIGNED uint32_t kNegMask_1111[4] = { 0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u };
    static constexpr YSTL_SIMD_ALIGNED uint32_t kNegMask_1001[4] = { 0x80000000u, 0u, 0u, 0x80000000u };

    // sincos_ps(x, sin, cos): keep the canonical order so sin0 holds the
    // sine and cos0 the cosine throughout the shuffle math below.
    __m128 sin0, cos0;
    simd::sincos_ps (_mm_mul_ps (m, _mm_load_ps (kDegToRad)), sin0, cos0);

    auto shf0 = _mm_shuffle_ps (cos0, sin0, _MM_SHUFFLE (2, 1, 0, 0));
    auto shf1 = _mm_shuffle_ps (cos0, cos0, _MM_SHUFFLE (0, 0, 2, 1));
    auto mul0 = _mm_mul_ps (shf0, shf1);

    shf0 = _mm_shuffle_ps (cos0, sin0, _MM_SHUFFLE (0, 1, 1, 1));
    shf1 = _mm_shuffle_ps (sin0, cos0, _MM_SHUFFLE (2, 2, 0, 0));
    shf0 = _mm_shuffle_ps (shf0, shf0, _MM_SHUFFLE (3, 0, 2, 0));

    auto shf2 = _mm_shuffle_ps (sin0, sin0, _MM_SHUFFLE (1, 0, 2, 2));
    shf2 = _mm_mul_ps (shf2, _mm_mul_ps (shf0, shf1));

    shf1 = _mm_shuffle_ps (sin0, cos0, _MM_SHUFFLE (1, 2, 1, 1));
    shf0 = _mm_shuffle_ps (cos0, sin0, _MM_SHUFFLE (2, 2, 1, 2));
    shf1 = _mm_shuffle_ps (shf1, shf1, _MM_SHUFFLE (3, 1, 2, 0));

    shf0 = _mm_xor_ps (shf0, _simd_load_mask (kNegMask_1001));
    shf0 = _mm_mul_ps (shf0, shf1);

    shf2 = _mm_add_ps (shf2, shf0);

    if (forward) {
      _mm_storeu_si64 (forward->data, _mm_castps_si128 (_mm_shuffle_ps (mul0, mul0, _MM_SHUFFLE (0, 0, 2, 0))));
      forward->data[2] = -_mm_cvtss_f32 (sin0);
    }

    if (right) {
      auto shf3 = _mm_shuffle_ps (shf2, mul0, _MM_SHUFFLE (3, 3, 1, 0));
      shf3 = _mm_xor_ps (shf3, _simd_load_mask (kNegMask_1111));

      _mm_storeu_si64 (right->data, _mm_castps_si128 (shf3));
      _mm_store_ss (&right->data[2], _mm_shuffle_ps (shf3, shf3, _MM_SHUFFLE (0, 0, 0, 2)));
    }

    if (upward) {
      _mm_storeu_si64 (upward->data, _mm_castps_si128 (_mm_shuffle_ps (shf2, shf2, _MM_SHUFFLE (0, 0, 3, 2))));
      upward->data[2] = _mm_cvtss_f32 (_mm_shuffle_ps (mul0, mul0, _MM_SHUFFLE (0, 0, 0, 1)));
    }
  }
  #else
  // native neon, no sse2neon emulated intrinsics anywhere on this path
  SimdVec3Wrap normalize () const {
    const float32x4_t mul = vmulq_f32 (m, m);

    #if defined(__aarch64__)
    float32x4_t sum = vpaddq_f32 (mul, mul);
    sum = vpaddq_f32 (sum, sum);

    const float32x4_t len = vsqrtq_f32 (sum);
    // zero length guard: divide by one instead, so a zero vector stays zero
    const float32x4_t safe = vbslq_f32 (vceqq_f32 (len, vdupq_n_f32 (0.0f)), vdupq_n_f32 (1.0f), len);

    return SimdVec3Wrap (vdivq_f32 (m, safe));
    #else
    // armv7 has no vector division or sqrt, fall back to scalar length
    const float32x2_t lo = vadd_f32 (vget_low_f32 (mul), vget_high_f32 (mul));
    const float len = ::sqrtf (vget_lane_f32 (vpadd_f32 (lo, lo), 0));
    const float inv = len == 0.0f ? 1.0f : 1.0f / len;

    return SimdVec3Wrap (vmulq_n_f32 (m, inv));
    #endif
  }

  YSTL_FORCE_INLINE void angle_vectors (SimdVec3Wrap &sines, SimdVec3Wrap &cosines) {
    static constexpr YSTL_SIMD_ALIGNED float kDegToRad[] = { kDegreeToRadians, kDegreeToRadians, kDegreeToRadians, kDegreeToRadians };
    simd::sincos_ps (vmulq_f32 (m, vld1q_f32 (kDegToRad)), sines.m, cosines.m);
  }
  #endif
};

#endif

}
