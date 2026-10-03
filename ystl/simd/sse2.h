// sse packed math helpers derived from the cephes library

#pragma once

#ifndef SSE_MATH_PS_H
  #define SSE_MATH_PS_H

// only the kernel that survived benchmarking against modern compilers is kept: sincos_ps

  #if defined(__GNUC__) || defined(__clang__)
    #define SSE_INLINE  static inline __attribute__((always_inline))
    #define SSE_ALIGN __attribute__((aligned(16)))
  #elif defined(_MSC_VER) || defined(__ICL)
    #define SSE_INLINE  static __forceinline
    #define SSE_ALIGN __declspec (align(16))
  #else
    #define SSE_INLINE  static inline
    #define SSE_ALIGN
  #endif

  #define DECLARE_SSEMATH_PS(Name, Val) \
    static const SSE_ALIGN constexpr float SSEMATH_PS_##Name[4] = { static_cast <float> (Val), static_cast <float> (Val), static_cast <float> (Val), static_cast <float> (Val) };

  #define DECLARE_SSEMATH_PI32(Name, Val) \
    static const SSE_ALIGN constexpr int32_t SSEMATH_PI32_##Name[4] = { static_cast <int32_t> (Val), static_cast <int32_t> (Val), static_cast <int32_t> (Val), static_cast <int32_t> (Val) };

// float constants
DECLARE_SSEMATH_PS (4_PI, 1.27323954473516f)

// integer values to mask out float bits
DECLARE_SSEMATH_PI32 (MASK_SIGN, 0x80000000)
DECLARE_SSEMATH_PI32 (INVMASK_SIGN, ~0x80000000)

// float coefficients
DECLARE_SSEMATH_PS (MINUS_DP1, -0.78515625f)
DECLARE_SSEMATH_PS (MINUS_DP2, -2.4187564849853515625e-4f)
DECLARE_SSEMATH_PS (MINUS_DP3, -3.77489497744594108e-8f)

DECLARE_SSEMATH_PS (SIN_P0, -1.9515295891e-4f)
DECLARE_SSEMATH_PS (SIN_P1, 8.3321608736e-3f)
DECLARE_SSEMATH_PS (SIN_P2, -1.6666654611e-1f)
DECLARE_SSEMATH_PS (COS_P0, 2.443315711809948e-5f)
DECLARE_SSEMATH_PS (COS_P1, -1.388731625493765e-3f)
DECLARE_SSEMATH_PS (COS_P2, 4.166664568298827e-2f)

// sine and cosine for angles within limited range for accuracy
SSE_INLINE void sincos_ps (__m128 ang, //!< angle in radian
  __m128 &rSin, //!< returns \f$\sin({ang})\f$
  __m128 &rCos //!< returns \f$\cos({ang})\f$
) {
  // abs( x )
  __m128 x = _mm_and_ps (ang, *reinterpret_cast<const __m128 *> (SSEMATH_PI32_INVMASK_SIGN));

  // tmp = x / ( pi / 4 )
  __m128 tmp = _mm_mul_ps (x, *reinterpret_cast<const __m128 *> (SSEMATH_PS_4_PI));

  // from cephes: j = ( j + 1 ) & ( ~1 )
  __m128i tmpi1 = _mm_cvttps_epi32 (tmp);
  tmpi1 = _mm_add_epi32 (tmpi1, _mm_set1_epi32 (1));
  tmpi1 = _mm_and_si128 (tmpi1, _mm_set1_epi32 (~1));
  tmp = _mm_cvtepi32_ps (tmpi1);

  // x = ( ( x - y * dp1 ) - y * dp2 ) - y * dp3;
  x = _mm_add_ps (x, _mm_mul_ps (tmp, *reinterpret_cast<const __m128 *> (SSEMATH_PS_MINUS_DP1)));
  x = _mm_add_ps (x, _mm_mul_ps (tmp, *reinterpret_cast<const __m128 *> (SSEMATH_PS_MINUS_DP2)));
  x = _mm_add_ps (x, _mm_mul_ps (tmp, *reinterpret_cast<const __m128 *> (SSEMATH_PS_MINUS_DP3)));

  // set cosine sign bit
  __m128i tmpi2 = _mm_sub_epi32 (tmpi1, _mm_set1_epi32 (2));
  __m128 sign_bit_cos = _mm_castsi128_ps (_mm_slli_epi32 (_mm_andnot_si128 (tmpi2, _mm_set1_epi32 (4)), 29));

  // set sine sign bit
  __m128 sign_bit_sin = _mm_and_ps (ang, *reinterpret_cast<const __m128 *> (SSEMATH_PI32_MASK_SIGN));
  __m128 swap_sign_bit_sin = _mm_castsi128_ps (_mm_slli_epi32 (_mm_and_si128 (tmpi1, _mm_set1_epi32 (4)), 29));
  sign_bit_sin = _mm_xor_ps (sign_bit_sin, swap_sign_bit_sin);

  // xx = x ^ 2
  __m128 xx = _mm_mul_ps (x, x);

  // cosine polynom
  __m128 y = *reinterpret_cast<const __m128 *> (SSEMATH_PS_COS_P0);
  y = _mm_mul_ps (y, xx);
  y = _mm_add_ps (y, *reinterpret_cast<const __m128 *> (SSEMATH_PS_COS_P1));
  y = _mm_mul_ps (y, xx);
  y = _mm_add_ps (y, *reinterpret_cast<const __m128 *> (SSEMATH_PS_COS_P2));
  y = _mm_mul_ps (y, xx);
  y = _mm_mul_ps (y, xx);
  y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
  y = _mm_add_ps (y, _mm_set1_ps (1.0f));

  // sine polynom
  __m128 y2 = *reinterpret_cast<const __m128 *> (SSEMATH_PS_SIN_P0);
  y2 = _mm_mul_ps (y2, xx);
  y2 = _mm_add_ps (y2, *reinterpret_cast<const __m128 *> (SSEMATH_PS_SIN_P1));
  y2 = _mm_mul_ps (y2, xx);
  y2 = _mm_add_ps (y2, *reinterpret_cast<const __m128 *> (SSEMATH_PS_SIN_P2));
  y2 = _mm_mul_ps (y2, xx);
  y2 = _mm_mul_ps (y2, x);
  y2 = _mm_add_ps (y2, x);

  // use masks to select polynom
  tmpi1 = _mm_and_si128 (tmpi1, _mm_set1_epi32 (2));
  tmpi1 = _mm_cmpeq_epi32 (tmpi1, _mm_setzero_si128 ());
  __m128 poly_mask = _mm_castsi128_ps (tmpi1);
  __m128 ysin2 = _mm_and_ps (poly_mask, y2);
  __m128 ysin1 = _mm_andnot_ps (poly_mask, y);
  y2 = _mm_sub_ps (y2, ysin2);
  y = _mm_sub_ps (y, ysin1);

  // toggle sign
  rSin = _mm_xor_ps (_mm_add_ps (ysin1, ysin2), sign_bit_sin);
  rCos = _mm_xor_ps (_mm_add_ps (y, y2), sign_bit_cos);
}

#endif // sse_math_ps_h
