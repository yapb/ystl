// benchmark_vector.cpp - benchmark ystl vector simd paths (dpps, normalize, hypot, anglevectors) vs scalar math
#include <ystl/ystl.h>
#include <cmath>
#include <vector>
#include <random>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumVectors = 4096;

static std::vector<Vector> generate_random_vectors (size_t count) {
  std::mt19937 gen (42);
  std::uniform_real_distribution<float> dist (-1000.0f, 1000.0f);

  std::vector<Vector> values (count);
  for (size_t i = 0; i < count; ++i) {
    values[i] = { dist (gen), dist (gen), dist (gen) };
  }
  return values;
}

static std::vector<Vector> generate_random_view_angles (size_t count) {
  std::mt19937 gen (1337);
  std::uniform_real_distribution<float> pitch (-89.0f, 89.0f);
  std::uniform_real_distribution<float> yaw (-180.0f, 180.0f);

  std::vector<Vector> values (count);
  for (size_t i = 0; i < count; ++i) {
    values[i] = { pitch (gen), yaw (gen), 0.0f };
  }
  return values;
}

template <typename T> static void sink (T &&value) {
  ystl::benchmark::deoptimize_value (std::forward<T> (value));
}

// scalar references: what

static Vector scalar_normalize (const Vector &v) {
  const float inv = 1.0f / ::sqrtf (v.x * v.x + v.y * v.y + v.z * v.z);
  return { v.x * inv, v.y * inv, v.z * inv };
}

static void scalar_angle_vectors (const Vector &angles, Vector *forward, Vector *right, Vector *upward) {
  float sp, cp, sy, cy;
  ystl::sincosf (ystl::deg2rad (angles.x), sp, cp);
  ystl::sincosf (ystl::deg2rad (angles.y), sy, cy);

  if (forward) {
    *forward = { cp * cy, cp * sy, -sp };
  }

  if (right) {
    *right = { sy, -cy, 0.0f }; // roll-free, matches ystl basis convention
  }

  if (upward) {
    *upward = { sp * cy, sp * sy, cp };
  }
}

// single vector benchmarks (latency-bound, 1 of 4 simd lanes used)
TEST_CASE ("Vector normalize benchmark (dpps + sqrt + div vs scalar) [benchmark][vector][simd]") {
  auto values = generate_random_vectors (kNumVectors);
  std::vector<Vector> results (kNumVectors);

  BENCHMARK_ADVANCED ("simd normalize") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        results[i] = values[i].normalize ();
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("scalar normalize") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        results[i] = scalar_normalize (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// anglevectors: the per-frame bot hot path (aim direction basis)

// extract variant pulls sin and cos components from simd registers
static void extract_angle_vectors (const Vector &angles, Vector *forward, Vector *right, Vector *upward) {
#if defined(YSTL_HAS_SIMD_SSE)
  static constexpr YSTL_SIMD_ALIGNED float kDegToRad[4] = { kDegreeToRadians, kDegreeToRadians, kDegreeToRadians, kDegreeToRadians };

  const __m128 ang = _mm_mul_ps (_mm_setr_ps (angles.x, angles.y, angles.z, 0.0f), _mm_load_ps (kDegToRad));

  __m128 sin0, cos0;
  simd::sincos_ps (ang, sin0, cos0);

  const float sp = _mm_cvtss_f32 (sin0);
  const float cp = _mm_cvtss_f32 (cos0);
  const float sy = _mm_cvtss_f32 (_mm_shuffle_ps (sin0, sin0, _MM_SHUFFLE (1, 1, 1, 1)));
  const float cy = _mm_cvtss_f32 (_mm_shuffle_ps (cos0, cos0, _MM_SHUFFLE (1, 1, 1, 1)));
  const float sr = _mm_cvtss_f32 (_mm_shuffle_ps (sin0, sin0, _MM_SHUFFLE (2, 2, 2, 2)));
  const float cr = _mm_cvtss_f32 (_mm_shuffle_ps (cos0, cos0, _MM_SHUFFLE (2, 2, 2, 2)));

  if (forward) {
    forward->data[0] = cp * cy;
    forward->data[1] = cp * sy;
    forward->data[2] = -sp;
  }

  if (right) {
    right->data[0] = -sr * sp * cy + cr * sy;
    right->data[1] = -sr * sp * sy - cr * cy;
    right->data[2] = -sr * cp;
  }

  if (upward) {
    upward->data[0] = cr * sp * cy + sr * sy;
    upward->data[1] = cr * sp * sy - sr * cy;
    upward->data[2] = cr * cp;
  }
#else
  scalar_angle_vectors (angles, forward, right, upward);
#endif
}

TEST_CASE ("Vector angleVectors benchmark (sincos_ps + shuffles vs scalar sincosf) [benchmark][vector][simd]") {
  auto angles = generate_random_view_angles (kNumVectors);
  std::vector<Vector> fwd (kNumVectors);
  std::vector<Vector> rgt (kNumVectors);
  std::vector<Vector> up (kNumVectors);

  BENCHMARK_ADVANCED ("simd angleVectors (sincos_ps)") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        angles[i].angle_vectors (&fwd[i], &rgt[i], &up[i]);
      }
      sink (fwd.data ());
      sink (rgt.data ());
      sink (up.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("scalar angleVectors (sincosf)") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        scalar_angle_vectors (angles[i], &fwd[i], &rgt[i], &up[i]);
      }
      sink (fwd.data ());
      sink (rgt.data ());
      sink (up.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("extract angleVectors (sincos_ps + scalar basis)") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        extract_angle_vectors (angles[i], &fwd[i], &rgt[i], &up[i]);
      }
      sink (fwd.data ());
      sink (rgt.data ());
      sink (up.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("simd angleVectors (forward only)") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        angles[i].angle_vectors (&fwd[i], nullptr, nullptr);
      }
      sink (fwd.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("scalar angleVectors (forward only)") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumVectors; ++i) {
        scalar_angle_vectors (angles[i], &fwd[i], nullptr, nullptr);
      }
      sink (fwd.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}
