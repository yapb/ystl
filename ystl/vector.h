// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/endian.h>
#include <ystl/mathlib.h>
#include <ystl/simd.h>
#include <ystl/traits.h>

namespace ystl {

// 3dmath vector
template <typename T> class Vec3D {
  // scalar kernels are float only, so other types lose simd acceleration
  static_assert (ystl::is_floating_point_v<T>, "Vec3D is intended for floating point types");

public:
  YSTL_DISABLE_ANONYMOUS_UNION_WARNING
  union {
    struct {
      T x, y, z;
    };
    T data[3] {};
  };
  YSTL_RESTORE_ANONYMOUS_UNION_WARNING

public:
  constexpr Vec3D () : x {}, y {}, z {} {}

  constexpr Vec3D (const T &scalar) : x (scalar), y (scalar), z (scalar) {}

  constexpr Vec3D (const T &x_, const T &y_, const T &z_) : x (x_), y (y_), z (z_) {}

  // implicit by design for hlsdk call sites and engine callbacks
  constexpr Vec3D (const T *rhs) : x (rhs[0]), y (rhs[1]), z (rhs[2]) {}

#if defined(YSTL_HAS_SIMD)
  constexpr Vec3D (const SimdVec3Wrap &rhs) : x (rhs.x), y (rhs.y), z (rhs.z) {}
#endif

  constexpr Vec3D (const Vec3D &) = default;

  constexpr Vec3D (nullptr_t) : x {}, y {}, z {} {}

public:
  // implicit by design, same hlsdk and engine callback rationale
  constexpr operator T *() {
    return data;
  }

  constexpr operator const T *() const {
    return data;
  }

  [[nodiscard]] constexpr Vec3D operator+ (const Vec3D &rhs) const {
    return { x + rhs.x, y + rhs.y, z + rhs.z };
  }

  [[nodiscard]] constexpr Vec3D operator- (const Vec3D &rhs) const {
    return { x - rhs.x, y - rhs.y, z - rhs.z };
  }

  [[nodiscard]] constexpr Vec3D operator- () const {
    return { -x, -y, -z };
  }

  [[nodiscard]] friend constexpr Vec3D operator* (const T &scale, const Vec3D &rhs) {
    return { rhs.x * scale, rhs.y * scale, rhs.z * scale };
  }

  [[nodiscard]] constexpr Vec3D operator* (const T &scale) const {
    return { scale * x, scale * y, scale * z };
  }

  [[nodiscard]] constexpr Vec3D operator/ (const T &rhs) const {
    // reciprocal multiply trades one rounding step for speed
    const auto inv = T { 1 } / rhs;
    return { inv * x, inv * y, inv * z };
  }

  // cross product, prefer cross as operators bind looser than compare
  [[nodiscard]] constexpr Vec3D operator^ (const Vec3D &rhs) const {
    return { y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x };
  }

  // dot product, same precedence caveat as operator^; prefer dot()
  [[nodiscard]] constexpr T operator| (const Vec3D &rhs) const {
    return x * rhs.x + y * rhs.y + z * rhs.z;
  }

  // named cross and dot aliases avoid the operator precedence trap
  [[nodiscard]] constexpr Vec3D cross (const Vec3D &rhs) const {
    return operator^ (rhs);
  }

  [[nodiscard]] constexpr T dot (const Vec3D &rhs) const {
    return operator| (rhs);
  }

  constexpr Vec3D &operator+= (const Vec3D &rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;

    return *this;
  }

  constexpr Vec3D &operator-= (const Vec3D &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;

    return *this;
  }

  constexpr Vec3D &operator*= (const T &rhs) {
    x *= rhs;
    y *= rhs;
    z *= rhs;

    return *this;
  }

  constexpr Vec3D &operator/= (const T &rhs) {
    // reciprocal-multiply, same precision caveat as operator/
    const auto inv = T { 1 } / rhs;

    x *= inv;
    y *= inv;
    z *= inv;

    return *this;
  }

  // fuzzy compare via fequal, handy but never use for ordering
  [[nodiscard]] constexpr bool operator== (const Vec3D &rhs) const {
    return ystl::fequal (x, rhs.x) && ystl::fequal (y, rhs.y) && ystl::fequal (z, rhs.z);
  }

  [[nodiscard]] constexpr bool operator!= (const Vec3D &rhs) const {
    return !operator== (rhs);
  }

  // nullptr comparison: without these, v == nullptr is ambiguous
  [[nodiscard]] constexpr bool operator== (nullptr_t) const {
    return empty ();
  }

  [[nodiscard]] constexpr bool operator!= (nullptr_t) const {
    return !empty ();
  }

  constexpr const T &operator[] (const int i) const {
    return data[i];
  }

  constexpr T &operator[] (const int i) {
    return data[i];
  }

  constexpr void operator= (nullptr_t) {
    clear ();
  }

  constexpr Vec3D &operator= (const Vec3D &) = default;

public:
  [[nodiscard]] T length () const {
    // scalar dot beats simd on all compilers, so skip the wrapper trip
    return ystl::sqrtf (length_sq ());
  }

  [[nodiscard]] constexpr T length_sq () const {
    return ystl::sqrf (x) + ystl::sqrf (y) + ystl::sqrf (z);
  }

  [[nodiscard]] T length2d () const {
    return ystl::sqrtf (length_sq2d ());
  }

  [[nodiscard]] constexpr T length_sq2d () const {
    return ystl::sqrf (x) + ystl::sqrf (y);
  }

  [[nodiscard]] T distance (const Vec3D &rhs) const {
    return (*this - rhs).length ();
  }

  [[nodiscard]] T distance2d (const Vec3D &rhs) const {
    return (*this - rhs).length2d ();
  }

  [[nodiscard]] T distance_sq (const Vec3D &rhs) const {
    return (*this - rhs).length_sq ();
  }

  [[nodiscard]] T distance_sq2d (const Vec3D &rhs) const {
    return (*this - rhs).length_sq2d ();
  }

  [[nodiscard]] constexpr Vec3D get2d () const {
    return { x, y, T {} };
  }

  [[nodiscard]] Vec3D normalize () const {
#if defined(YSTL_HAS_SIMD)
    // guard zero on the scalar path, as simd kernels disagree on it
    if (empty ()) {
      return { T {}, T {}, static_cast<T> (kFloatEpsilon) };
    }
    return SimdVec3Wrap { x, y, z }.normalize ();
#else
    const auto len = length ();

    if (ystl::fzero (len)) {
      return { T {}, T {}, static_cast<T> (kFloatEpsilon) };
    }
    const auto inv = T { 1 } / len;
    return { x * inv, y * inv, z * inv };
#endif
  }

  [[nodiscard]] Vec3D normalize2d () const {
    const auto len = length2d ();

    if (ystl::fzero (len)) {
      return { T {}, static_cast<T> (kFloatEpsilon), T {} };
    }
    const auto inv = T { 1 } / len;
    return { x * inv, y * inv, T {} };
  }

  T normalize_in_place () {
    const auto len = length ();

    if (ystl::fzero (len)) {
      *this = { T {}, T {}, static_cast<T> (kFloatEpsilon) };
    }
    else {
      const auto mul = T { 1 } / len;
      *this = { x * mul, y * mul, z * mul };
    }
    return len;
  }

  [[nodiscard]] constexpr bool empty () const {
    return ystl::fzero (x) && ystl::fzero (y) && ystl::fzero (z);
  }

  constexpr void clear () {
    x = y = z = T {};
  }

  Vec3D &clamp_angles () {
    x = ystl::wrap_angle (x);
    y = ystl::wrap_angle (y);
    z = T {};

    return *this;
  }

  // converts a spatial location determined by the vector passed into an absolute x angle (pitch) from the origin of the world
  [[nodiscard]] T pitch () const {
    if (ystl::fzero (z)) {
      return T {};
    }
    return ystl::rad2deg (ystl::atan2f (z, length2d ()));
  }

  // converts a spatial location determined by the vector passed into an absolute y angle (yaw) from the origin of the world
  [[nodiscard]] T yaw () const {
    if (ystl::fzero (x) && ystl::fzero (y)) {
      return T {};
    }
    return ystl::rad2deg (ystl::atan2f (y, x));
  }

  // converts a spatial location determined by the vector passed in into constant absolute angles from the origin of the world
  [[nodiscard]] Vec3D angles () const {
    if (ystl::fzero (x) && ystl::fzero (y)) {
      return { z > T {} ? T { 90 } : T { 270 }, T {}, T {} };
    }
    return { ystl::rad2deg (ystl::atan2f (z, length2d ())), ystl::rad2deg (ystl::atan2f (y, x)), T {} };
  }

  // build forward, right and up vectors from a view angle
  void angle_vectors (Vec3D *forward, Vec3D *right, Vec3D *upward) const {
#if defined(YSTL_HAS_SIMD_SSE)
    SimdVec3Wrap { x, y, z }.angle_vectors<Vec3D> (forward, right, upward);
#elif defined(YSTL_HAS_SIMD_NEON) || defined(YSTL_HAS_SIMD_RVV)
    SimdVec3Wrap s, c;
    SimdVec3Wrap { x, y, z }.angle_vectors (s, c);

    build_basis (s, c, forward, right, upward);
#else
    Vec3D s, c;
    const Vec3D r { ystl::deg2rad (x), ystl::deg2rad (y), ystl::deg2rad (z) };

    ystl::sincosf (r.x, s.x, c.x);
    ystl::sincosf (r.y, s.y, c.y);

    // roll terms feed right/upward only; skip them when just forward is needed
    if (right || upward) {
      ystl::sincosf (r.z, s.z, c.z);
    }
    build_basis (s, c, forward, right, upward);
#endif
  }

  [[nodiscard]] Vec3D forward () const {
    Vec3D fwd {};
    angle_vectors (&fwd, nullptr, nullptr);
    return fwd;
  }

  [[nodiscard]] Vec3D upward () const {
    Vec3D up {};
    angle_vectors (nullptr, nullptr, &up);
    return up;
  }

  [[nodiscard]] Vec3D right () const {
    Vec3D rt {};
    angle_vectors (nullptr, &rt, nullptr);
    return rt;
  }

public:
  [[nodiscard]] static constexpr bool bbox_intersects (const Vec3D &min1, const Vec3D &max1, const Vec3D &min2, const Vec3D &max2) {
    return min1.x < max2.x && max1.x > min2.x && min1.y < max2.y && max1.y > min2.y && min1.z < max2.z && max1.z > min2.z;
  }

  // inclusive point-in-box, faces count as inside (unlike the strict bboxIntersects above)
  [[nodiscard]] static constexpr bool bbox_contains_point (const Vec3D &point, const Vec3D &mins, const Vec3D &maxs) {
    return point.x >= mins.x && point.x <= maxs.x && point.y >= mins.y && point.y <= maxs.y && point.z >= mins.z && point.z <= maxs.z;
  }

  // same, but xy footprint only (z ignored, e.g. goal-zone brushes vs node origins)
  [[nodiscard]] static constexpr bool bbox_contains_point2_d (const Vec3D &point, const Vec3D &mins, const Vec3D &maxs) {
    return point.x >= mins.x && point.x <= maxs.x && point.y >= mins.y && point.y <= maxs.y;
  }

private:
  // shared basis builder for the scalar and simd angle paths
  template <typename S> static void build_basis (const S &s, const S &c, Vec3D *forward, Vec3D *right, Vec3D *upward) {
    if (forward) {
      *forward = { c.x * c.y, c.x * s.y, -s.x };
    }

    if (right) {
      *right = { -s.z * s.x * c.y + c.z * s.y, -s.z * s.x * s.y - c.z * c.y, -s.z * c.x };
    }

    if (upward) {
      *upward = { c.z * s.x * c.y + s.z * s.y, c.z * s.x * s.y - s.z * c.y, c.z * c.x };
    }
  }
};

// default is float
using Vector = Vec3D<float>;

// little-endian descriptor, the union needs manual field visiting
template <typename T> struct leio::Fields<Vec3D<T>> {
  template <typename V, typename F> static constexpr void each (V &value, F &&visit) {
    visit (value.data[0]);
    visit (value.data[1]);
    visit (value.data[2]);
  }
};

static_assert (leio::fully_covered<Vec3D<float>> (), "Vec3D<float> must be fully covered");

}
