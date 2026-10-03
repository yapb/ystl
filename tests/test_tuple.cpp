// test_tuple.cpp - tests for ystl/tuple.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Tuple default construction zero-initializes [tuple]") {
  Tuple<int, float> t;
  REQUIRE ((get<0> (t) == 0));
  REQUIRE ((get<1> (t) == 0.0f));
}

TEST_CASE ("Tuple value construction [tuple]") {
  Tuple<int, String, float> t (42, String ("hello"), 3.14f);
  REQUIRE ((get<0> (t) == 42));
  REQUIRE ((get<1> (t) == "hello"));
  REQUIRE ((get<2> (t) == 3.14f));
}

TEST_CASE ("Tuple makeTuple with decay [tuple]") {
  auto t = make_tuple (42, String ("hello"), 3.14f);
  REQUIRE ((get<0> (t) == 42));
  REQUIRE ((get<1> (t) == "hello"));
  REQUIRE ((get<2> (t) == 3.14f));
}

TEST_CASE ("Tuple single element [tuple]") {
  Tuple<int> t (42);
  REQUIRE ((get<0> (t) == 42));
}

// copy / move
TEST_CASE ("Tuple copy construction [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> b (a);
  REQUIRE ((get<0> (b) == 42));
  REQUIRE ((get<1> (b) == "hello"));
  // original unchanged
  REQUIRE ((get<0> (a) == 42));
}

TEST_CASE ("Tuple move construction [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> b (ystl::move (a));
  REQUIRE ((get<0> (b) == 42));
  REQUIRE ((get<1> (b) == "hello"));
}

TEST_CASE ("Tuple copy assignment [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> b;
  b = a;
  REQUIRE ((get<0> (b) == 42));
  REQUIRE ((get<1> (b) == "hello"));
}

TEST_CASE ("Tuple move assignment [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> b;
  b = ystl::move (a);
  REQUIRE ((get<0> (b) == 42));
  REQUIRE ((get<1> (b) == "hello"));
}

// get by index
TEST_CASE ("Tuple get by index returns reference [tuple]") {
  Tuple<int, String> t (10, String ("world"));
  REQUIRE ((get<0> (t) == 10));
  REQUIRE ((get<1> (t) == "world"));

  get<0> (t) = 99;
  REQUIRE ((get<0> (t) == 99));
}

TEST_CASE ("Tuple const get by index [tuple]") {
  const Tuple<int, String> t (42, String ("hello"));
  REQUIRE ((get<0> (t) == 42));
  REQUIRE ((get<1> (t) == "hello"));
}

TEST_CASE ("Tuple get by index rvalue [tuple]") {
  Tuple<String, int> t (String ("hello"), 42);
  String s = get<0> (ystl::move (t));
  REQUIRE ((s == "hello"));
}

// get by type
TEST_CASE ("Tuple get by type [tuple]") {
  Tuple<int, String, float> t (42, String ("hello"), 3.14f);
  REQUIRE ((get<int> (t) == 42));
  REQUIRE ((get<String> (t) == "hello"));
  REQUIRE ((get<float> (t) == 3.14f));

  get<int> (t) = 99;
  REQUIRE ((get<int> (t) == 99));
}

TEST_CASE ("Tuple const get by type [tuple]") {
  const Tuple<int, String> t (42, String ("hello"));
  REQUIRE ((get<int> (t) == 42));
  REQUIRE ((get<String> (t) == "hello"));
}

// comparison operators
TEST_CASE ("Tuple equality [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> b (42, String ("hello"));
  Tuple<int, String> c (42, String ("world"));
  Tuple<int, String> d (99, String ("hello"));

  REQUIRE ((a == b));
  REQUIRE_FALSE ((a == c));
  REQUIRE_FALSE ((a == d));
}

TEST_CASE ("Tuple inequality [tuple]") {
  Tuple<int, String> a (42, String ("hello"));
  Tuple<int, String> c (42, String ("world"));

  REQUIRE ((a != c));
}

TEST_CASE ("Tuple less-than lexicographic [tuple]") {
  Tuple<int, int> a (1, 2);
  Tuple<int, int> b (1, 3);
  Tuple<int, int> c (2, 0);

  REQUIRE ((a < b)); // first equal, second less
  REQUIRE ((a < c)); // first less
  REQUIRE_FALSE ((b < a));
  REQUIRE_FALSE ((c < a));
}

TEST_CASE ("Tuple greater/less-equal [tuple]") {
  Tuple<int, int> a (1, 2);
  Tuple<int, int> b (1, 2);
  Tuple<int, int> c (1, 3);

  REQUIRE ((a >= b));
  REQUIRE ((a <= b));
  REQUIRE ((c > a));
  REQUIRE ((a < c));
}

// swap
TEST_CASE ("Tuple swap exchanges contents [tuple]") {
  Tuple<int, String> a (1, String ("hello"));
  Tuple<int, String> b (2, String ("world"));
  a.swap (b);
  REQUIRE ((get<0> (a) == 2));
  REQUIRE ((get<1> (a) == "world"));
  REQUIRE ((get<0> (b) == 1));
  REQUIRE ((get<1> (b) == "hello"));
}

TEST_CASE ("Tuple free swap [tuple]") {
  Tuple<int, String> a (1, String ("hello"));
  Tuple<int, String> b (2, String ("world"));
  swap (a, b);
  REQUIRE ((get<0> (a) == 2));
  REQUIRE ((get<1> (a) == "world"));
}

// traits
TEST_CASE ("tuple_size returns number of elements [tuple]") {
  REQUIRE ((tuple_size_v<Tuple<int, float, String>> == 3u));
  REQUIRE ((tuple_size_v<Tuple<int>> == 1u));
  REQUIRE ((tuple_size_v<Tuple<>> == 0u));
}

TEST_CASE ("tuple_element resolves correct type [tuple]") {
  static_assert (is_same<tuple_element_t<0, Tuple<int, float, String>>, int>::value, "");
  static_assert (is_same<tuple_element_t<1, Tuple<int, float, String>>, float>::value, "");
  static_assert (is_same<tuple_element_t<2, Tuple<int, float, String>>, String>::value, "");
}

// heterogeneous copy /
TEST_CASE ("Tuple heterogeneous copy [tuple]") {
  Tuple<int, const char *> a (42, "hello");
  Tuple<int, String> b (a);
  REQUIRE ((get<0> (b) == 42));
  REQUIRE ((get<1> (b) == "hello"));
}

// many elements
TEST_CASE ("Tuple with many elements [tuple]") {
  Tuple<int, float, String, bool, int> t (1, 2.0f, String ("x"), true, 5);
  REQUIRE ((get<0> (t) == 1));
  REQUIRE ((get<1> (t) == 2.0f));
  REQUIRE ((get<2> (t) == "x"));
  REQUIRE ((get<3> (t) == true));
  REQUIRE ((get<4> (t) == 5));
}
