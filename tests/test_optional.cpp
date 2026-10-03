// test_optional.cpp - tests for ystl/optional.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Optional default construction is empty [optional]") {
  Optional<int> a;
  REQUIRE_FALSE (a.has ());
  REQUIRE_FALSE (a);
}

TEST_CASE ("Optional nullopt construction is empty [optional]") {
  Optional<int> a (Nullopt);
  REQUIRE_FALSE (a.has ());
  REQUIRE_FALSE (a);
}

TEST_CASE ("Optional value construction (lvalue) [optional]") {
  int v = 42;
  Optional<int> a (v);
  REQUIRE (a.has ());
  REQUIRE (a);
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional value construction (rvalue) [optional]") {
  Optional<int> a (42);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional in-place construction [optional]") {
  Optional<int> a (InPlace, 42);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional in-place construction with multiple args [optional]") {
  Optional<String> a (InPlace, "hello", 5u);
  REQUIRE (a.has ());
  REQUIRE (*a == "hello");
}

// copy construction
TEST_CASE ("Optional copy construction from engaged [optional]") {
  Optional<int> a (42);
  Optional<int> b (a);
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
  // original unchanged
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional copy construction from empty [optional]") {
  Optional<int> a;
  Optional<int> b (a);
  REQUIRE_FALSE (b.has ());
}

// move construction
TEST_CASE ("Optional move construction from engaged [optional]") {
  Optional<int> a (42);
  Optional<int> b (ystl::move (a));
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional move construction from empty [optional]") {
  Optional<int> a;
  Optional<int> b (ystl::move (a));
  REQUIRE_FALSE (b.has ());
}

// nullopt assignment
TEST_CASE ("Optional nullopt assignment clears engaged [optional]") {
  Optional<int> a (42);
  a = Nullopt;
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional nullopt assignment on empty is no-op [optional]") {
  Optional<int> a;
  a = Nullopt;
  REQUIRE_FALSE (a.has ());
}

// value assignment
TEST_CASE ("Optional value assignment to empty [optional]") {
  Optional<int> a;
  a = 42;
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional value assignment replaces engaged [optional]") {
  Optional<int> a (10);
  a = 42;
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional rvalue assignment to empty [optional]") {
  Optional<int> a;
  int v = 42;
  a = ystl::move (v);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

// copy assignment
TEST_CASE ("Optional copy assignment empty=empty [optional]") {
  Optional<int> a;
  Optional<int> b;
  b = a;
  REQUIRE_FALSE (b.has ());
}

TEST_CASE ("Optional copy assignment engaged=empty [optional]") {
  Optional<int> a;
  Optional<int> b (42);
  b = a;
  REQUIRE_FALSE (b.has ());
}

TEST_CASE ("Optional copy assignment empty=engaged [optional]") {
  Optional<int> a (42);
  Optional<int> b;
  b = a;
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
  // original unchanged
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional copy assignment both engaged [optional]") {
  Optional<int> a (42);
  Optional<int> b (10);
  b = a;
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
}

// move assignment
TEST_CASE ("Optional move assignment engaged=empty [optional]") {
  Optional<int> a (42);
  Optional<int> b;
  b = ystl::move (a);
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional move assignment empty=engaged [optional]") {
  Optional<int> a;
  Optional<int> b (42);
  b = ystl::move (a);
  REQUIRE_FALSE (b.has ());
}

TEST_CASE ("Optional move assignment both engaged [optional]") {
  Optional<int> a (42);
  Optional<int> b (10);
  b = ystl::move (a);
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
  REQUIRE_FALSE (a.has ());
}

// self-assignment
TEST_CASE ("Optional copy self-assignment is safe [optional]") {
  Optional<int> a (42);
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
  a = a;
#if defined(__clang__)
  #pragma clang diagnostic pop
#endif
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional move self-assignment is safe [optional]") {
  Optional<int> a (42);
  a = ystl::move (a);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

// emplace / reset
TEST_CASE ("Optional emplace constructs value [optional]") {
  Optional<int> a;
  a.emplace (42);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional emplace replaces existing value [optional]") {
  Optional<int> a (10);
  a.emplace (42);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional emplace with multiple args [optional]") {
  Optional<String> a;
  a.emplace ("world", 5u);
  REQUIRE (a.has ());
  REQUIRE (*a == "world");
}

TEST_CASE ("Optional reset clears engaged [optional]") {
  Optional<int> a (42);
  a.reset ();
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional reset on empty is no-op [optional]") {
  Optional<int> a;
  a.reset ();
  REQUIRE_FALSE (a.has ());
}

// swap
TEST_CASE ("Optional swap both engaged [optional]") {
  Optional<int> a (10);
  Optional<int> b (20);
  a.swap (b);
  REQUIRE (*a == 20);
  REQUIRE (*b == 10);
}

TEST_CASE ("Optional swap engaged and empty [optional]") {
  Optional<int> a (42);
  Optional<int> b;
  a.swap (b);
  REQUIRE_FALSE (a.has ());
  REQUIRE (b.has ());
  REQUIRE (*b == 42);
}

TEST_CASE ("Optional swap empty and engaged [optional]") {
  Optional<int> a;
  Optional<int> b (42);
  a.swap (b);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
  REQUIRE_FALSE (b.has ());
}

TEST_CASE ("Optional swap both empty [optional]") {
  Optional<int> a;
  Optional<int> b;
  a.swap (b);
  REQUIRE_FALSE (a.has ());
  REQUIRE_FALSE (b.has ());
}

TEST_CASE ("Optional swap with non-trivial type [optional]") {
  Optional<String> a (String ("hello"));
  Optional<String> b (String ("world"));
  a.swap (b);
  REQUIRE (*a == "world");
  REQUIRE (*b == "hello");
}

// has / operator bool
TEST_CASE ("Optional has returns true when engaged [optional]") {
  Optional<int> a (42);
  REQUIRE (a.has ());
  REQUIRE (a);
}

TEST_CASE ("Optional has returns false when empty [optional]") {
  Optional<int> a;
  REQUIRE_FALSE (a.has ());
  REQUIRE_FALSE (a);
}

// value / valueor
TEST_CASE ("Optional value returns reference when engaged [optional]") {
  Optional<int> a (42);
  REQUIRE (a.value () == 42);

  a.value () = 99;
  REQUIRE (*a == 99);
}

TEST_CASE ("Optional const value returns reference [optional]") {
  const Optional<int> a (42);
  REQUIRE (a.value () == 42);
}

TEST_CASE ("Optional valueOr returns value when engaged [optional]") {
  Optional<int> a (42);
  REQUIRE (a.value_or (0) == 42);
}

TEST_CASE ("Optional valueOr returns default when empty [optional]") {
  Optional<int> a;
  REQUIRE (a.value_or (99) == 99);
}

// operator* / operator->
TEST_CASE ("Optional operator* returns reference [optional]") {
  Optional<int> a (42);
  REQUIRE (*a == 42);

  *a = 99;
  REQUIRE (*a == 99);
}

TEST_CASE ("Optional const operator* returns reference [optional]") {
  const Optional<int> a (42);
  REQUIRE (*a == 42);
}

TEST_CASE ("Optional operator-> accesses members [optional]") {
  Optional<String> a (String ("hello"));
  REQUIRE (a->size () == 5u);
}

TEST_CASE ("Optional const operator-> accesses members [optional]") {
  const Optional<String> a (String ("hello"));
  REQUIRE (a->size () == 5u);
}

// comparison operators:
TEST_CASE ("Optional equality both engaged same value [optional]") {
  Optional<int> a (42);
  Optional<int> b (42);
  REQUIRE (a == b);
}

TEST_CASE ("Optional equality both engaged different value [optional]") {
  Optional<int> a (42);
  Optional<int> b (10);
  REQUIRE_FALSE (a == b);
  REQUIRE (a != b);
}

TEST_CASE ("Optional equality both empty [optional]") {
  Optional<int> a;
  Optional<int> b;
  REQUIRE (a == b);
}

TEST_CASE ("Optional equality engaged vs empty [optional]") {
  Optional<int> a (42);
  Optional<int> b;
  REQUIRE_FALSE (a == b);
  REQUIRE (a != b);
}

TEST_CASE ("Optional less-than comparison [optional]") {
  Optional<int> a (10);
  Optional<int> b (20);
  Optional<int> empty;

  REQUIRE (a < b);
  REQUIRE_FALSE (b < a);
  REQUIRE (empty < a);
  REQUIRE_FALSE (a < empty);
  REQUIRE_FALSE (empty < empty);
}

// comparison operators:
TEST_CASE ("Optional equality with nullopt [optional]") {
  Optional<int> a (42);
  Optional<int> empty;

  REQUIRE (empty == Nullopt);
  REQUIRE (Nullopt == empty);
  REQUIRE_FALSE (a == Nullopt);
  REQUIRE_FALSE (Nullopt == a);
}

TEST_CASE ("Optional inequality with nullopt [optional]") {
  Optional<int> a (42);
  Optional<int> empty;

  REQUIRE (a != Nullopt);
  REQUIRE (Nullopt != a);
  REQUIRE_FALSE (empty != Nullopt);
}

TEST_CASE ("Optional less-than with nullopt [optional]") {
  Optional<int> a (42);
  Optional<int> empty;

  REQUIRE_FALSE (a < Nullopt);
  REQUIRE_FALSE (empty < Nullopt);
  REQUIRE (Nullopt < a);
  REQUIRE_FALSE (Nullopt < empty);
}

// comparison operators:
TEST_CASE ("Optional equality with value [optional]") {
  Optional<int> a (42);
  Optional<int> empty;

  REQUIRE (a == 42);
  REQUIRE (42 == a);
  REQUIRE_FALSE (empty == 42);
  REQUIRE_FALSE (42 == empty);
}

TEST_CASE ("Optional inequality with value [optional]") {
  Optional<int> a (42);
  Optional<int> empty;

  REQUIRE (a != 10);
  REQUIRE (empty != 42);
  REQUIRE_FALSE (a != 42);
}

TEST_CASE ("Optional less-than with value [optional]") {
  Optional<int> a (10);
  Optional<int> empty;

  REQUIRE (a < 20);
  REQUIRE_FALSE (a < 5);
  REQUIRE (empty < 20);
  REQUIRE_FALSE (20 < a);
}

// makeoptional
TEST_CASE ("makeOptional constructs optional with value [optional]") {
  auto a = make_optional<int> (42);
  REQUIRE (a.has ());
  REQUIRE (*a == 42);
}

TEST_CASE ("makeOptional with string [optional]") {
  auto a = make_optional<String> ("hello", 5u);
  REQUIRE (a.has ());
  REQUIRE (*a == "hello");
}

// non-trivial type
TEST_CASE ("Optional with String default construction is empty [optional]") {
  Optional<String> a;
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional with String value construction [optional]") {
  Optional<String> a (String ("hello"));
  REQUIRE (a.has ());
  REQUIRE (*a == "hello");
}

TEST_CASE ("Optional with String copy construction [optional]") {
  Optional<String> a (String ("hello"));
  Optional<String> b (a);
  REQUIRE (b.has ());
  REQUIRE (*b == "hello");
  REQUIRE (*a == "hello");
}

TEST_CASE ("Optional with String move construction [optional]") {
  Optional<String> a (String ("hello"));
  Optional<String> b (ystl::move (a));
  REQUIRE (b.has ());
  REQUIRE (*b == "hello");
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional with String emplace [optional]") {
  Optional<String> a;
  a.emplace ("world", 5u);
  REQUIRE (a.has ());
  REQUIRE (*a == "world");
}

TEST_CASE ("Optional with String reset calls destructor [optional]") {
  Optional<String> a (String ("hello"));
  a.reset ();
  REQUIRE_FALSE (a.has ());
}

TEST_CASE ("Optional with String value assignment replaces [optional]") {
  Optional<String> a (String ("hello"));
  a = String ("world");
  REQUIRE (a.has ());
  REQUIRE (*a == "world");
}

TEST_CASE ("Optional with String copy assignment [optional]") {
  Optional<String> a (String ("hello"));
  Optional<String> b;
  b = a;
  REQUIRE (b.has ());
  REQUIRE (*b == "hello");
  REQUIRE (*a == "hello");
}

TEST_CASE ("Optional with String move assignment [optional]") {
  Optional<String> a (String ("hello"));
  Optional<String> b;
  b = ystl::move (a);
  REQUIRE (b.has ());
  REQUIRE (*b == "hello");
  REQUIRE_FALSE (a.has ());
}
