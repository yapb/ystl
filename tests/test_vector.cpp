// test_vector.cpp - tests for ystl/vector.h (vec3d / vector)
#include <ystl/ystl.h>
#include <ystl/test.h>

#include <cmath>

using namespace ystl;

static constexpr float kEps = 1e-3f;

// construction
TEST_CASE ("Vector default construction is zero [vector]") {
  Vector v;
  REQUIRE (v.x == Approx (0.0f));
  REQUIRE (v.y == Approx (0.0f));
  REQUIRE (v.z == Approx (0.0f));
}

TEST_CASE ("Vector scalar construction fills all components [vector]") {
  Vector v (5.0f);
  REQUIRE (v.x == Approx (5.0f));
  REQUIRE (v.y == Approx (5.0f));
  REQUIRE (v.z == Approx (5.0f));
}

TEST_CASE ("Vector component construction [vector]") {
  Vector v (1.0f, 2.0f, 3.0f);
  REQUIRE (v.x == Approx (1.0f));
  REQUIRE (v.y == Approx (2.0f));
  REQUIRE (v.z == Approx (3.0f));
}

TEST_CASE ("Vector from pointer [vector]") {
  float arr[3] = { 4.0f, 5.0f, 6.0f };
  Vector v (arr);
  REQUIRE (v.x == Approx (4.0f));
  REQUIRE (v.y == Approx (5.0f));
  REQUIRE (v.z == Approx (6.0f));
}

TEST_CASE ("Vector nullptr construction clears [vector]") {
  Vector v (1.0f, 2.0f, 3.0f);
  v = nullptr;
  REQUIRE (v.x == Approx (0.0f));
  REQUIRE (v.y == Approx (0.0f));
  REQUIRE (v.z == Approx (0.0f));
}

// array / subscript
TEST_CASE ("Vector data array and operator[] are consistent [vector]") {
  Vector v (10.0f, 20.0f, 30.0f);
  REQUIRE (v[0] == Approx (10.0f));
  REQUIRE (v[1] == Approx (20.0f));
  REQUIRE (v[2] == Approx (30.0f));
  REQUIRE (v.data[0] == Approx (10.0f));
}

// arithmetic operators
TEST_CASE ("Vector operator+ adds component-wise [vector]") {
  Vector a (1, 2, 3), b (4, 5, 6);
  Vector c = a + b;
  REQUIRE (c.x == Approx (5.0f));
  REQUIRE (c.y == Approx (7.0f));
  REQUIRE (c.z == Approx (9.0f));
}

TEST_CASE ("Vector operator- subtracts component-wise [vector]") {
  Vector a (5, 7, 9), b (1, 2, 3);
  Vector c = a - b;
  REQUIRE (c.x == Approx (4.0f));
  REQUIRE (c.y == Approx (5.0f));
  REQUIRE (c.z == Approx (6.0f));
}

TEST_CASE ("Vector unary minus negates components [vector]") {
  Vector v (1, -2, 3);
  Vector neg = -v;
  REQUIRE (neg.x == Approx (-1.0f));
  REQUIRE (neg.y == Approx (2.0f));
  REQUIRE (neg.z == Approx (-3.0f));
}

TEST_CASE ("Vector scalar multiplication [vector]") {
  Vector v (1, 2, 3);
  Vector scaled = v * 2.0f;
  REQUIRE (scaled.x == Approx (2.0f));
  REQUIRE (scaled.y == Approx (4.0f));
  REQUIRE (scaled.z == Approx (6.0f));

  Vector scaled2 = 3.0f * v;
  REQUIRE (scaled2.x == Approx (3.0f));
}

TEST_CASE ("Vector scalar division [vector]") {
  Vector v (4, 6, 8);
  Vector d = v / 2.0f;
  // exact: the divisor must not be perturbed by any hidden epsilon
  REQUIRE (d.x == 2.0f);
  REQUIRE (d.y == 3.0f);
  REQUIRE (d.z == 4.0f);
}

TEST_CASE ("Vector division by zero follows IEEE semantics [vector]") {
  Vector v (1, 0, -1);
  Vector d = v / 0.0f;

  REQUIRE (std::isinf (d.x)); // 1 / 0 -> +inf
  REQUIRE (std::isnan (d.y)); // 0 * inf -> nan
  REQUIRE (std::isinf (d.z)); // -1 / 0 -> -inf
}

// compound assignment
TEST_CASE ("Vector operator+= accumulates component-wise [vector]") {
  Vector v (1, 2, 3);
  v += Vector (10, 20, 30);
  REQUIRE (v.x == Approx (11.0f));
  REQUIRE (v.y == Approx (22.0f));
  REQUIRE (v.z == Approx (33.0f));
}

TEST_CASE ("Vector operator-= subtracts component-wise [vector]") {
  Vector v (10, 20, 30);
  v -= Vector (1, 2, 3);
  REQUIRE (v.x == Approx (9.0f));
  REQUIRE (v.y == Approx (18.0f));
  REQUIRE (v.z == Approx (27.0f));
}

TEST_CASE ("Vector operator*= scales component-wise [vector]") {
  Vector v (2, 4, 6);
  v *= 0.5f;
  REQUIRE (v.x == Approx (1.0f));
  REQUIRE (v.y == Approx (2.0f));
  REQUIRE (v.z == Approx (3.0f));
}

// dot product
TEST_CASE ("Vector dot product (operator|) [vector]") {
  Vector a (1, 0, 0), b (0, 1, 0);
  REQUIRE ((a | b) == Approx (0.0f)); // perpendicular

  Vector c (1, 2, 3), d (4, 5, 6);
  REQUIRE ((c | d) == Approx (32.0f)); // 1*4 + 2*5 + 3*6
}

// cross product
TEST_CASE ("Vector cross product (operator^) [vector]") {
  Vector x (1, 0, 0), y (0, 1, 0);
  Vector z = x ^ y;
  REQUIRE (z.x == Approx (0.0f).epsilon (kEps));
  REQUIRE (z.y == Approx (0.0f).epsilon (kEps));
  REQUIRE (z.z == Approx (1.0f).epsilon (kEps));
}

// equality
TEST_CASE ("Vector operator== uses epsilon comparison [vector]") {
  Vector a (1.0f, 2.0f, 3.0f);
  Vector b (1.0f, 2.0f, 3.0f);
  Vector c (1.0f, 2.0f, 4.0f);
  REQUIRE (a == b);
  REQUIRE_FALSE (a == c);
  REQUIRE (a != c);
}

TEST_CASE ("Vector nullptr comparison means zero vector [vector]") {
  Vector zero;
  Vector nonzero (1.0f, 2.0f, 3.0f);

  // used to be ambiguous (member operator== vs builtin pointer comparison)
  REQUIRE (zero == nullptr);
  REQUIRE_FALSE (zero != nullptr);
  REQUIRE (nonzero != nullptr);
  REQUIRE_FALSE (nonzero == nullptr);
}

TEST_CASE ("Vector dot/cross aliases match operators [vector]") {
  Vector a (1, 2, 3), b (4, 5, 6);
  REQUIRE (a.dot (b) == Approx (a | b));
  REQUIRE ((a.cross (b) == (a ^ b)));
}

// length / lengthsq
TEST_CASE ("Vector length computes Euclidean length [vector]") {
  Vector v (3.0f, 4.0f, 0.0f);
  REQUIRE (v.length () == Approx (5.0f).epsilon (kEps));

  Vector unit (1, 0, 0);
  REQUIRE (unit.length () == Approx (1.0f).epsilon (kEps));
}

TEST_CASE ("Vector lengthSq computes squared length [vector]") {
  Vector v (1, 2, 2);
  REQUIRE (v.length_sq () == Approx (9.0f).epsilon (kEps));
}

TEST_CASE ("Vector length2d computes XY length [vector]") {
  Vector v (3.0f, 4.0f, 100.0f);
  REQUIRE (v.length2d () == Approx (5.0f).epsilon (kEps));
}

TEST_CASE ("Vector lengthSq2d computes XY squared length [vector]") {
  Vector v (3.0f, 4.0f, 100.0f);
  REQUIRE (v.length_sq2d () == Approx (25.0f).epsilon (kEps));
}

// distance
TEST_CASE ("Vector distance between two points [vector]") {
  Vector a (0, 0, 0), b (3, 4, 0);
  REQUIRE (a.distance (b) == Approx (5.0f).epsilon (kEps));
}

TEST_CASE ("Vector distanceSq between two points [vector]") {
  Vector a (0, 0, 0), b (3, 4, 0);
  REQUIRE (a.distance_sq (b) == Approx (25.0f).epsilon (kEps));
}

TEST_CASE ("Vector distance2d ignores Z component [vector]") {
  Vector a (0, 0, 0), b (3, 4, 100);
  REQUIRE (a.distance2d (b) == Approx (5.0f).epsilon (kEps));
}

// normalize
TEST_CASE ("Vector normalize produces unit vector [vector]") {
  Vector v (3.0f, 4.0f, 0.0f);
  Vector n = v.normalize ();
  REQUIRE (n.length () == Approx (1.0f).epsilon (kEps));
  REQUIRE (n.x == Approx (0.6f).epsilon (kEps));
  REQUIRE (n.y == Approx (0.8f).epsilon (kEps));
}

TEST_CASE ("Vector normalize2d normalises XY plane only [vector]") {
  Vector v (3.0f, 4.0f, 10.0f);
  Vector n = v.normalize2d ();
  REQUIRE (n.length2d () == Approx (1.0f).epsilon (kEps));
  REQUIRE (n.z == Approx (0.0f));
}

TEST_CASE ("Vector normalizeInPlace modifies in-place and returns length [vector]") {
  Vector v (0, 5, 0);
  float len = v.normalize_in_place ();
  REQUIRE (len == Approx (5.0f).epsilon (kEps));
  REQUIRE (v.length () == Approx (1.0f).epsilon (kEps));
}

TEST_CASE ("Vector normalize of zero vector is deterministic on every path [vector]") {
  // scalar and simd (sse/neon/rvv) paths must agree; pins {0, 0, eps}
  Vector n = Vector (0.0f, 0.0f, 0.0f).normalize ();
  REQUIRE (n.x == Approx (0.0f));
  REQUIRE (n.y == Approx (0.0f));
  REQUIRE (n.z == Approx (ystl::kFloatEpsilon));

  Vector n2d = Vector (0.0f, 0.0f, 5.0f).normalize2d ();
  REQUIRE (n2d.x == Approx (0.0f));
  REQUIRE (n2d.y == Approx (ystl::kFloatEpsilon));
  REQUIRE (n2d.z == Approx (0.0f));
}

// empty / clear
TEST_CASE ("Vector empty detects near-zero vector [vector]") {
  Vector zero;
  REQUIRE (zero.empty ());

  Vector nonzero (1, 0, 0);
  REQUIRE_FALSE (nonzero.empty ());
}

TEST_CASE ("Vector clear zeroes all components [vector]") {
  Vector v (1, 2, 3);
  v.clear ();
  REQUIRE (v.x == Approx (0.0f));
  REQUIRE (v.y == Approx (0.0f));
  REQUIRE (v.z == Approx (0.0f));
  REQUIRE (v.empty ());
}

// get2d
TEST_CASE ("Vector get2d zeroes the Z component [vector]") {
  Vector v (1, 2, 3);
  Vector v2d = v.get2d ();
  REQUIRE (v2d.x == Approx (1.0f));
  REQUIRE (v2d.y == Approx (2.0f));
  REQUIRE (v2d.z == Approx (0.0f));
}

// bboxintersects
TEST_CASE ("Vector bboxIntersects detects overlapping bboxes [vector]") {
  Vector min1 (0, 0, 0), max1 (2, 2, 2);
  Vector min2 (1, 1, 1), max2 (3, 3, 3);
  REQUIRE (Vector::bbox_intersects (min1, max1, min2, max2));
}

TEST_CASE ("Vector bboxIntersects returns false for non-overlapping boxes [vector]") {
  Vector min1 (0, 0, 0), max1 (1, 1, 1);
  Vector min2 (5, 5, 5), max2 (6, 6, 6);
  REQUIRE_FALSE (Vector::bbox_intersects (min1, max1, min2, max2));
}

// bboxcontainspoint
TEST_CASE ("Vector bboxContainsPoint detects inside point [vector]") {
  Vector mins (0, 0, 0), maxs (2, 2, 2);
  REQUIRE (Vector::bbox_contains_point (Vector (1, 1, 1), mins, maxs));
}

TEST_CASE ("Vector bboxContainsPoint counts faces as inside [vector]") {
  Vector mins (0, 0, 0), maxs (2, 2, 2);
  REQUIRE (Vector::bbox_contains_point (Vector (0, 0, 0), mins, maxs));
  REQUIRE (Vector::bbox_contains_point (Vector (2, 2, 2), mins, maxs));
}

TEST_CASE ("Vector bboxContainsPoint returns false for outside point [vector]") {
  Vector mins (0, 0, 0), maxs (2, 2, 2);
  REQUIRE_FALSE (Vector::bbox_contains_point (Vector (3, 1, 1), mins, maxs));
  REQUIRE_FALSE (Vector::bbox_contains_point (Vector (1, 1, 3), mins, maxs));
}

TEST_CASE ("Vector bboxContainsPoint2D ignores z [vector]") {
  Vector mins (0, 0, 100), maxs (2, 2, 200);
  REQUIRE (Vector::bbox_contains_point2_d (Vector (1, 1, -50), mins, maxs));
  REQUIRE_FALSE (Vector::bbox_contains_point2_d (Vector (3, 1, 150), mins, maxs));
}

// angles / yaw / pitch
TEST_CASE ("Vector yaw returns zero for unit X vector [vector]") {
  Vector v (1.0f, 0.0f, 0.0f);
  REQUIRE (v.yaw () == Approx (0.0f).epsilon (kEps));
}

TEST_CASE ("Vector yaw returns 90 for unit Y vector [vector]") {
  Vector v (0.0f, 1.0f, 0.0f);
  REQUIRE (v.yaw () == Approx (90.0f).epsilon (kEps));
}

// clampangles
TEST_CASE ("Vector clampAngles wraps X and Y and zeroes Z [vector]") {
  Vector v (200.0f, -200.0f, 45.0f);
  v.clamp_angles ();
  REQUIRE (v.z == Approx (0.0f));
  // x and y should be wrapped into [-180, 180)
  REQUIRE (v.x >= -180.0f);
  REQUIRE (v.x < 180.0f);
  REQUIRE (v.y >= -180.0f);
  REQUIRE (v.y < 180.0f);
}

// additional tests for missing coverage

// vector compound
TEST_CASE ("Vector operator/= divides components by scalar [vector]") {
  Vector v (4.0f, 6.0f, 8.0f);
  v /= 2.0f;
  // exact: the divisor must not be perturbed by any hidden epsilon
  REQUIRE (v.x == 2.0f);
  REQUIRE (v.y == 3.0f);
  REQUIRE (v.z == 4.0f);
}

// vector 2d operations
TEST_CASE ("Vector distanceSq2d computes 2D squared distance [vector]") {
  Vector a (1.0f, 2.0f, 10.0f);
  Vector b (4.0f, 6.0f, 20.0f); // z component should be ignored
  float dist2 = a.distance_sq2d (b);
  // (4-1)^2 + (6-2)^2 = 9 + 16 = 25
  REQUIRE (dist2 == Approx (25.0f));
}

// vector angle-related
TEST_CASE ("Vector pitch computes pitch angle [vector]") {
  Vector v (0.0f, 0.0f, 1.0f); // straight up
  float pitch = v.pitch ();
  REQUIRE (pitch == Approx (90.0f).epsilon (kEps));

  Vector v2 (1.0f, 0.0f, 0.0f); // horizontal
  float pitch2 = v2.pitch ();
  REQUIRE (pitch2 == Approx (0.0f).epsilon (kEps));
}

TEST_CASE ("Vector angles returns pitch and yaw [vector]") {
  Vector v (1.0f, 0.0f, 0.0f);
  Vector angles = v.angles ();
  REQUIRE (angles.x == Approx (0.0f).epsilon (kEps)); // pitch
  REQUIRE (angles.y == Approx (0.0f).epsilon (kEps)); // yaw
  REQUIRE (angles.z == Approx (0.0f).epsilon (kEps));

  Vector v2 (0.0f, 0.0f, 1.0f);
  Vector angles2 = v2.angles ();
  REQUIRE (angles2.x == Approx (90.0f).epsilon (kEps)); // pitch up
}

TEST_CASE ("Vector angleVectors computes forward/right/up vectors [vector]") {
  Vector angles (0.0f, 0.0f, 0.0f); // looking along +x
  Vector forward, right, upward;

  angles.angle_vectors (&forward, &right, &upward);

  // forward should be +x
  REQUIRE (forward.x == Approx (1.0f).epsilon (kEps));
  REQUIRE (forward.y == Approx (0.0f).epsilon (kEps));
  REQUIRE (forward.z == Approx (0.0f).epsilon (kEps));

  // right should be -y (right-handed coordinate system)
  REQUIRE (right.x == Approx (0.0f).epsilon (kEps));
  REQUIRE (right.y == Approx (-1.0f).epsilon (kEps));
  REQUIRE (right.z == Approx (0.0f).epsilon (kEps));

  // up should be +z
  REQUIRE (upward.x == Approx (0.0f).epsilon (kEps));
  REQUIRE (upward.y == Approx (0.0f).epsilon (kEps));
  REQUIRE (upward.z == Approx (1.0f).epsilon (kEps));
}

TEST_CASE ("Vector forward/upward/right return basis vectors [vector]") {
  Vector angles (0.0f, 90.0f, 0.0f); // looking along +y
  Vector forward = angles.forward ();
  Vector upward = angles.upward ();
  Vector right = angles.right ();

  // forward should be +y (looking east)
  REQUIRE (ystl::abs (forward.x) <= 1e-5f);
  REQUIRE (forward.y == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (forward.z) <= 1e-5f);

  // right should be +x (south in right-handed system)
  REQUIRE (right.x == Approx (1.0f).epsilon (kEps));
  REQUIRE (ystl::abs (right.y) <= 1e-5f);
  REQUIRE (ystl::abs (right.z) <= 1e-5f);

  // up should be +z (unchanged by yaw)
  REQUIRE (ystl::abs (upward.x) <= 1e-5f);
  REQUIRE (ystl::abs (upward.y) <= 1e-5f);
  REQUIRE (upward.z == Approx (1.0f).epsilon (kEps));
}

// vector pointer
TEST_CASE ("Vector pointer conversion operators [vector]") {
  Vector v (1.0f, 2.0f, 3.0f);

  // const pointer conversion
  const Vector &cv = v;
  const float *cptr = cv;
  REQUIRE (cptr[0] == Approx (1.0f));
  REQUIRE (cptr[1] == Approx (2.0f));
  REQUIRE (cptr[2] == Approx (3.0f));

  // non-const pointer conversion
  float *ptr = v;
  ptr[1] = 5.0f;
  REQUIRE (v.y == Approx (5.0f));
}

// vector yaw with
TEST_CASE ("Vector yaw with 45 degree angle [vector]") {
  Vector v (1.0f, 1.0f, 0.0f); // 45 degrees in xy plane
  float yaw = v.yaw ();
  REQUIRE (yaw == Approx (45.0f).epsilon (kEps));
}

TEST_CASE ("Vector yaw with negative angle [vector]") {
  Vector v (1.0f, -1.0f, 0.0f); // -45 degrees in xy plane
  float yaw = v.yaw ();
  REQUIRE (yaw == Approx (-45.0f).epsilon (kEps));
}
