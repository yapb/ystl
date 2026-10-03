// test_hashset.cpp - tests for ystl/hashset.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

TEST_CASE ("HashSet default construction is empty [hashset]") {
  HashSet<String> s;
  REQUIRE (s.empty ());
  REQUIRE (s.size () == 0u);
}

TEST_CASE ("HashSet insert and exists [hashset]") {
  HashSet<String> s;
  REQUIRE (s.insert ("alpha"));
  REQUIRE (s.insert ("beta"));
  REQUIRE_FALSE (s.insert ("alpha")); // duplicate

  REQUIRE (s.size () == 2u);
  REQUIRE (s.exists ("alpha"));
  REQUIRE (s.exists ("beta"));
  REQUIRE_FALSE (s.exists ("gamma"));
  REQUIRE (s.contains ("alpha"));
}

TEST_CASE ("HashSet insert rvalue [hashset]") {
  HashSet<String> s;
  String key ("moved_key_long_padding");

  REQUIRE (s.insert (ystl::move (key)));
  REQUIRE (s.size () == 1u);
  REQUIRE (s.exists ("moved_key_long_padding"));
}

TEST_CASE ("HashSet erase removes entries [hashset]") {
  HashSet<String> s;
  s.insert ("one");
  s.insert ("two");

  REQUIRE (s.erase ("one") == 1u);
  REQUIRE (s.erase ("one") == 0u);
  REQUIRE (s.size () == 1u);
  REQUIRE_FALSE (s.exists ("one"));
  REQUIRE (s.exists ("two"));
}

TEST_CASE ("HashSet clear removes all entries [hashset]") {
  HashSet<String> s;
  s.insert ("a");
  s.insert ("b");
  s.clear ();

  REQUIRE (s.empty ());
  REQUIRE_FALSE (s.exists ("a"));
}

TEST_CASE ("HashSet iteration visits every key [hashset]") {
  HashSet<int> s;

  for (int i = 0; i < 64; ++i) {
    REQUIRE (s.insert (i));
  }
  REQUIRE (s.size () == 64u);

  int seen[64] = {};

  for (const auto &key : s) {
    REQUIRE (key >= 0 && key < 64);
    ++seen[key];
  }

  for (int i = 0; i < 64; ++i) {
    REQUIRE (seen[i] == 1);
  }
}

TEST_CASE ("HashSet reserve grows capacity [hashset]") {
  HashSet<String> s;
  s.reserve (1000);
  REQUIRE (s.capacity () >= 1000u);
  REQUIRE (s.empty ());

  s.insert ("after_reserve");
  REQUIRE (s.exists ("after_reserve"));
}

TEST_CASE ("HashSet int keys with growth [hashset]") {
  HashSet<int32_t> s;

  for (int32_t i = 0; i < 500; ++i) {
    s.insert (i * 2 + 1);
  }

  for (int32_t i = 0; i < 500; ++i) {
    REQUIRE (s.exists (i * 2 + 1));
    REQUIRE_FALSE (s.exists (i * 2));
  }
  REQUIRE (s.size () == 500u);
}
