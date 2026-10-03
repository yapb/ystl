// test_movable.cpp - tests for ystl/movable.h (ystl::move, ystl::forward, ystl::swap)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// ystl::move
TEST_CASE ("ystl::move produces an rvalue reference [movable]") {
  int a = 42;
  int b = ystl::move (a);
  REQUIRE (b == 42);
}

TEST_CASE ("ystl::move enables move construction [movable]") {
  struct Movable {
    int value { 0 };
    bool moved { false };
    Movable () = default;
    Movable (Movable &&rhs) noexcept : value (rhs.value), moved (true) {
      rhs.value = 0;
    }
  };

  Movable src;
  src.value = 99;

  Movable dst (ystl::move (src));
  REQUIRE (dst.moved);
  REQUIRE (dst.value == 99);
  REQUIRE (src.value == 0);
}

// ystl::forward
TEST_CASE ("ystl::forward passes lvalue as lvalue [movable]") {
  auto fwd = [] (int &x) -> int & {
    return ystl::forward<int &> (x);
  };
  int v = 7;
  int &ref = fwd (v);
  REQUIRE (&ref == &v);
}

TEST_CASE ("ystl::forward passes rvalue as rvalue (via perfect forwarding) [movable]") {
  // verify that forwarding preserves the value category at compile time
  auto consume = [] (int &&x) {
    return x * 2;
  };
  int result = consume (ystl::forward<int> (10));
  REQUIRE (result == 20);
}

// ystl::swap - scalar types
TEST_CASE ("ystl::swap exchanges two integer values [movable]") {
  int a = 10, b = 20;
  ystl::swap (a, b);
  REQUIRE (a == 20);
  REQUIRE (b == 10);
}

TEST_CASE ("ystl::swap exchanges two float values [movable]") {
  float a = 1.5f, b = 2.5f;
  ystl::swap (a, b);
  REQUIRE (a == Approx (2.5f));
  REQUIRE (b == Approx (1.5f));
}

TEST_CASE ("ystl::swap with same value is idempotent [movable]") {
  int a = 5;
  ystl::swap (a, a);
  REQUIRE (a == 5);
}

// ystl::swap - movable
TEST_CASE ("ystl::swap works with move-only types [movable]") {
  struct MO {
    int v;
    MO (int v) : v (v) {}
    MO (MO &&rhs) noexcept : v (rhs.v) {
      rhs.v = 0;
    }
    MO &operator= (MO &&rhs) noexcept {
      v = rhs.v;
      rhs.v = 0;
      return *this;
    }
  };

  MO a (100), b (200);
  ystl::swap (a, b);
  REQUIRE (a.v == 200);
  REQUIRE (b.v == 100);
}

// ystl::swap - fixed-size
TEST_CASE ("ystl::swap exchanges C arrays element-wise [movable]") {
  int a[3] = { 1, 2, 3 };
  int b[3] = { 4, 5, 6 };
  ystl::swap (a, b);

  REQUIRE (a[0] == 4);
  REQUIRE (a[1] == 5);
  REQUIRE (a[2] == 6);
  REQUIRE (b[0] == 1);
  REQUIRE (b[1] == 2);
  REQUIRE (b[2] == 3);
}

TEST_CASE ("ystl::swap on same array (no-op) [movable]") {
  int a[3] = { 7, 8, 9 };
  ystl::swap (a, a); // should be a no-op (pointer equality check)
  REQUIRE (a[0] == 7);
  REQUIRE (a[1] == 8);
  REQUIRE (a[2] == 9);
}

// noncopyable /
TEST_CASE ("NonCopyable-derived class is default-constructible [movable]") {
  struct NC : public ystl::NonCopyable {
    explicit NC () = default;
    int x { 7 };
  };
  NC obj;
  REQUIRE (obj.x == 7);
}

TEST_CASE ("NonMovable-derived class is default-constructible [movable]") {
  struct NM : public ystl::NonMovable {
    explicit NM () = default;
    int y { 13 };
  };
  NM obj;
  REQUIRE (obj.y == 13);
}
