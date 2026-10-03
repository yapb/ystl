// SPDX-License-Identifier: Unlicense

#pragma once

#include <riscv_vector.h>

constexpr auto c_sincof_p0 = -1.9515295891E-4f;
constexpr auto c_sincof_p1 = 8.3321608736E-3f;
constexpr auto c_sincof_p2 = -1.6666654611E-1f;

constexpr auto c_minus_cephes_DP1 = -0.78515625f;
constexpr auto c_minus_cephes_DP2 = -2.4187564849853515625e-4f;
constexpr auto c_minus_cephes_DP3 = -3.77489497744594108e-8f;

constexpr auto c_coscof_p0 = 2.443315711809948E-005f;
constexpr auto c_coscof_p1 = -1.388731625493765E-003f;
constexpr auto c_coscof_p2 = 4.166664568298827E-002f;

constexpr auto c_cephes_FOPI = 1.27323954473516f;

static inline void sincos_ps (vfloat32m1_t x, vfloat32m1_t &ysin, vfloat32m1_t &ycos) {
  const size_t vl = 4;

  vfloat32m1_t xmm1, xmm2, xmm3, y;
  vuint32m1_t emm2;
  vbool32_t sign_mask_sin, sign_mask_cos;

  vfloat32m1_t zero = __riscv_vfmv_v_f_f32m1 (0.0f, vl);
  sign_mask_sin = __riscv_vmflt_vf_f32m1_b32 (x, 0.0f, vl);
  x = __riscv_vfabs_v_f32m1 (x, vl);

  y = __riscv_vfmul_vf_f32m1 (x, c_cephes_FOPI, vl);

  emm2 = __riscv_vfcvt_xu_f_v_u32m1 (y, vl);
  emm2 = __riscv_vadd_vx_u32m1 (emm2, 1, vl);
  emm2 = __riscv_vand_vx_u32m1 (emm2, ~1u, vl);
  y = __riscv_vfcvt_f_xu_v_f32m1 (emm2, vl);

  vuint32m1_t emm2_and_2 = __riscv_vand_vx_u32m1 (emm2, 2, vl);
  vbool32_t poly_mask = __riscv_vmsne_vx_u32m1_b32 (emm2_and_2, 0, vl);

  xmm1 = __riscv_vfmul_vf_f32m1 (y, c_minus_cephes_DP1, vl);
  xmm2 = __riscv_vfmul_vf_f32m1 (y, c_minus_cephes_DP2, vl);
  xmm3 = __riscv_vfmul_vf_f32m1 (y, c_minus_cephes_DP3, vl);
  x = __riscv_vfadd_vv_f32m1 (x, xmm1, vl);
  x = __riscv_vfadd_vv_f32m1 (x, xmm2, vl);
  x = __riscv_vfadd_vv_f32m1 (x, xmm3, vl);

  vuint32m1_t emm2_and_4 = __riscv_vand_vx_u32m1 (emm2, 4, vl);
  vbool32_t emm2_has_4 = __riscv_vmsne_vx_u32m1_b32 (emm2_and_4, 0, vl);
  sign_mask_sin = __riscv_vmxor_mm_b32 (sign_mask_sin, emm2_has_4, vl);

  vuint32m1_t emm2_minus_2 = __riscv_vsub_vx_u32m1 (emm2, 2, vl);
  vuint32m1_t emm2_m2_and_4 = __riscv_vand_vx_u32m1 (emm2_minus_2, 4, vl);
  sign_mask_cos = __riscv_vmsne_vx_u32m1_b32 (emm2_m2_and_4, 0, vl);

  vfloat32m1_t z = __riscv_vfmul_vv_f32m1 (x, x, vl);
  vfloat32m1_t y1, y2;

  y1 = __riscv_vfmul_vf_f32m1 (z, c_coscof_p0, vl);
  y2 = __riscv_vfmul_vf_f32m1 (z, c_sincof_p0, vl);
  y1 = __riscv_vfadd_vf_f32m1 (y1, c_coscof_p1, vl);
  y2 = __riscv_vfadd_vf_f32m1 (y2, c_sincof_p1, vl);
  y1 = __riscv_vfmul_vv_f32m1 (y1, z, vl);
  y2 = __riscv_vfmul_vv_f32m1 (y2, z, vl);
  y1 = __riscv_vfadd_vf_f32m1 (y1, c_coscof_p2, vl);
  y2 = __riscv_vfadd_vf_f32m1 (y2, c_sincof_p2, vl);
  y1 = __riscv_vfmul_vv_f32m1 (y1, z, vl);
  y2 = __riscv_vfmul_vv_f32m1 (y2, z, vl);
  y1 = __riscv_vfmul_vv_f32m1 (y1, z, vl);
  y2 = __riscv_vfmul_vv_f32m1 (y2, x, vl);
  y1 = __riscv_vfsub_vv_f32m1 (y1, __riscv_vfmul_vf_f32m1 (z, 0.5f, vl), vl);
  y2 = __riscv_vfadd_vv_f32m1 (y2, x, vl);
  y1 = __riscv_vfadd_vf_f32m1 (y1, 1.0f, vl);

  vfloat32m1_t ys = __riscv_vmerge_vvm_f32m1 (y2, y1, poly_mask, vl);
  vfloat32m1_t yc = __riscv_vmerge_vvm_f32m1 (y1, y2, poly_mask, vl);

  ysin = __riscv_vmerge_vvm_f32m1 (ys, __riscv_vfneg_v_f32m1 (ys, vl), sign_mask_sin, vl);
  ycos = __riscv_vmerge_vvm_f32m1 (__riscv_vfneg_v_f32m1 (yc, vl), yc, sign_mask_cos, vl);
}

static inline vfloat32m1_t div_ps (vfloat32m1_t a, vfloat32m1_t b) {
  const size_t vl = 4;
  return __riscv_vfdiv_vv_f32m1 (a, b, vl);
}

static inline vfloat32m1_t sqrt_ps (vfloat32m1_t a) {
  const size_t vl = 4;
  return __riscv_vfsqrt_v_f32m1 (a, vl);
}

static inline vfloat32m1_t rvv_load4f (const float *data) {
  const size_t vl = 4;
  return __riscv_vle32_v_f32m1 (data, vl);
}

static inline void rvv_store4f (float *data, vfloat32m1_t v) {
  const size_t vl = 4;
  __riscv_vse32_v_f32m1 (data, v, vl);
}

static inline vfloat32m1_t rvv_set4f (float a, float b, float c, float d) {
  const size_t vl = 4;
  alignas (16) float data[4] = { a, b, c, d };
  return __riscv_vle32_v_f32m1 (data, vl);
}

static inline vfloat32m1_t rvv_splat4f (float val) {
  const size_t vl = 4;
  return __riscv_vfmv_v_f_f32m1 (val, vl);
}

static inline vfloat32m1_t rvv_zero4f () {
  const size_t vl = 4;
  return __riscv_vfmv_v_f_f32m1 (0.0f, vl);
}

static inline vfloat32m1_t rvv_mul4f (vfloat32m1_t a, vfloat32m1_t b) {
  const size_t vl = 4;
  return __riscv_vfmul_vv_f32m1 (a, b, vl);
}

static inline vfloat32m1_t rvv_add4f (vfloat32m1_t a, vfloat32m1_t b) {
  const size_t vl = 4;
  return __riscv_vfadd_vv_f32m1 (a, b, vl);
}

static inline vfloat32m1_t rvv_sub4f (vfloat32m1_t a, vfloat32m1_t b) {
  const size_t vl = 4;
  return __riscv_vfsub_vv_f32m1 (a, b, vl);
}

static inline float rvv_extract_f32 (vfloat32m1_t v, int index) {
  const size_t vl = 4;
  alignas (16) float data[4];
  __riscv_vse32_v_f32m1 (data, v, vl);
  return data[index];
}

static inline float rvv_reduce_sum4f (vfloat32m1_t v) {
  const size_t vl = 4;
  vfloat32m1_t zero = __riscv_vfmv_v_f_f32m1 (0.0f, vl);
  vfloat32m1_t sum = __riscv_vfredusum_vs_f32m1_f32m1 (v, zero, vl);
  return __riscv_vfmv_f_s_f32m1_f32 (sum);
}
