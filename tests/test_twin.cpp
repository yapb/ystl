// test_twin.cpp - tests for ystl/twin.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Twin default construction compiles and does not crash [twin]") {
  Twin<int, int> t (0, 0);
  // twin's default ctor is = default; it does not zero-initialise primitive members
  REQUIRE ((t.first == 0));
  REQUIRE ((t.second == 0));
}

TEST_CASE ("Twin value construction stores both members [twin]") {
  Twin<int, float> t (42, 3.14f);
  REQUIRE ((t.first == 42));
  REQUIRE ((t.second == Approx (3.14f)));
}

TEST_CASE ("Twin copy construction copies both members [twin]") {
  Twin<int, int> a (10, 20);
  Twin<int, int> b (a);
  REQUIRE ((b.first == 10));
  REQUIRE ((b.second == 20));
}

TEST_CASE ("Twin move construction transfers both members [twin]") {
  Twin<int, int> a (5, 7);
  Twin<int, int> b (ystl::move (a));
  REQUIRE ((b.first == 5));
  REQUIRE ((b.second == 7));
}

// assignment
TEST_CASE ("Twin copy assignment copies both members [twin]") {
  Twin<int, int> a (1, 2);
  Twin<int, int> b;
  b = a;
  REQUIRE ((b.first == 1));
  REQUIRE ((b.second == 2));
}

TEST_CASE ("Twin move assignment transfers both members [twin]") {
  Twin<int, int> a (3, 4);
  Twin<int, int> b;
  b = ystl::move (a);
  REQUIRE ((b.first == 3));
  REQUIRE ((b.second == 4));
}

// comparison operators
TEST_CASE ("Twin operator== compares both members [twin]") {
  Twin<int, int> a (1, 2);
  Twin<int, int> b (1, 2);
  Twin<int, int> c (1, 3);
  REQUIRE ((a == b));
  REQUIRE_FALSE ((a == c));
}

TEST_CASE ("Twin operator!= detects any difference [twin]") {
  Twin<int, int> a (1, 2);
  Twin<int, int> b (1, 3);
  REQUIRE ((a != b));
  REQUIRE_FALSE ((a != Twin<int, int> (1, 2)));
}

TEST_CASE ("Twin operator< orders by first then second [twin]") {
  Twin<int, int> lo (1, 1);
  Twin<int, int> hi (2, 0);
  REQUIRE ((lo < hi));
  REQUIRE_FALSE ((hi < lo));

  // same first, different second
  Twin<int, int> p (5, 3);
  Twin<int, int> q (5, 10);
  REQUIRE ((p < q));
  REQUIRE_FALSE ((q < p));
}

TEST_CASE ("Twin operator> orders by first then second [twin]") {
  Twin<int, int> lo (1, 5);
  Twin<int, int> hi (2, 0);
  REQUIRE ((hi > lo));
  REQUIRE_FALSE ((lo > hi));
}

TEST_CASE ("Twin operator<= and operator>= work correctly [twin]") {
  Twin<int, int> a (3, 3);
  Twin<int, int> b (3, 3);
  Twin<int, int> c (4, 0);

  REQUIRE ((a <= b));
  REQUIRE ((a <= c));
  REQUIRE ((c >= a));
  REQUIRE_FALSE ((c <= a));
}

// mixed-type construction
TEST_CASE ("Twin supports different first and second types [twin]") {
  Twin<int, float> t (10, 1.5f);
  REQUIRE ((t.first == 10));
  REQUIRE ((t.second == Approx (1.5f)));
}

// make_twin helper
TEST_CASE ("make_twin creates twin with correct types [twin]") {
  auto t = make_twin (42, 3.14f);
  REQUIRE ((t.first == 42));
  REQUIRE ((t.second == Approx (3.14f)));
}

TEST_CASE ("make_twin decays references and arrays [twin]") {
  int x = 10;
  float y = 2.5f;
  auto t = make_twin (x, y);
  REQUIRE ((t.first == 10));
  REQUIRE ((t.second == Approx (2.5f)));
  x = 99;
  REQUIRE ((t.first == 10));
}

// swap member and free
TEST_CASE ("Twin::swap exchanges members [twin]") {
  Twin<int, int> a (1, 2);
  Twin<int, int> b (3, 4);
  a.swap (b);
  REQUIRE ((a.first == 3));
  REQUIRE ((a.second == 4));
  REQUIRE ((b.first == 1));
  REQUIRE ((b.second == 2));
}

TEST_CASE ("Free swap function exchanges twins [twin]") {
  Twin<int, int> a (10, 20);
  Twin<int, int> b (30, 40);
  swap (a, b);
  REQUIRE ((a.first == 30));
  REQUIRE ((a.second == 40));
  REQUIRE ((b.first == 10));
  REQUIRE ((b.second == 20));
}

// converting construction
TEST_CASE ("Twin converting copy construction [twin]") {
  Twin<short, short> a (5, 10);
  Twin<int, int> b (a);
  REQUIRE ((b.first == 5));
  REQUIRE ((b.second == 10));
}

TEST_CASE ("Twin converting move construction [twin]") {
  Twin<short, short> a (7, 14);
  Twin<int, int> b (ystl::move (a));
  REQUIRE ((b.first == 7));
  REQUIRE ((b.second == 14));
}

// converting assignment
TEST_CASE ("Twin converting copy assignment [twin]") {
  Twin<short, short> a (3, 6);
  Twin<int, int> b;
  b = a;
  REQUIRE ((b.first == 3));
  REQUIRE ((b.second == 6));
}

TEST_CASE ("Twin converting move assignment [twin]") {
  Twin<short, short> a (8, 16);
  Twin<int, int> b;
  b = ystl::move (a);
  REQUIRE ((b.first == 8));
  REQUIRE ((b.second == 16));
}
