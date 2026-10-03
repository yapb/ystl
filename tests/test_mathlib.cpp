// test_mathlib.cpp - tests for ystl/mathlib.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

static constexpr float kEps = 1e-4f;

// constants
TEST_CASE ("Math constants have expected values [mathlib]") {
  REQUIRE (kMathPi == Approx (3.14159265f).epsilon (1e-5f));
  REQUIRE (kDegreeToRadians == Approx (kMathPi / 180.0f).epsilon (1e-7f));
  REQUIRE (kRadiansToDegree == Approx (180.0f / kMathPi).epsilon (1e-5f));
}

// ystl::abs
TEST_CASE ("ystl::abs on float [mathlib]") {
  REQUIRE (ystl::abs (3.0f) == Approx (3.0f));
  REQUIRE (ystl::abs (-3.0f) == Approx (3.0f));
  REQUIRE (ystl::abs (0.0f) == Approx (0.0f));
}

TEST_CASE ("ystl::abs on int [mathlib]") {
  REQUIRE (ystl::abs (5) == 5);
  REQUIRE (ystl::abs (-5) == 5);
  REQUIRE (ystl::abs (0) == 0);
}

// ystl::sqrf
TEST_CASE ("ystl::sqrf squares a value [mathlib]") {
  REQUIRE (ystl::sqrf (3.0f) == Approx (9.0f));
  REQUIRE (ystl::sqrf (0.0f) == Approx (0.0f));
  REQUIRE (ystl::sqrf (-4.0f) == Approx (16.0f));
  REQUIRE (ystl::sqrf (2) == 4);
}

// ystl::sqrtf
TEST_CASE ("ystl::sqrtf computes square root [mathlib]") {
  REQUIRE (ystl::sqrtf (4.0f) == Approx (2.0f).epsilon (kEps));
  REQUIRE (ystl::sqrtf (9.0f) == Approx (3.0f).epsilon (kEps));
  REQUIRE (ystl::sqrtf (0.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::sqrtf (2.0f) == Approx (1.41421356f).epsilon (kEps));
}

// ystl::rsqrtf
TEST_CASE ("ystl::rsqrtf approximates 1/sqrt(x) [mathlib]") {
  REQUIRE (ystl::rsqrtf (4.0f) == Approx (0.5f).epsilon (0.01f));
  REQUIRE (ystl::rsqrtf (9.0f) == Approx (1.0f / 3.0f).epsilon (0.01f));
  REQUIRE (ystl::rsqrtf (1.0f) == Approx (1.0f).epsilon (0.01f));
}

// ystl::sinf / ystl::cosf
TEST_CASE ("ystl::sinf computes sine [mathlib]") {
  REQUIRE (ystl::abs (ystl::sinf (0.0f)) <= 1e-6f);
  REQUIRE (ystl::sinf (kMathPi / 2) == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (ystl::sinf (kMathPi)) <= 1e-6f);
}

TEST_CASE ("ystl::cosf computes cosine [mathlib]") {
  REQUIRE (ystl::cosf (0.0f) == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (ystl::cosf (kMathPi / 2)) <= 1e-6f);
  REQUIRE (ystl::cosf (kMathPi) == Approx (-1.0f).epsilon (kEps));
}

// ystl::tanf
TEST_CASE ("ystl::tanf computes tangent [mathlib]") {
  REQUIRE (ystl::tanf (0.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::tanf (kMathPi / 4) == Approx (1.0f).epsilon (0.001f));
}

// ystl::atan2f
TEST_CASE ("ystl::atan2f computes two-argument arctangent [mathlib]") {
  REQUIRE (ystl::atan2f (0.0f, 1.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::atan2f (1.0f, 0.0f) == Approx (kMathPi / 2).epsilon (kEps));
  REQUIRE (ystl::atan2f (0.0f, -1.0f) == Approx (kMathPi).epsilon (kEps));
}

// ystl::powf
TEST_CASE ("ystl::powf raises x to the power y [mathlib]") {
  REQUIRE (ystl::powf (2.0f, 10.0f) == Approx (1024.0f).epsilon (0.5f));
  REQUIRE (ystl::powf (3.0f, 2.0f) == Approx (9.0f).epsilon (kEps));
  REQUIRE (ystl::powf (1.0f, 100.0f) == Approx (1.0f).epsilon (kEps));
}

// ystl::log10f
TEST_CASE ("ystl::log10f computes base-10 logarithm [mathlib]") {
  REQUIRE (ystl::log10f (1.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::log10f (10.0f) == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::log10f (100.0f) == Approx (2.0f).epsilon (kEps));
}

// ystl::roundf
TEST_CASE ("ystl::roundf rounds to nearest integer [mathlib]") {
  REQUIRE (ystl::roundf (2.4f) == Approx (2.0f));
  REQUIRE (ystl::roundf (2.5f) == Approx (3.0f));
  REQUIRE (ystl::roundf (-2.4f) == Approx (-2.0f));
  REQUIRE (ystl::roundf (-2.5f) == Approx (-3.0f));
}

// ystl::ceilf
TEST_CASE ("ystl::ceilf rounds up to next integer [mathlib]") {
  REQUIRE (ystl::ceilf (2.1f) == Approx (3.0f));
  REQUIRE (ystl::ceilf (-2.9f) == Approx (-2.0f));
  REQUIRE (ystl::ceilf (3.0f) == Approx (3.0f));

  // negative fractional parts the old add-magic used to get wrong
  REQUIRE (ystl::ceilf (-0.6f) == Approx (0.0f));
  REQUIRE (ystl::ceilf (-1.6f) == Approx (-1.0f));
}

// ystl::floorf
TEST_CASE ("ystl::floorf rounds down to previous integer [mathlib]") {
  REQUIRE (ystl::floorf (2.9f) == Approx (2.0f));
  REQUIRE (ystl::floorf (-2.1f) == Approx (-3.0f));
  REQUIRE (ystl::floorf (3.0f) == Approx (3.0f));

  // negative fractional parts the old add-magic used to get wrong
  REQUIRE (ystl::floorf (-0.4f) == Approx (-1.0f));
  REQUIRE (ystl::floorf (-1.4f) == Approx (-2.0f));
  REQUIRE (ystl::floorf (-2.5f) == Approx (-3.0f));
}

// ystl::sincosf
TEST_CASE ("ystl::sincosf produces sin and cos simultaneously [mathlib]") {
  float s {}, c {};
  ystl::sincosf (0.0f, s, c);
  REQUIRE (s == Approx (0.0f).epsilon (kEps));
  REQUIRE (c == Approx (1.0f).epsilon (kEps));

  ystl::sincosf (kMathPi / 2, s, c);
  REQUIRE (s == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (c) <= 1e-6f);
}

// ystl::fzero / ystl::fequal
TEST_CASE ("ystl::fzero returns true near zero [mathlib]") {
  REQUIRE (ystl::fzero (0.0f));
  REQUIRE (ystl::fzero (0.001f));
  REQUIRE_FALSE (ystl::fzero (0.1f));
}

TEST_CASE ("ystl::fequal detects near equality [mathlib]") {
  REQUIRE (ystl::fequal (1.0f, 1.0f));
  REQUIRE (ystl::fequal (1.0f, 1.0005f));
  REQUIRE_FALSE (ystl::fequal (1.0f, 1.01f));
}

// ystl::deg2rad /
TEST_CASE ("ystl::deg2rad converts degrees to radians [mathlib]") {
  REQUIRE (ystl::deg2rad (0.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::deg2rad (180.0f) == Approx (kMathPi).epsilon (kEps));
  REQUIRE (ystl::deg2rad (90.0f) == Approx (kMathPi / 2).epsilon (kEps));
}

TEST_CASE ("ystl::rad2deg converts radians to degrees [mathlib]") {
  REQUIRE (ystl::rad2deg (0.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::rad2deg (kMathPi) == Approx (180.0f).epsilon (kEps));
}

// ystl::wrapangle /
TEST_CASE ("ystl::wrapAngle wraps to [-180, 180) [mathlib]") {
  REQUIRE (ystl::wrap_angle (0.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (180.0f) == Approx (-180.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (-180.0f) == Approx (-180.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (270.0f) == Approx (-90.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (-270.0f) == Approx (90.0f).epsilon (kEps));

  // large negatives used to land on a half-integer floor from the add-magic
  REQUIRE (ystl::wrap_angle (-360.0f) == Approx (0.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (-300.0f) == Approx (60.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (-288.0f) == Approx (72.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (-400.0f) == Approx (-40.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (400.0f) == Approx (40.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle (540.0f) == Approx (-180.0f).epsilon (kEps));
}

TEST_CASE ("ystl::wrapAngle360 wraps angles within one full revolution [mathlib]") {
  REQUIRE (ystl::abs (ystl::wrap_angle360 (0.0f)) <= 1e-4f);
  REQUIRE (ystl::wrap_angle360 (90.0f) == Approx (90.0f).epsilon (kEps));
  REQUIRE (ystl::wrap_angle360 (180.0f) == Approx (180.0f).epsilon (kEps));
  // wrapangle360 uses floor(x/720+0.5), so 360 maps to -360 and 720 maps to 0
  REQUIRE (ystl::abs (ystl::wrap_angle360 (720.0f)) <= 1e-4f);
}

TEST_CASE ("ystl::anglesDifference computes wrapped difference [mathlib]") {
  REQUIRE (ystl::angles_difference (10.0f, 5.0f) == Approx (5.0f).epsilon (kEps));
  REQUIRE (ystl::angles_difference (350.0f, 10.0f) == Approx (-20.0f).epsilon (kEps));
}

// additional tests for missing coverage

// ystl::abs for double type
TEST_CASE ("ystl::abs on double [mathlib]") {
  REQUIRE (ystl::abs (3.0) == Approx (3.0));
  REQUIRE (ystl::abs (-3.0) == Approx (3.0));
  REQUIRE (ystl::abs (0.0) == Approx (0.0));
}

// ystl::sqrf for double
TEST_CASE ("ystl::sqrf on double [mathlib]") {
  REQUIRE (ystl::sqrf (3.0) == Approx (9.0));
  REQUIRE (ystl::sqrf (0.0) == Approx (0.0));
  REQUIRE (ystl::sqrf (-4.0) == Approx (16.0));
}

// ystl::min / ystl::max /
TEST_CASE ("min returns the smaller value [mathlib]") {
  REQUIRE (ystl::min (1, 2) == 1);
  REQUIRE (ystl::min (2, 1) == 1);
  REQUIRE (ystl::min (-5, 5) == -5);
  REQUIRE (ystl::min (7, 7) == 7);
  REQUIRE (ystl::min (1.5f, 2.5f) == 1.5f);
  REQUIRE (ystl::min (0.0f, -1.0f) == -1.0f);
}

TEST_CASE ("max returns the larger value [mathlib]") {
  REQUIRE (ystl::max (1, 2) == 2);
  REQUIRE (ystl::max (2, 1) == 2);
  REQUIRE (ystl::max (-5, 5) == 5);
  REQUIRE (ystl::max (7, 7) == 7);
  REQUIRE (ystl::max (1.5f, 2.5f) == 2.5f);
  REQUIRE (ystl::max (0.0f, -1.0f) == 0.0f);
}

TEST_CASE ("clamp constrains value to [a, b] [mathlib]") {
  REQUIRE (ystl::clamp (5, 0, 10) == 5);
  REQUIRE (ystl::clamp (-1, 0, 10) == 0);
  REQUIRE (ystl::clamp (15, 0, 10) == 10);
  REQUIRE (ystl::clamp (0, 0, 10) == 0);
  REQUIRE (ystl::clamp (10, 0, 10) == 10);

  REQUIRE (ystl::clamp (0.5f, 0.0f, 1.0f) == 0.5f);
  REQUIRE (ystl::clamp (-0.5f, 0.0f, 1.0f) == 0.0f);
  REQUIRE (ystl::clamp (1.5f, 0.0f, 1.0f) == 1.0f);
}

// sse specializations
#if defined(YSTL_HAS_SIMD_SSE)
TEST_CASE ("ystl::min SSE specialization [mathlib]") {
  float a = 3.0f, b = 5.0f;
  REQUIRE (ystl::min (a, b) == Approx (3.0f));
  REQUIRE (ystl::min (b, a) == Approx (3.0f));
  REQUIRE (ystl::min (a, a) == Approx (3.0f));
}

TEST_CASE ("ystl::max SSE specialization [mathlib]") {
  float a = 3.0f, b = 5.0f;
  REQUIRE (ystl::max (a, b) == Approx (5.0f));
  REQUIRE (ystl::max (b, a) == Approx (5.0f));
  REQUIRE (ystl::max (a, a) == Approx (3.0f));
}

TEST_CASE ("ystl::clamp SSE specialization [mathlib]") {
  REQUIRE (ystl::clamp (2.0f, 0.0f, 1.0f) == Approx (1.0f));
  REQUIRE (ystl::clamp (-1.0f, 0.0f, 1.0f) == Approx (0.0f));
  REQUIRE (ystl::clamp (0.5f, 0.0f, 1.0f) == Approx (0.5f));
  REQUIRE (ystl::clamp (0.0f, 0.0f, 1.0f) == Approx (0.0f));
  REQUIRE (ystl::clamp (1.0f, 0.0f, 1.0f) == Approx (1.0f));
}
#endif

// platform-specific
TEST_CASE ("Platform-specific math functions produce correct results [mathlib]") {
  // platform math must match standard results

  float angle = kMathPi / 4.0f; // 45 degrees

  // sin/cos should be consistent
  float s = ystl::sinf (angle);
  float c = ystl::cosf (angle);
  REQUIRE (s == Approx (0.70710678f).epsilon (kEps));
  REQUIRE (c == Approx (0.70710678f).epsilon (kEps));

  // tan = sin/cos
  float t = ystl::tanf (angle);
  REQUIRE (t == Approx (1.0f).epsilon (0.001f));

  // atan2(1,1) = 45 degrees
  float a = ystl::atan2f (1.0f, 1.0f);
  REQUIRE (a == Approx (kMathPi / 4.0f).epsilon (kEps));

  // sqrt and rsqrt relationship
  float x = 4.0f;
  float sqrt_x = ystl::sqrtf (x);
  float rsqrt_x = ystl::rsqrtf (x);
  REQUIRE (sqrt_x == Approx (2.0f).epsilon (kEps));
  REQUIRE (rsqrt_x * sqrt_x == Approx (1.0f).epsilon (0.01f));
}

// edge cases for math
TEST_CASE ("Math functions handle edge cases [mathlib]") {
  // very small values
  REQUIRE (ystl::sqrtf (1e-10f) == Approx (1e-5f).epsilon (0.01f));
  REQUIRE (ystl::sinf (1e-6f) == Approx (1e-6f).epsilon (0.01f));

  // very large values (wrap for trig functions)
  REQUIRE (ystl::abs (ystl::sinf (1000.0f * kMathPi)) <= 1e-3f);

  // powf with exponent 0
  REQUIRE (ystl::powf (2.0f, 0.0f) == Approx (1.0f).epsilon (kEps));

  // powf with base 1
  REQUIRE (ystl::powf (1.0f, 100.0f) == Approx (1.0f).epsilon (kEps));
}
