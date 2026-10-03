// test_hashmap.cpp - tests for ystl/hashmap.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction / empty
TEST_CASE ("HashMap default construction is empty [hashmap]") {
  HashMap<String, int> m;
  REQUIRE (m.empty ());
  REQUIRE (m.size () == 0u);
}

// operator[] insert &
TEST_CASE ("HashMap operator[] inserts and retrieves values [hashmap]") {
  HashMap<String, int> m;
  m["alpha"] = 1;
  m["beta"] = 2;
  m["gamma"] = 3;

  REQUIRE (m.size () == 3u);
  REQUIRE (m["alpha"] == 1);
  REQUIRE (m["beta"] == 2);
  REQUIRE (m["gamma"] == 3);
}

TEST_CASE ("HashMap operator[] updates existing key [hashmap]") {
  HashMap<String, int> m;
  m["x"] = 10;
  m["x"] = 20;

  REQUIRE (m.size () == 1u);
  REQUIRE (m["x"] == 20);
}

// insert
TEST_CASE ("HashMap insert returns true for new key [hashmap]") {
  HashMap<String, int> m;
  REQUIRE (m.insert ("key", 42));
  REQUIRE (m.size () == 1u);
  REQUIRE (m["key"] == 42);
}

TEST_CASE ("HashMap insert returns false for duplicate key [hashmap]") {
  HashMap<String, int> m;
  m["dup"] = 1;
  REQUIRE_FALSE (m.insert ("dup", 2));
  REQUIRE (m["dup"] == 1); // unchanged
}

// exists
TEST_CASE ("HashMap exists returns true for present key [hashmap]") {
  HashMap<String, int> m;
  m["present"] = 99;
  REQUIRE (m.exists ("present"));
  REQUIRE_FALSE (m.exists ("absent"));
}

// erase
TEST_CASE ("HashMap erase removes an existing key [hashmap]") {
  HashMap<String, int> m;
  m["a"] = 1;
  m["b"] = 2;

  REQUIRE (m.erase ("a") == 1u);
  REQUIRE (m.size () == 1u);
  REQUIRE_FALSE (m.exists ("a"));
  REQUIRE (m.exists ("b"));
}

TEST_CASE ("HashMap erase on missing key returns 0 [hashmap]") {
  HashMap<String, int> m;
  REQUIRE (m.erase ("nonexistent") == 0u);
}

// clear
TEST_CASE ("HashMap clear removes all entries [hashmap]") {
  HashMap<String, int> m;
  m["p"] = 1;
  m["q"] = 2;
  m.clear ();

  REQUIRE (m.empty ());
  REQUIRE (m.size () == 0u);
  REQUIRE_FALSE (m.exists ("p"));
}

// zap
TEST_CASE ("HashMap zap clears and resets storage [hashmap]") {
  HashMap<String, int> m;
  for (int i = 0; i < 50; ++i) {
    m[String ().assignf ("%d", i)] = i;
  }
  m.zap ();
  REQUIRE (m.empty ());
}

// capacity
TEST_CASE ("HashMap capacity is non-zero after construction [hashmap]") {
  HashMap<String, int> m;
  REQUIRE (m.capacity () > 0u);
}

// iteration
TEST_CASE ("HashMap iteration visits all inserted entries [hashmap]") {
  HashMap<String, int> m;
  m["x"] = 10;
  m["y"] = 20;
  m["z"] = 30;

  int total = 0;
  int count = 0;
  for (auto [k, v] : m) {
    total += v;
    ++count;
  }
  REQUIRE (count == 3);
  REQUIRE (total == 60);
}

// move constructor / move
TEST_CASE ("HashMap move constructor transfers state [hashmap]") {
  HashMap<String, int> a;
  a["one"] = 1;
  a["two"] = 2;

  HashMap<String, int> b (ystl::move (a));
  REQUIRE (b.size () == 2u);
  REQUIRE (b.exists ("one"));
  REQUIRE (b.exists ("two"));
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
}

TEST_CASE ("HashMap move assignment transfers state [hashmap]") {
  HashMap<String, int> a;
  a["hello"] = 42;

  HashMap<String, int> b;
  b = ystl::move (a);
  REQUIRE (b.size () == 1u);
  REQUIRE (b.exists ("hello"));
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
}

// initializer-list
TEST_CASE ("HashMap initializer-list construction [hashmap]") {
  HashMap<String, int> m {
    { String ("a"), 1 },
    { String ("b"), 2 },
    { String ("c"), 3 }
  };
  REQUIRE (m.size () == 3u);
  REQUIRE (m["a"] == 1);
  REQUIRE (m["b"] == 2);
  REQUIRE (m["c"] == 3);
}

// integer key hashmap
TEST_CASE ("HashMap with int32_t keys works correctly [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m[1] = 100;
  m[2] = 200;
  m[3] = 300;

  REQUIRE (m.size () == 3u);
  REQUIRE (m[1] == 100);
  REQUIRE (m[2] == 200);
  REQUIRE (m[3] == 300);
  REQUIRE (m.erase (2) == 1u);
  REQUIRE (m.size () == 2u);
  REQUIRE_FALSE (m.exists (2));
}

// rehash - insert enough
TEST_CASE ("HashMap handles many insertions with correct retrieval [hashmap]") {
  HashMap<int32_t, int32_t> m;
  for (int32_t i = 0; i < 200; ++i) {
    m[i] = i * 10;
  }
  REQUIRE (m.size () == 200u);
  for (int32_t i = 0; i < 200; ++i) {
    REQUIRE (m.exists (i));
    REQUIRE (m[i] == i * 10);
  }
}

// reserve
TEST_CASE ("HashMap reserve grows capacity to at least the requested size [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m.reserve (64);
  REQUIRE (m.capacity () >= 64u);
}

TEST_CASE ("HashMap reserve preserves existing entries [hashmap]") {
  HashMap<int32_t, int32_t> m;
  for (int32_t i = 0; i < 10; ++i) {
    m[i] = i;
  }
  m.reserve (256);
  REQUIRE (m.capacity () >= 256u);
  REQUIRE (m.size () == 10u);
  for (int32_t i = 0; i < 10; ++i) {
    REQUIRE (m.exists (i));
    REQUIRE (m[i] == i);
  }
}

// const_iterator
TEST_CASE ("HashMap const_iterator iterates all entries [hashmap]") {
  HashMap<String, int> m;
  m["a"] = 1;
  m["b"] = 2;
  m["c"] = 3;

  const auto &cm = m;
  int total = 0;
  int count = 0;
  for (auto [k, v] : cm) {
    total += v;
    ++count;
  }
  REQUIRE (count == 3);
  REQUIRE (total == 6);
}

TEST_CASE ("HashMap cbegin/cend work correctly [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m[1] = 10;
  m[2] = 20;

  int sum = 0;
  for (auto it = m.cbegin (); it != m.cend (); ++it) {
    auto [k, v] = *it;
    sum += v;
  }
  REQUIRE (sum == 30);
}

// hash<const char*> null
TEST_CASE ("HashMap with const char* key handles null gracefully [hashmap]") {
  HashMap<const char *, int> m;
  m["hello"] = 1;
  REQUIRE (m.exists ("hello"));
  REQUIRE (m["hello"] == 1);
}

// insert with rvalue
TEST_CASE ("HashMap insert with rvalue moves the value [hashmap]") {
  HashMap<String, String> m;
  String val ("moved_value");
  REQUIRE (m.insert ("key", ystl::move (val)));
  REQUIRE (m["key"] == "moved_value");
}

TEST_CASE ("HashMap insert rvalue returns false for duplicate [hashmap]") {
  HashMap<String, String> m;
  m["dup"] = "original";
  REQUIRE_FALSE (m.insert ("dup", String ("new_value")));
  REQUIRE (m["dup"] == "original");
}

// identityhash template
TEST_CASE ("HashMap with IdentityHash uses identity hash [hashmap]") {
  HashMap<int32_t, int32_t, IdentityHash<int32_t>> m;
  m[10] = 100;
  m[20] = 200;
  m[30] = 300;

  REQUIRE (m.size () == 3u);
  REQUIRE (m[10] == 100);
  REQUIRE (m[20] == 200);
  REQUIRE (m[30] == 300);
}

// move assignment
TEST_CASE ("HashMap move assignment self-assignment is safe [hashmap]") {
  HashMap<String, int> m;
  m["a"] = 1;
  m["b"] = 2;

  m = ystl::move (m);

  REQUIRE (m.size () == 2u);
  REQUIRE (m["a"] == 1);
  REQUIRE (m["b"] == 2);
}

// iterator post-increment
TEST_CASE ("HashMap iterator post-increment returns old position [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m[1] = 10;
  m[2] = 20;

  auto it = m.begin ();
  auto [k1, v1] = *it;
  auto old = it++;
  auto [k2, v2] = *old;

  REQUIRE (k1 == k2);
  REQUIRE (v1 == v2);
  REQUIRE (it != old);
}

// tombstone reuse
TEST_CASE ("HashMap reuses tombstone slots on insert [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m[1] = 10;
  m[2] = 20;
  m[3] = 30;

  size_t cap_before = m.capacity ();

  REQUIRE (m.erase (2) == 1u);
  REQUIRE_FALSE (m.exists (2));

  m[2] = 200;
  REQUIRE (m.exists (2));
  REQUIRE (m[2] == 200);
  REQUIRE (m.capacity () == cap_before);
}

TEST_CASE ("HashMap handles multiple erase and reinsert cycles [hashmap]") {
  HashMap<int32_t, int32_t> m;

  for (int i = 0; i < 10; ++i) {
    m[i] = i * 10;
  }

  for (int i = 0; i < 10; i += 2) {
    m.erase (i);
  }

  REQUIRE (m.size () == 5u);

  for (int i = 0; i < 10; i += 2) {
    m[i] = i * 100;
  }

  REQUIRE (m.size () == 10u);
  for (int i = 0; i < 10; ++i) {
    REQUIRE (m.exists (i));
  }
}

// reserve when smaller
TEST_CASE ("HashMap reserve smaller than current is no-op [hashmap]") {
  HashMap<int32_t, int32_t> m;
  m.reserve (64);
  size_t cap = m.capacity ();

  m.reserve (16);
  REQUIRE (m.capacity () == cap);
}

// hash collisions with
TEST_CASE ("HashMap handles hash collisions correctly [hashmap]") {
  struct CollidingHash {
    uint32_t operator() (int32_t) const noexcept {
      return 0;
    }
  };

  HashMap<int32_t, int32_t, CollidingHash> m;
  m[1] = 10;
  m[2] = 20;
  m[3] = 30;
  m[4] = 40;

  REQUIRE (m.size () == 4u);
  REQUIRE (m[1] == 10);
  REQUIRE (m[2] == 20);
  REQUIRE (m[3] == 30);
  REQUIRE (m[4] == 40);

  REQUIRE (m.erase (2) == 1u);
  REQUIRE (m.exists (1));
  REQUIRE_FALSE (m.exists (2));
  REQUIRE (m.exists (3));
  REQUIRE (m.exists (4));

  m[2] = 200;
  REQUIRE (m[2] == 200);
}

// stringref key type
TEST_CASE ("HashMap with StringRef key works correctly [hashmap]") {
  HashMap<StringRef, int> m;
  m[StringRef ("hello")] = 1;
  m[StringRef ("world")] = 2;

  REQUIRE (m.size () == 2u);
  REQUIRE (m.exists (StringRef ("hello")));
  REQUIRE (m.exists (StringRef ("world")));
  REQUIRE (m[StringRef ("hello")] == 1);
  REQUIRE (m[StringRef ("world")] == 2);
}

// empty map operations
TEST_CASE ("HashMap exists on empty map returns false [hashmap]") {
  HashMap<String, int> m;
  m.clear ();
  REQUIRE_FALSE (m.exists ("anything"));
}

TEST_CASE ("HashMap erase on empty map returns 0 [hashmap]") {
  HashMap<String, int> m;
  m.clear ();
  REQUIRE (m.erase ("anything") == 0u);
}

// contains
TEST_CASE ("HashMap contains matches exists [hashmap]") {
  HashMap<String, int> m;
  m["a"] = 1;

  REQUIRE (m.contains ("a"));
  REQUIRE_FALSE (m.contains ("b"));
}

TEST_CASE ("HashMap contains on empty map returns false [hashmap]") {
  HashMap<String, int> m;
  REQUIRE_FALSE (m.contains ("anything"));
}

// eraseif
TEST_CASE ("HashMap eraseIf removes matching entries [hashmap]") {
  HashMap<int, int> m;

  for (int i = 0; i < 10; ++i) {
    m[i] = i * i;
  }

  // evens: 0, 2, 4, 6, 8 plus key 9 (81 > 80)
  const auto removed = m.erase_if ([] (const int &key, const int &value) {
    return (key % 2) == 0 || value > 80;
  });

  REQUIRE (removed == 6u);
  REQUIRE (m.size () == 4u);

  REQUIRE_FALSE (m.contains (0));
  REQUIRE_FALSE (m.contains (2));
  REQUIRE_FALSE (m.contains (9));

  REQUIRE (m.contains (1));
  REQUIRE (m.contains (3));
  REQUIRE (m.contains (5));
  REQUIRE (m.contains (7));
  REQUIRE (m[3] == 9);
}

TEST_CASE ("HashMap eraseIf with no matches [hashmap]") {
  HashMap<int, int> m;
  m[1] = 10;
  m[2] = 20;

  REQUIRE (m.erase_if ([] (const int &, const int &) {
    return false;
  }) == 0u);
  REQUIRE (m.size () == 2u);
  REQUIRE (m.contains (1));
  REQUIRE (m.contains (2));
}

TEST_CASE ("HashMap eraseIf on empty map [hashmap]") {
  HashMap<int, int> m;
  REQUIRE (m.erase_if ([] (const int &, const int &) {
    return true;
  }) == 0u);
}

// probing must reach
TEST_CASE ("HashMap probing visits every slot under total collisions [hashmap]") {
  struct CollidingHash {
    uint32_t operator() (int32_t) const noexcept {
      return 0;
    }
  };

  HashMap<int32_t, int32_t, CollidingHash> m;

  // 11 colliding keys fill table just below load factor
  for (int i = 0; i < 11; ++i) {
    m[i] = i;
  }
  REQUIRE (m.size () == 11u);
  REQUIRE (m.capacity () == 16u);

  // must land in one of the free slots without corrupting the existing ones
  m[100] = 100;
  REQUIRE (m.size () == 12u);

  for (int i = 0; i < 11; ++i) {
    REQUIRE (m[i] == i);
  }
  REQUIRE (m[100] == 100);
}

// operator[] over a
TEST_CASE ("HashMap operator[] on reused slot returns default value [hashmap]") {
  HashMap<int, int> m;
  m[5] = 42;

  REQUIRE (m.erase (5) == 1u);
  REQUIRE (m[5] == 0);
  REQUIRE (m.size () == 1u);
}

// heavy insert/erase
TEST_CASE ("HashMap survives heavy insert/erase churn [hashmap]") {
  HashMap<int, int> m;
  constexpr int kKeys = 64;

  for (int i = 0; i < kKeys; ++i) {
    m[i] = i;
  }
  REQUIRE (m.size () == kKeys);

  for (int cycle = 0; cycle < 100; ++cycle) {
    for (int i = 0; i < kKeys; i += 2) {
      REQUIRE (m.erase (i) == 1u);
    }
    REQUIRE (m.size () == kKeys / 2);

    for (int i = 0; i < kKeys; i += 2) {
      m[i] = i + cycle;
    }
    REQUIRE (m.size () == kKeys);
  }

  for (int i = 0; i < kKeys; ++i) {
    REQUIRE (m.exists (i));
  }
}

// rehash aliasing: arguments may reference our own storage, growth must snapshot them first
// note: exactly 5 entries on a fresh table (capacity 8, trigger at 5) so the next insert grows
TEST_CASE ("HashMap operator[] with aliased key survives rehash [hashmap]") {
  HashMap<String, int> m;

  for (int i = 0; i < 5; ++i) {
    String k;
    k.assignf ("long_key_number_%d_padding", i);
    m[k] = i;
  }
  REQUIRE (m.capacity () == 8u);

  // bind to internal storage, then erase so the reference reads "" (still valid, erase never reallocates)
  const String &aliased = (*m.begin ()).first;
  String victim = aliased;
  m.erase (victim);
  REQUIRE (aliased.empty ());

  m[aliased] = 42; // miss + growth: the old code located and copied from the freed reference here

  REQUIRE (m.size () == 5u);
  REQUIRE (m[""] == 42);

  for (int i = 0; i < 5; ++i) {
    String k;
    k.assignf ("long_key_number_%d_padding", i);

    if (k == victim) {
      continue;
    }
    REQUIRE (m[k] == i);
  }
}

TEST_CASE ("HashMap operator[] rvalue key across growth [hashmap]") {
  HashMap<String, int> m;

  for (int i = 0; i < 5; ++i) {
    String k;
    k.assignf ("long_key_number_%d_padding", i);
    m[k] = i;
  }
  REQUIRE (m.capacity () == 8u);

  m[String ("rvalue_new_key_long_pad")] = 7;

  REQUIRE (m.size () == 6u);
  REQUIRE (m["rvalue_new_key_long_pad"] == 7);
}

TEST_CASE ("HashMap insert with aliased value survives rehash [hashmap]") {
  HashMap<String, String> m;

  for (int i = 0; i < 5; ++i) {
    String k, v;
    k.assignf ("long_key_number_%d_padding", i);
    v.assignf ("long_value_number_%d_padding", i);
    m[k] = ystl::move (v);
  }
  REQUIRE (m.capacity () == 8u);

  String victim_key;
  victim_key.assignf ("long_key_number_%d_padding", 2);
  String expected;
  expected.assignf ("long_value_number_%d_padding", 2);

  String new_key ("brand_new_key_long_padding");
  REQUIRE (m.insert (new_key, m[victim_key])); // miss + growth, the value aliases internal storage

  REQUIRE (m.size () == 6u);
  REQUIRE (m[new_key] == expected);

  for (int i = 0; i < 5; ++i) {
    String k, v;
    k.assignf ("long_key_number_%d_padding", i);
    v.assignf ("long_value_number_%d_padding", i);
    REQUIRE (m[k] == v);
  }
}

TEST_CASE ("HashMap insert rvalue key and value across growth [hashmap]") {
  HashMap<String, String> m;

  for (int i = 0; i < 5; ++i) {
    String k, v;
    k.assignf ("long_key_number_%d_padding", i);
    v.assignf ("long_value_number_%d_padding", i);
    m.insert (k, ystl::move (v));
  }
  REQUIRE (m.capacity () == 8u);

  String victim_key;
  victim_key.assignf ("long_key_number_%d_padding", 4);
  String expected;
  expected.assignf ("long_value_number_%d_padding", 4);

  // fresh rvalue key, value moved out of our own storage: exercises the && snapshot lines
  REQUIRE (m.insert (String ("fresh_key_long_padding"), ystl::move (m[victim_key])));

  REQUIRE (m.size () == 6u);
  REQUIRE (m["fresh_key_long_padding"] == expected);
}
