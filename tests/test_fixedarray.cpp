// test_fixedarray.cpp - tests for ystl/fixedarray.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// size / empty
TEST_CASE ("FixedArray size and empty [fixedarray]") {
  FixedArray<int, 8> a {};
  REQUIRE (a.size () == 8u);
  REQUIRE_FALSE (a.empty ());

  FixedArray<int, 0> b {};
  REQUIRE (b.size () == 0u);
  REQUIRE (b.empty ());
}

// default / zero
TEST_CASE ("FixedArray default construction zero-initializes [fixedarray]") {
  FixedArray<int, 4> a {};
  for (size_t i = 0; i < 4; ++i) {
    REQUIRE (a[i] == 0);
  }
}

// aggregate
TEST_CASE ("FixedArray aggregate initialization [fixedarray]") {
  FixedArray<int, 5> a = { 10, 20, 30, 40, 50 };
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 20);
  REQUIRE (a[2] == 30);
  REQUIRE (a[3] == 40);
  REQUIRE (a[4] == 50);
}

TEST_CASE ("FixedArray partial aggregate initialization zero-fills remainder [fixedarray]") {
  FixedArray<int, 5> a = { 1, 2 };
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 0);
  REQUIRE (a[3] == 0);
  REQUIRE (a[4] == 0);
}

// operator[] / at
TEST_CASE ("FixedArray operator[] read and write [fixedarray]") {
  FixedArray<int, 3> a {};
  a[0] = 10;
  a[1] = 20;
  a[2] = 30;

  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 20);
  REQUIRE (a[2] == 30);
}

TEST_CASE ("FixedArray at read and write [fixedarray]") {
  FixedArray<int, 3> a {};
  a.at (0) = 7;
  a.at (2) = 42;

  REQUIRE (a.at (0) == 7);
  REQUIRE (a.at (2) == 42);
}

TEST_CASE ("FixedArray const accessors [fixedarray]") {
  const FixedArray<int, 3> a = { 5, 10, 15 };
  REQUIRE (a[0] == 5);
  REQUIRE (a[1] == 10);
  REQUIRE (a.at (2) == 15);
}

// front / back
TEST_CASE ("FixedArray front and back [fixedarray]") {
  FixedArray<int, 3> a = { 100, 200, 300 };
  REQUIRE (a.front () == 100);
  REQUIRE (a.back () == 300);

  a.front () = 1;
  a.back () = 9;
  REQUIRE (a[0] == 1);
  REQUIRE (a[2] == 9);
}

TEST_CASE ("FixedArray const front and back [fixedarray]") {
  const FixedArray<int, 3> a = { 10, 20, 30 };
  REQUIRE (a.front () == 10);
  REQUIRE (a.back () == 30);
}

// data
TEST_CASE ("FixedArray data returns pointer to contiguous storage [fixedarray]") {
  FixedArray<int, 3> a = { 1, 2, 3 };
  int *p = a.data ();
  REQUIRE (p[0] == 1);
  REQUIRE (p[1] == 2);
  REQUIRE (p[2] == 3);
}

TEST_CASE ("FixedArray const data [fixedarray]") {
  const FixedArray<int, 3> a = { 5, 6, 7 };
  const int *p = a.data ();
  REQUIRE (p[0] == 5);
}

// fill
TEST_CASE ("FixedArray fill sets all elements [fixedarray]") {
  FixedArray<int, 5> a {};
  a.fill (42);
  for (size_t i = 0; i < 5; ++i) {
    REQUIRE (a[i] == 42);
  }
}

// begin / end / range-for
TEST_CASE ("FixedArray range-based for loop [fixedarray]") {
  FixedArray<int, 4> a = { 1, 2, 3, 4 };
  int sum = 0;
  for (auto v : a) {
    sum += v;
  }
  REQUIRE (sum == 10);
}

TEST_CASE ("FixedArray begin and end iterators [fixedarray]") {
  FixedArray<int, 3> a = { 10, 20, 30 };
  int sum = 0;
  for (auto it = a.begin (); it != a.end (); ++it) {
    sum += *it;
  }
  REQUIRE (sum == 60);
}

TEST_CASE ("FixedArray const begin and end [fixedarray]") {
  const FixedArray<int, 3> a = { 5, 10, 15 };
  int sum = 0;
  for (auto it = a.cbegin (); it != a.cend (); ++it) {
    sum += *it;
  }
  REQUIRE (sum == 30);
}

// swap
TEST_CASE ("FixedArray swap exchanges elements [fixedarray]") {
  FixedArray<int, 3> a = { 1, 2, 3 };
  FixedArray<int, 3> b = { 10, 20, 30 };
  a.swap (b);

  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 20);
  REQUIRE (a[2] == 30);
  REQUIRE (b[0] == 1);
  REQUIRE (b[1] == 2);
  REQUIRE (b[2] == 3);
}

// equality operators
TEST_CASE ("FixedArray equality comparison [fixedarray]") {
  FixedArray<int, 3> a = { 1, 2, 3 };
  FixedArray<int, 3> b = { 1, 2, 3 };
  FixedArray<int, 3> c = { 1, 2, 4 };

  REQUIRE (a == b);
  REQUIRE_FALSE (a == c);
  REQUIRE (a != c);
}

// copy and move semantics
TEST_CASE ("FixedArray copy constructor [fixedarray]") {
  FixedArray<int, 3> a = { 10, 20, 30 };
  FixedArray<int, 3> b (a);

  REQUIRE (b[0] == 10);
  REQUIRE (b[1] == 20);
  REQUIRE (b[2] == 30);
}

TEST_CASE ("FixedArray copy assignment [fixedarray]") {
  FixedArray<int, 3> a = { 1, 2, 3 };
  FixedArray<int, 3> b {};
  b = a;

  REQUIRE (b[0] == 1);
  REQUIRE (b[1] == 2);
  REQUIRE (b[2] == 3);
}

TEST_CASE ("FixedArray move constructor [fixedarray]") {
  FixedArray<int, 3> a = { 5, 6, 7 };
  FixedArray<int, 3> b (ystl::move (a));

  REQUIRE (b[0] == 5);
  REQUIRE (b[1] == 6);
  REQUIRE (b[2] == 7);
}

TEST_CASE ("FixedArray move assignment [fixedarray]") {
  FixedArray<int, 3> a = { 10, 20, 30 };
  FixedArray<int, 3> b {};
  b = ystl::move (a);

  REQUIRE (b[0] == 10);
  REQUIRE (b[1] == 20);
  REQUIRE (b[2] == 30);
}

// non-trivial type
TEST_CASE ("FixedArray with non-trivial type default init [fixedarray]") {
  FixedArray<String, 3> a {};
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == "");
  REQUIRE (a[1] == "");
  REQUIRE (a[2] == "");
}

TEST_CASE ("FixedArray with non-trivial type aggregate init [fixedarray]") {
  FixedArray<String, 3> a = { String ("one"), String ("two"), String ("three") };
  REQUIRE (a[0] == "one");
  REQUIRE (a[1] == "two");
  REQUIRE (a[2] == "three");
}

TEST_CASE ("FixedArray with non-trivial type copy [fixedarray]") {
  FixedArray<String, 2> a = { String ("hello"), String ("world") };
  FixedArray<String, 2> b (a);

  REQUIRE (b[0] == "hello");
  REQUIRE (b[1] == "world");
}

TEST_CASE ("FixedArray with non-trivial type move [fixedarray]") {
  FixedArray<String, 2> a = { String ("foo"), String ("bar") };
  FixedArray<String, 2> b (ystl::move (a));

  REQUIRE (b[0] == "foo");
  REQUIRE (b[1] == "bar");
}

TEST_CASE ("FixedArray with non-trivial type fill [fixedarray]") {
  FixedArray<String, 3> a {};
  a.fill (String ("z"));
  for (size_t i = 0; i < 3; ++i) {
    REQUIRE (a[i] == "z");
  }
}

TEST_CASE ("FixedArray with non-trivial type swap [fixedarray]") {
  FixedArray<String, 2> a = { String ("a1"), String ("a2") };
  FixedArray<String, 2> b = { String ("b1"), String ("b2") };
  a.swap (b);

  REQUIRE (a[0] == "b1");
  REQUIRE (a[1] == "b2");
  REQUIRE (b[0] == "a1");
  REQUIRE (b[1] == "a2");
}
