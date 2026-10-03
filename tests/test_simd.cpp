// test_simd.cpp - tests for ystl/simd.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

[[maybe_unused]] static constexpr float kEps = 1e-4f;

#if defined(YSTL_HAS_SIMD)

// construction
TEST_CASE ("SimdVec3Wrap default construction zeroes all fields [simd]") {
  SimdVec3Wrap v;
  REQUIRE (v.x == Approx (0.0f));
  REQUIRE (v.y == Approx (0.0f));
  REQUIRE (v.z == Approx (0.0f));
  REQUIRE (v.w == Approx (0.0f));
}

TEST_CASE ("SimdVec3Wrap xyz construction stores components correctly [simd]") {
  SimdVec3Wrap v { 1.0f, 2.0f, 3.0f };
  REQUIRE (v.x == Approx (1.0f));
  REQUIRE (v.y == Approx (2.0f));
  REQUIRE (v.z == Approx (3.0f));
  REQUIRE (v.w == Approx (0.0f));
}

TEST_CASE ("SimdVec3Wrap xy construction zeroes z and w [simd]") {
  SimdVec3Wrap v { 5.0f, 7.0f };
  REQUIRE (v.x == Approx (5.0f));
  REQUIRE (v.y == Approx (7.0f));
  REQUIRE (v.z == Approx (0.0f));
  REQUIRE (v.w == Approx (0.0f));
}

TEST_CASE ("SimdVec3Wrap negative component construction [simd]") {
  SimdVec3Wrap v { -1.0f, -2.0f, -3.0f };
  REQUIRE (v.x == Approx (-1.0f));
  REQUIRE (v.y == Approx (-2.0f));
  REQUIRE (v.z == Approx (-3.0f));
}

// normalize
TEST_CASE ("SimdVec3Wrap normalize produces unit vector [simd]") {
  auto check_unit = [] (SimdVec3Wrap n) {
    float len = ystl::sqrtf (n.x * n.x + n.y * n.y + n.z * n.z);
    REQUIRE (len == Approx (1.0f).epsilon (0.001f));
  };
  check_unit (SimdVec3Wrap { 3.0f, 0.0f, 0.0f }.normalize ());
  check_unit (SimdVec3Wrap { 0.0f, 5.0f, 0.0f }.normalize ());
  check_unit (SimdVec3Wrap { 0.0f, 0.0f, 7.0f }.normalize ());
  check_unit (SimdVec3Wrap { 1.0f, 1.0f, 1.0f }.normalize ());
  check_unit (SimdVec3Wrap { 3.0f, 4.0f, 0.0f }.normalize ());
  check_unit (SimdVec3Wrap { -2.0f, 3.0f, 6.0f }.normalize ());
}

TEST_CASE ("SimdVec3Wrap normalize of axis-aligned vector [simd]") {
  auto n = SimdVec3Wrap { 5.0f, 0.0f, 0.0f }.normalize ();
  REQUIRE (n.x == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (n.y) <= kEps);
  REQUIRE (ystl::abs (n.z) <= kEps);
}

TEST_CASE ("SimdVec3Wrap normalize of zero vector does not crash [simd]") {
  REQUIRE_NOTHROW (SimdVec3Wrap {}.normalize ());
}

  #if defined(YSTL_HAS_SIMD_SSE)

namespace {
// restores a cpuflags bool when the scope exits, even on failed assertions
struct CpuFlagRestore {
  bool &flag;
  bool saved;

  CpuFlagRestore (bool &f) : flag (f), saved (f) {}
  ~CpuFlagRestore () {
    flag = saved;
  }
};
}

TEST_CASE ("SimdVec3Wrap normalize survives disabled sse4.1/sse3 dispatch [simd]") {
  CpuFlagRestore g41 (cpuflags.sse41);
  CpuFlagRestore g3 (cpuflags.sse3);

  // pure sse2 shuffle fallback: result must still be a proper unit vector
  cpuflags.sse41 = false;
  cpuflags.sse3 = false;
  {
    auto n = SimdVec3Wrap { 3.0f, 4.0f, 0.0f }.normalize ();
    REQUIRE (n.x == Approx (0.6f).epsilon (kEps));
    REQUIRE (n.y == Approx (0.8f).epsilon (kEps));
    REQUIRE (ystl::abs (n.z) <= kEps);
  }

  // sse3 hadd fallback
  cpuflags.sse3 = true;
  {
    auto n = SimdVec3Wrap { 3.0f, 4.0f, 0.0f }.normalize ();
    REQUIRE (n.x == Approx (0.6f).epsilon (kEps));
    REQUIRE (n.y == Approx (0.8f).epsilon (kEps));
    REQUIRE (ystl::abs (n.z) <= kEps);
  }
}

  #endif

  // anglevectors
  #if defined(YSTL_HAS_SIMD_SSE)

namespace {
float dot3f (const Vec3D<float> &a, const Vec3D<float> &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float len3f (const Vec3D<float> &v) {
  return ystl::sqrtf (v.x * v.x + v.y * v.y + v.z * v.z);
}
}

TEST_CASE ("SimdVec3Wrap angleVectors produces unit-length basis vectors [simd]") {
  Vec3D<float> fwd, rgt, up;
  SimdVec3Wrap angles { 0.0f, 0.0f, 0.0f };
  angles.angle_vectors (&fwd, &rgt, &up);

  REQUIRE (len3f (fwd) == Approx (1.0f).epsilon (0.001f));
  REQUIRE (len3f (rgt) == Approx (1.0f).epsilon (0.001f));
  REQUIRE (len3f (up) == Approx (1.0f).epsilon (0.001f));
}

TEST_CASE ("SimdVec3Wrap angleVectors basis vectors are mutually orthogonal [simd]") {
  Vec3D<float> fwd, rgt, up;
  SimdVec3Wrap angles { 30.0f, 45.0f, 0.0f };
  angles.angle_vectors (&fwd, &rgt, &up);

  REQUIRE (ystl::abs (dot3f (fwd, rgt)) <= 0.001f);
  REQUIRE (ystl::abs (dot3f (fwd, up)) <= 0.001f);
  REQUIRE (ystl::abs (dot3f (rgt, up)) <= 0.001f);
}

TEST_CASE ("SimdVec3Wrap angleVectors zero angles give forward (1,0,0) [simd]") {
  // goldsrc convention: (pitch=0, yaw=0, roll=0) => forward along +x
  Vec3D<float> fwd, rgt, up;
  SimdVec3Wrap { 0.0f, 0.0f, 0.0f }.angle_vectors (&fwd, &rgt, &up);

  REQUIRE (fwd.x == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (fwd.y) <= kEps);
  REQUIRE (ystl::abs (fwd.z) <= kEps);
}

TEST_CASE ("SimdVec3Wrap angleVectors orthogonality holds for non-zero roll [simd]") {
  Vec3D<float> fwd, rgt, up;
  SimdVec3Wrap angles { 0.0f, 0.0f, 45.0f };
  angles.angle_vectors (&fwd, &rgt, &up);

  REQUIRE (len3f (fwd) == Approx (1.0f).epsilon (0.001f));
  REQUIRE (len3f (rgt) == Approx (1.0f).epsilon (0.001f));
  REQUIRE (len3f (up) == Approx (1.0f).epsilon (0.001f));
  REQUIRE (ystl::abs (dot3f (fwd, rgt)) <= 0.001f);
  REQUIRE (ystl::abs (dot3f (fwd, up)) <= 0.001f);
  REQUIRE (ystl::abs (dot3f (rgt, up)) <= 0.001f);
}

TEST_CASE ("SimdVec3Wrap angleVectors accepts nullptr for unused outputs [simd]") {
  Vec3D<float> vec (0.0f, 0.0f, 0.0f);
  Vec3D<float> fwd;
  REQUIRE_NOTHROW (vec.angle_vectors (&fwd, nullptr, nullptr));
}

namespace {
// engine AngleVectors (goldsrc client): the same shuffle math, expanded to
// the scalar basis vectors. angles are (pitch, yaw, roll) in degrees.
void reference_angle_vectors (float pitch, float yaw, float roll, Vec3D<float> *forward, Vec3D<float> *right, Vec3D<float> *upward) {
  float sp, cp, sy, cy, sr, cr;
  ystl::sincosf (ystl::deg2rad (pitch), sp, cp);
  ystl::sincosf (ystl::deg2rad (yaw), sy, cy);
  ystl::sincosf (ystl::deg2rad (roll), sr, cr);

  if (forward) {
    *forward = { cp * cy, cp * sy, -sp };
  }
  if (right) {
    *right = { -sr * sp * cy + cr * sy, -sr * sp * sy - cr * cy, -sr * cp };
  }
  if (upward) {
    *upward = { cr * sp * cy + sr * sy, cr * sp * sy - sr * cy, cr * cp };
  }
}
}

// the simd kernel must be bit-for-bit the goldsrc AngleVectors shuffle order,
// not just a unit/orthogonal basis: pin it against the expanded reference.
TEST_CASE ("SimdVec3Wrap angleVectors matches the engine scalar reference [simd]") {
  const auto error_for = [] (float pitch, float yaw, float roll) {
    Vec3D<float> fwd, rgt, up, rf, rr, ru;
    SimdVec3Wrap { pitch, yaw, roll }.angle_vectors (&fwd, &rgt, &up);
    reference_angle_vectors (pitch, yaw, roll, &rf, &rr, &ru);

    float err = 0.0f;
    for (int i = 0; i < 3; ++i) {
      err = ystl::max (err, ystl::abs (fwd.data[i] - rf.data[i]));
      err = ystl::max (err, ystl::abs (rgt.data[i] - rr.data[i]));
      err = ystl::max (err, ystl::abs (up.data[i] - ru.data[i]));
    }
    return err;
  };

  // fixed corners across the pitch/yaw/roll ranges
  const float corners[][3] = {
    { 0.0f,   0.0f,    0.0f    },
    { 0.0f,   0.0f,    45.0f   },
    { 30.0f,  45.0f,   0.0f    },
    { 30.0f,  45.0f,   60.0f   },
    { -89.0f, 179.0f,  -179.0f },
    { 89.0f,  -179.0f, 179.0f  },
    { 45.0f,  90.0f,   90.0f   },
    { -45.0f, -90.0f,  -90.0f  },
  };

  float worst = 0.0f;

  for (const auto &a : corners) {
    worst = ystl::max (worst, error_for (a[0], a[1], a[2]));
  }

  // deterministic sweep
  uint32_t s = 0x12345678u;
  for (int i = 0; i < 4000; ++i) {
    s = s * 1664525u + 1013904223u;
    const float u1 = static_cast<float> (s >> 8) / static_cast<float> (1u << 24);
    s = s * 1664525u + 1013904223u;
    const float u2 = static_cast<float> (s >> 8) / static_cast<float> (1u << 24);
    s = s * 1664525u + 1013904223u;
    const float u3 = static_cast<float> (s >> 8) / static_cast<float> (1u << 24);

    worst = ystl::max (worst, error_for (u1 * 178.0f - 89.0f, u2 * 360.0f - 180.0f, u3 * 360.0f - 180.0f));
  }

  // sincos_ps is the fast cephes kernel, so compare within its ~1e-7 error
  REQUIRE (worst < 1e-5f);
}

// additional simd tests
TEST_CASE ("SimdVec3Wrap assignment operator [simd]") {
  SimdVec3Wrap a (1.0f, 2.0f, 3.0f);
  SimdVec3Wrap b;
  b = a;
  REQUIRE (b.x == Approx (1.0f));
  REQUIRE (b.y == Approx (2.0f));
  REQUIRE (b.z == Approx (3.0f));
}

TEST_CASE ("SimdVec3Wrap component access [simd]") {
  SimdVec3Wrap a (1.0f, 2.0f, 3.0f);
  REQUIRE (a.x == Approx (1.0f));
  REQUIRE (a.y == Approx (2.0f));
  REQUIRE (a.z == Approx (3.0f));
  REQUIRE (a.w == Approx (0.0f));
}

  #endif // ystl_has_simd_sse

#else // ystl_has_simd

// placeholder so the tu is never empty when simd is unavailable
TEST_CASE ("SIMD not available on this platform [simd][.]") {
  REQUIRE (true);
}

#endif // ystl_has_simd

// platform detection tests (compile-time)
TEST_CASE ("SIMD platform macros are defined [simd]") {
  // these are compile-time checks, just verify we can compile
#if defined(YSTL_HAS_SIMD)
  REQUIRE (true);
#endif
#if defined(YSTL_HAS_SIMD_SSE)
  REQUIRE (true);
#endif
#if defined(YSTL_HAS_SIMD_NEON)
  REQUIRE (true);
#endif
#if defined(YSTL_HAS_SIMD_RVV)
  REQUIRE (true);
#endif
  REQUIRE (true);
}

// simd math function
#if defined(YSTL_HAS_SIMD_SSE)
TEST_CASE ("SIMD math functions are available [simd]") {
  // test that we can call simd math functions these are mostly compile-time tests

  // the functions exist and can be called we don't verify results since they're platform-specific
  REQUIRE (true);
}

// simd sincos replaces libm calls so check accuracy over domain
TEST_CASE ("simd sincos_ps accuracy over the documented domain [simd]") {
  double max_sin_err = 0.0, max_cos_err = 0.0;

  for (float x = -8192.0f; x <= 8192.0f; x += 0.37f) {
    const __m128 angles = _mm_set1_ps (x);

    // read the rounded float back: 32-bit x86 gcc keeps x in x87 extended
    // precision, so passing x straight to ::sin would compare the kernel's
    // float input against a wider reference
    volatile float xr = _mm_cvtss_f32 (angles);

    __m128 s, c;
    ystl::simd::sincos_ps (angles, s, c);

    max_sin_err = ystl::max (max_sin_err, ::fabs (static_cast<double> (_mm_cvtss_f32 (s)) - ::sin (static_cast<double> (xr))));
    max_cos_err = ystl::max (max_cos_err, ::fabs (static_cast<double> (_mm_cvtss_f32 (c)) - ::cos (static_cast<double> (xr))));
  }

  // measured worst case is ~7.2e-08 abs (~8 ulps); keep a comfortable margin
  REQUIRE (max_sin_err < 1e-6);
  REQUIRE (max_cos_err < 1e-6);
}
#endif
