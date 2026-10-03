// test_array.cpp - tests for ystl/array.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Array default construction is empty [array]") {
  Array<int> a;
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
  REQUIRE (a.capacity () == 0u);
}

TEST_CASE ("Array constructed with default value [array]") {
  Array<int> a (5u, 0);
  REQUIRE (a.size () == 5u);
  for (size_t i = 0; i < 5; ++i) {
    REQUIRE (a[i] == 0);
  }
}

TEST_CASE ("Array initializer-list construction [array]") {
  Array<int> a { 10, 20, 30 };
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 20);
  REQUIRE (a[2] == 30);
}

// push / emplace / pop /
TEST_CASE ("Array push and pop round-trip [array]") {
  Array<int> a;
  REQUIRE (a.push (1));
  REQUIRE (a.push (2));
  REQUIRE (a.push (3));
  REQUIRE (a.size () == 3u);

  REQUIRE (a.pop () == 3);
  REQUIRE (a.size () == 2u);
  REQUIRE (a.pop () == 2);
  REQUIRE (a.pop () == 1);
  REQUIRE (a.empty ());
}

TEST_CASE ("Array emplace constructs in place [array]") {
  Array<int> a;
  a.emplace (42);
  a.emplace (100);
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 42);
  REQUIRE (a[1] == 100);
}

TEST_CASE ("Array discard removes last element [array]") {
  Array<int> a { 1, 2, 3 };
  a.discard ();
  REQUIRE (a.size () == 2u);
  REQUIRE (a.last () == 2);
}

// first / last / at /
TEST_CASE ("Array first and last accessors [array]") {
  Array<int> a { 5, 10, 15 };
  REQUIRE (a.first () == 5);
  REQUIRE (a.last () == 15);
}

TEST_CASE ("Array at and operator[] access elements [array]") {
  Array<int> a { 7, 14, 21 };
  REQUIRE (a.at (0) == 7);
  REQUIRE (a.at (1) == 14);
  REQUIRE (a[2] == 21);

  a.at (0) = 99;
  REQUIRE (a[0] == 99);
}

// insert
TEST_CASE ("Array insert at index shifts existing elements [array]") {
  Array<int> a { 1, 3, 4 };
  int v = 2;
  REQUIRE (a.insert (1u, v));
  REQUIRE (a.size () == 4u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 4);
}

TEST_CASE ("Array insert beyond length appends [array]") {
  Array<int> a { 1, 2 };
  int v = 99;
  REQUIRE (a.insert (100u, v));
  REQUIRE (a.last () == 99);
}

// erase
TEST_CASE ("Array erase removes elements and compacts [array]") {
  Array<int> a { 10, 20, 30, 40 };
  REQUIRE (a.erase (1u, 2u)); // remove elements at index 1 and 2
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 40);
}

TEST_CASE ("Array erase rejects out-of-bounds range [array]") {
  Array<int> a { 1, 2, 3 };
  // index + count > length_ must return false without corrupting length
  REQUIRE_FALSE (a.erase (2u, 2u)); // 2+2=4 > 3
  REQUIRE (a.size () == 3u);
  REQUIRE_FALSE (a.erase (3u, 1u)); // 3+1=4 > 3
  REQUIRE (a.size () == 3u);
}

TEST_CASE ("Array shift removes first element [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.shift ());
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 2);
}

TEST_CASE ("Array unshift prepends element [array]") {
  Array<int> a { 2, 3 };
  REQUIRE (a.unshift (1));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
}

// remove
TEST_CASE ("Array remove erases element by value [array]") {
  Array<int> a { 10, 20, 30 };
  REQUIRE (a.remove (a[1])); // remove 20
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 30);
}

// resize / reserve /
TEST_CASE ("Array resize grows with default-constructed elements [array]") {
  Array<int> a;
  REQUIRE (a.resize (4u));
  REQUIRE (a.size () == 4u);
  // default int - whatever the default is, length is correct
}

TEST_CASE ("Array resize shrinks by discarding tail [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.resize (3u));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[2] == 3);
}

TEST_CASE ("Array reserve pre-allocates capacity [array]") {
  Array<int> a;
  REQUIRE (a.reserve (100u));
  REQUIRE (a.capacity () >= 100u);
  REQUIRE (a.size () == 0u); // reserve does not change length
}

TEST_CASE ("Array ensure guarantees at-least length [array]") {
  Array<int> a { 1, 2 };
  REQUIRE (a.ensure (5u));
  REQUIRE (a.capacity () >= 5u);
}

TEST_CASE ("Array shrink reduces capacity to length [array]") {
  Array<int> a;
  a.reserve (200u);
  a.push (1);
  a.push (2);
  REQUIRE (a.capacity () > 2u);

  REQUIRE (a.shrink ());
  REQUIRE (a.capacity () == 2u);
  REQUIRE (a.size () == 2u);
}

// fill / clear / empty
TEST_CASE ("Array fill replaces all elements with given value [array]") {
  Array<int> a { 1, 2, 3 };
  a.fill (7);
  REQUIRE (a.size () == 3u);
  for (size_t i = 0; i < a.size (); ++i) {
    REQUIRE (a[i] == 7);
  }
}

TEST_CASE ("Array clear empties the array [array]") {
  Array<int> a { 1, 2, 3 };
  a.clear ();
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
}

// reverse
TEST_CASE ("Array reverse flips element order [array]") {
  Array<int> a { 1, 2, 3, 4 };
  a.reverse ();
  REQUIRE (a[0] == 4);
  REQUIRE (a[1] == 3);
  REQUIRE (a[2] == 2);
  REQUIRE (a[3] == 1);
}

// extend / assign
TEST_CASE ("Array extend appends elements from another array [array]") {
  Array<int> a { 1, 2 };
  Array<int> b { 3, 4, 5 };
  REQUIRE (a.extend (b));
  REQUIRE (a.size () == 5u);
  REQUIRE (a[4] == 5);
}

TEST_CASE ("Array assign clears then extends [array]") {
  Array<int> a { 1, 2 };
  Array<int> b { 9, 8, 7 };
  REQUIRE (a.assign (ystl::move (b)));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 9);
}

// move constructor / move
TEST_CASE ("Array move constructor transfers ownership [array]") {
  Array<int> a { 1, 2, 3 };
  Array<int> b (ystl::move (a));
  REQUIRE (b.size () == 3u);
  REQUIRE (b[0] == 1);
  REQUIRE (a.empty ());
}

TEST_CASE ("Array move assignment transfers ownership [array]") {
  Array<int> a { 10, 20 };
  Array<int> b;
  b = ystl::move (a);
  REQUIRE (b.size () == 2u);
  REQUIRE (b[0] == 10);
  REQUIRE (a.empty ());
}

// data / begin / end
TEST_CASE ("Array data returns pointer to underlying buffer [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.data () != nullptr);
  REQUIRE (a.data ()[0] == 1);
}

TEST_CASE ("Array range-based for loop iterates all elements [array]") {
  Array<int> a { 10, 20, 30 };
  int sum = 0;
  for (auto v : a) {
    sum += v;
  }
  REQUIRE (sum == 60);
}

// set
TEST_CASE ("Array set assigns value at arbitrary index [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.set (1u, 99));
  REQUIRE (a[1] == 99);
}

// insert with empty
TEST_CASE ("Array insert from empty array is a no-op success [array]") {
  Array<int> a { 1, 2, 3 };
  Array<int> empty;
  REQUIRE (a.insert (1u, static_cast<const decltype (a) &> (empty)));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
}

// insert shift with
TEST_CASE ("Array insert at front with non-trivial type preserves all elements [array]") {
  // string exercises construct versus assign split in shift
  Array<String> a;
  a.push (String ("b"));
  a.push (String ("c"));

  String v ("a");
  REQUIRE (a.insert (0u, v));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == "a");
  REQUIRE (a[1] == "b");
  REQUIRE (a[2] == "c");
}

TEST_CASE ("Array insert multiple elements at front with non-trivial type [array]") {
  Array<String> a;
  a.push (String ("c"));
  a.push (String ("d"));

  String items[2] = { String ("a"), String ("b") };
  REQUIRE (a.insert (0u, items, 2u));
  REQUIRE (a.size () == 4u);
  REQUIRE (a[0] == "a");
  REQUIRE (a[1] == "b");
  REQUIRE (a[2] == "c");
  REQUIRE (a[3] == "d");
}

// smallarray typedef
TEST_CASE ("SmallArray pre-allocates 64-element capacity [array]") {
  SmallArray<int> sa;
  REQUIRE (sa.capacity () >= 64u);
  REQUIRE (sa.empty ());
}

// insert with count=0
TEST_CASE ("Array insert with count=0 returns true without modification [array]") {
  Array<int> a { 1, 2, 3 };
  int items[] = { 99 };
  REQUIRE (a.insert (0u, items, 0u));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
}

// erase with trivially
TEST_CASE ("Array erase on trivially copyable type works correctly [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.erase (1u, 2u));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 4);
  REQUIRE (a[2] == 5);
}

TEST_CASE ("Array erase at start with trivially copyable type [array]") {
  Array<int> a { 10, 20, 30, 40 };
  REQUIRE (a.erase (0u, 2u));
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 30);
  REQUIRE (a[1] == 40);
}

// shuffle
TEST_CASE ("Array shuffle changes element order [array]") {
  Array<int> a;
  for (int i = 0; i < 100; ++i) {
    a.push (i);
  }

  Array<int> original;
  for (int v : a) {
    original.push (v);
  }

  a.shuffle ();
  REQUIRE (a.size () == 100u);

  bool different = false;
  for (size_t i = 0; i < a.size (); ++i) {
    if (a[i] != original[i]) {
      different = true;
      break;
    }
  }
  REQUIRE (different);
}

TEST_CASE ("Array shuffle preserves all elements [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  a.shuffle ();
  REQUIRE (a.size () == 5u);

  int sum = 0;
  for (int v : a) {
    sum += v;
  }
  REQUIRE (sum == 15);
}

// random
TEST_CASE ("Array random returns element from array [array]") {
  Array<int> a { 10, 20, 30, 40, 50 };
  int val = a.random ();
  bool found = false;
  for (int v : a) {
    if (v == val) {
      found = true;
      break;
    }
  }
  REQUIRE (found);
}

TEST_CASE ("Array random on single element returns that element [array]") {
  Array<int> a { 42 };
  REQUIRE (a.random () == 42);
}

TEST_CASE ("Array random const version [array]") {
  const Array<int> a { 1, 2, 3 };
  int val = a.random ();
  REQUIRE ((val >= 1 && val <= 3));
}

// index
TEST_CASE ("Array index returns correct index of element [array]") {
  Array<int> a { 10, 20, 30 };
  REQUIRE (a.index (a[0]) == 0u);
  REQUIRE (a.index (a[1]) == 1u);
  REQUIRE (a.index (a[2]) == 2u);
}

// insert self-insertion
TEST_CASE ("Array insert from self duplicates [array]") {
  Array<int> a { 1, 2, 3 };
  const Array<int> &ref = a;
  REQUIRE (a.insert (1u, ref));
  REQUIRE (a.size () == 6u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 1);
  REQUIRE (a[2] == 2);
  REQUIRE (a[3] == 3);
  REQUIRE (a[4] == 2);
  REQUIRE (a[5] == 3);
}

// length with custom
TEST_CASE ("Array length with custom return type [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.size<int> () == 5);
  REQUIRE (a.size<int32_t> () == 5);
  REQUIRE (a.size<size_t> () == 5u);
}

// shrink edge cases
TEST_CASE ("Array shrink returns false when length equals capacity [array]") {
  Array<int> a;
  a.push (1);
  a.shrink ();
  REQUIRE_FALSE (a.shrink ());
}

TEST_CASE ("Array shrink releases empty storage [array]") {
  Array<int> a;
  a.reserve (10u);
  a.clear ();
  REQUIRE (a.shrink ());
  REQUIRE (a.capacity () == 0u);
}

// insert with nullptr
TEST_CASE ("Array insert with nullptr returns false [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE_FALSE (a.insert (0u, static_cast<int *> (nullptr), 5u));
  REQUIRE (a.size () == 3u);
}

// move assignment
TEST_CASE ("Array move assignment self-assignment is safe [array]") {
  Array<int> a { 1, 2, 3 };
  a = ystl::move (a);
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
}

// ReservePolicy::Proportional
TEST_CASE ("Array with ReservePolicy::Proportional grows correctly [array]") {
  Array<int, ReservePolicy::Proportional> a;
  for (int i = 0; i < 20; ++i) {
    REQUIRE (a.push (i));
  }
  REQUIRE (a.size () == 20u);
  for (int i = 0; i < 20; ++i) {
    REQUIRE (a[i] == i);
  }
}

TEST_CASE ("Array with ReservePolicy::Proportional reserve [array]") {
  Array<int, ReservePolicy::Proportional> a;
  REQUIRE (a.reserve (10u));
  REQUIRE (a.capacity () >= 10u);
  REQUIRE (a.size () == 0u);
}

// array with initial size
TEST_CASE ("Array with initial size template parameter pre-allocates [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 32> a;
  REQUIRE (a.capacity () >= 32u);
  REQUIRE (a.empty ());
}

// erase with non-trivial
TEST_CASE ("Array erase with non-trivial type calls destructors [array]") {
  Array<String> a;
  a.push (String ("one"));
  a.push (String ("two"));
  a.push (String ("three"));
  a.push (String ("four"));

  REQUIRE (a.erase (1u, 2u));
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == "one");
  REQUIRE (a[1] == "four");
}

// find / contains
TEST_CASE ("Array find returns pointer to matching element [array]") {
  Array<int> a { 10, 20, 30, 40 };
  int *found = a.find (20);
  REQUIRE (found != nullptr);
  REQUIRE (*found == 20);
}

TEST_CASE ("Array find returns nullptr for non-existent element [array]") {
  Array<int> a { 10, 20, 30 };
  REQUIRE (a.find (99) == nullptr);
}

TEST_CASE ("Array find const version [array]") {
  const Array<int> a { 5, 10, 15 };
  const int *found = a.find (10);
  REQUIRE (found != nullptr);
  REQUIRE (*found == 10);
  REQUIRE (a.find (99) == nullptr);
}

TEST_CASE ("Array contains returns true for existing element [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.contains (3));
  REQUIRE (a.contains (1));
  REQUIRE (a.contains (5));
}

TEST_CASE ("Array contains returns false for non-existent element [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE_FALSE (a.contains (99));
  REQUIRE_FALSE (a.contains (0));
}

// const accessors: first,
TEST_CASE ("Array const first returns reference [array]") {
  const Array<int> a { 42, 100 };
  REQUIRE (a.first () == 42);
}

TEST_CASE ("Array const last returns reference [array]") {
  const Array<int> a { 1, 2, 3 };
  REQUIRE (a.last () == 3);
}

TEST_CASE ("Array const data returns pointer [array]") {
  const Array<int> a { 7, 14, 21 };
  const int *ptr = a.data ();
  REQUIRE (ptr != nullptr);
  REQUIRE (ptr[0] == 7);
  REQUIRE (ptr[1] == 14);
  REQUIRE (ptr[2] == 21);
}

// sbo tests
TEST_CASE ("Array with SBO starts with inline storage [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 16> a;
  REQUIRE (a.capacity () == 16u);
  REQUIRE (a.empty ());

  // push elements within sbo capacity
  for (int i = 0; i < 16; ++i) {
    REQUIRE (a.push (i));
  }
  REQUIRE (a.size () == 16u);
  REQUIRE (a.capacity () == 16u); // should not have grown
}

TEST_CASE ("Array with SBO transitions to heap when exceeding capacity [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 4> a;
  REQUIRE (a.capacity () == 4u);

  for (int i = 0; i < 4; ++i) {
    REQUIRE (a.push (i));
  }
  REQUIRE (a.size () == 4u);
  REQUIRE (a.capacity () == 4u);

  // this should trigger heap allocation
  REQUIRE (a.push (99));
  REQUIRE (a.size () == 5u);
  REQUIRE (a.capacity () > 4u); // should have grown
  REQUIRE (a.last () == 99);
}

TEST_CASE ("Array with SBO move constructor from SBO source [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 16> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.capacity () == 16u); // still in sbo

  Array<int, ReservePolicy::PowerOfTwo, 16> b (ystl::move (a));
  REQUIRE (b.size () == 5u);
  REQUIRE (b[0] == 1);
  REQUIRE (b[4] == 5);
  REQUIRE (a.empty ()); // source should be empty after move
}

TEST_CASE ("Array with SBO move constructor from heap source [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 4> a;
  for (int i = 0; i < 10; ++i) {
    a.push (i);
  }
  REQUIRE (a.capacity () > 4u); // on heap

  Array<int, ReservePolicy::PowerOfTwo, 4> b (ystl::move (a));
  REQUIRE (b.size () == 10u);
  REQUIRE (b[0] == 0);
  REQUIRE (b[9] == 9);
  REQUIRE (a.empty ());
}

TEST_CASE ("Array with SBO move assignment from SBO source [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 16> a { 10, 20, 30 };
  Array<int, ReservePolicy::PowerOfTwo, 16> b;

  b = ystl::move (a);
  REQUIRE (b.size () == 3u);
  REQUIRE (b[0] == 10);
  REQUIRE (b[1] == 20);
  REQUIRE (b[2] == 30);
  REQUIRE (a.empty ());
}

TEST_CASE ("Array with SBO shrink returns to SBO [array]") {
  Array<int, ReservePolicy::PowerOfTwo, 8> a;

  // fill beyond sbo capacity to force heap allocation
  for (int i = 0; i < 20; ++i) {
    a.push (i);
  }
  REQUIRE (a.capacity () > 8u); // on heap

  // shrink down
  for (int i = 0; i < 15; ++i) {
    a.discard ();
  }
  REQUIRE (a.size () == 5u);

  // shrink should bring back to sbo
  REQUIRE (a.shrink ());
  REQUIRE (a.capacity () == 8u); // back to sbo capacity
}

TEST_CASE ("SmallArray uses SBO by default [array]") {
  SmallArray<int, 32> sa;
  REQUIRE (sa.capacity () >= 32u);

  // fill within sbo capacity
  for (int i = 0; i < 32; ++i) {
    sa.push (i);
  }
  REQUIRE (sa.size () == 32u);
  REQUIRE (sa[31] == 31);
}

// array constructor
TEST_CASE ("Array with size only constructor reserves capacity [array]") {
  Array<int> a (100u);
  REQUIRE (a.capacity () >= 100u);
  REQUIRE (a.size () == 0u); // length is 0, just capacity reserved
}

// shuffle edge cases
TEST_CASE ("Array shuffle on empty array is safe [array]") {
  Array<int> a;
  a.shuffle (); // should not crash
  REQUIRE (a.empty ());
}

TEST_CASE ("Array shuffle on single element is safe [array]") {
  Array<int> a { 42 };
  a.shuffle (); // should not crash
  REQUIRE (a.size () == 1u);
  REQUIRE (a[0] == 42);
}

// assign from lvalue
TEST_CASE ("Array assign from lvalue clears and extends [array]") {
  Array<int> a { 1, 2 };
  Array<int> b { 9, 8, 7 };

  REQUIRE (a.assign (b)); // lvalue, not move
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 9);
  REQUIRE (a[1] == 8);
  REQUIRE (a[2] == 7);
  REQUIRE (b.size () == 3u); // source unchanged (lvalue)
}

// capacity behavior
TEST_CASE ("Array capacity grows with push [array]") {
  Array<int> a;
  size_t prev_cap = a.capacity ();

  for (int i = 0; i < 100; ++i) {
    REQUIRE (a.push (i));
    REQUIRE (a.capacity () >= prev_cap);
    prev_cap = a.capacity ();
  }
  REQUIRE (a.size () == 100u);
}

TEST_CASE ("Array capacity unchanged by resize shrink [array]") {
  Array<int> a;
  a.reserve (100u);
  size_t cap = a.capacity ();

  for (int i = 0; i < 50; ++i) {
    a.push (i);
  }

  REQUIRE (a.resize (20u));
  REQUIRE (a.size () == 20u);
  REQUIRE (a.capacity () == cap); // capacity unchanged
}

// erase edge cases
TEST_CASE ("Array erase all elements [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.erase (0u, 5u));
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
}

TEST_CASE ("Array erase single element at end [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.erase (4u, 1u));
  REQUIRE (a.size () == 4u);
  REQUIRE (a.last () == 4);
}

TEST_CASE ("Array erase count zero is no-op [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.erase (1u, 0u));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
}

// reverse edge cases
TEST_CASE ("Array reverse on empty array is safe [array]") {
  Array<int> a;
  a.reverse ();
  REQUIRE (a.empty ());
}

TEST_CASE ("Array reverse on single element is no-op [array]") {
  Array<int> a { 42 };
  a.reverse ();
  REQUIRE (a.size () == 1u);
  REQUIRE (a[0] == 42);
}

TEST_CASE ("Array reverse on odd length array [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  a.reverse ();
  REQUIRE (a[0] == 5);
  REQUIRE (a[1] == 4);
  REQUIRE (a[2] == 3); // middle element stays
  REQUIRE (a[3] == 2);
  REQUIRE (a[4] == 1);
}

// set beyond current
TEST_CASE ("Array set beyond length grows array [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.set (10u, 99));
  REQUIRE (a.size () == 11u);
  REQUIRE (a[10] == 99);
  // elements between old length and new index should be default-initialized
}

// unshift edge cases
TEST_CASE ("Array unshift to empty array [array]") {
  Array<int> a;
  REQUIRE (a.unshift (42));
  REQUIRE (a.size () == 1u);
  REQUIRE (a[0] == 42);
}

// remove non-existent
TEST_CASE ("Array erase out of range returns false [array]") {
  Array<int> a { 1, 2, 3 };

  // erase is identity based and rejects foreign objects
  REQUIRE_FALSE (a.erase (99u, 1));
  REQUIRE (a.size () == 3u); // array unchanged
}

// emplace with multiple
TEST_CASE ("Array emplace with String type [array]") {
  Array<String> a;
  a.emplace ("hello");
  a.emplace ("world");
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == "hello");
  REQUIRE (a[1] == "world");
}

// pop on single element
TEST_CASE ("Array pop on single element results in empty [array]") {
  Array<int> a { 42 };
  REQUIRE (a.pop () == 42);
  REQUIRE (a.empty ());
}

// fill on empty array
TEST_CASE ("Array fill on empty array is safe [array]") {
  Array<int> a;
  a.fill (99); // should not crash
  REQUIRE (a.empty ());
}

// extend from empty array
TEST_CASE ("Array extend from empty array [array]") {
  Array<int> a { 1, 2, 3 };
  Array<int> empty;
  REQUIRE (a.extend (empty));
  REQUIRE (a.size () == 3u); // unchanged
}

// ensure when already
TEST_CASE ("Array ensure returns true when length already sufficient [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  REQUIRE (a.ensure (3u)); // already have 5 elements
  REQUIRE (a.size () == 5u);
}

// findif
TEST_CASE ("Array findIf returns first match [array]") {
  Array<int> a { 3, 7, 11, 7 };

  auto *first = a.find_if ([] (int value) {
    return (value % 7) == 0;
  });

  REQUIRE (first != nullptr);
  REQUIRE (*first == 7);
  REQUIRE (first == &a[1]); // first match, not the later duplicate
}

TEST_CASE ("Array findIf with no match returns nullptr [array]") {
  const Array<int> a { 2, 4, 6 };

  REQUIRE (a.find_if ([] (int value) {
    return (value % 2) != 0;
  }) == nullptr);
}

TEST_CASE ("Array findIf works on const arrays [array]") {
  const Array<int> a { 1, 2, 3 };

  const auto *found = a.find_if ([] (int value) {
    return value == 2;
  });

  REQUIRE (found != nullptr);
  REQUIRE (*found == 2);
}

// self move assignment
TEST_CASE ("Array self move assignment is safe [array]") {
  Array<int> a { 1, 2, 3, 4, 5 };
  Array<int> &ref = a;
  a = ystl::move (ref);
  REQUIRE (a.size () == 5u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[4] == 5);
}

// eraseif
TEST_CASE ("Array eraseIf removes matching elements [array]") {
  Array<int> a { 1, 2, 3, 4, 5, 6 };
  const auto removed = a.erase_if ([] (int value) {
    return (value % 2) == 0;
  });

  REQUIRE (removed == 3u);
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 3);
  REQUIRE (a[2] == 5);
}

TEST_CASE ("Array eraseIf with no matches [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.erase_if ([] (int) {
    return false;
  }) == 0u);
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[2] == 3);
}

TEST_CASE ("Array eraseIf removing everything [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.erase_if ([] (int) {
    return true;
  }) == 3u);
  REQUIRE (a.empty ());
}

TEST_CASE ("Array eraseIf removes only leading elements [array]") {
  Array<int> a { 1, 2, 3, 4 };
  const auto removed = a.erase_if ([] (int value) {
    return value < 3;
  });

  REQUIRE (removed == 2u);
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 3);
  REQUIRE (a[1] == 4);
}

TEST_CASE ("Array eraseIf on non-trivial type [array]") {
  Array<String> a;
  a.push ("keep");
  a.push ("drop-me");
  a.push ("also-keep");
  a.push ("drop-me-too");
  a.push ("final-keep");

  const auto removed = a.erase_if ([] (const String &str) {
    return StringRef (str).starts_with ("drop");
  });

  REQUIRE (removed == 2u);
  REQUIRE (a.size () == 3u);
  REQUIRE (StringRef (a[0]) == "keep");
  REQUIRE (StringRef (a[1]) == "also-keep");
  REQUIRE (StringRef (a[2]) == "final-keep");
}

TEST_CASE ("Array eraseIf handles move-only types without assignment [array]") {
  // tracks destructor calls to prove each element is destroyed exactly once
  struct MoveOnly {
    int value = -1;
    int *destroy_count = nullptr;

    MoveOnly () = default;
    MoveOnly (int v, int *counter) : value (v), destroy_count (counter) {}

    MoveOnly (const MoveOnly &) = delete;
    MoveOnly &operator= (const MoveOnly &) = delete;
    MoveOnly &operator= (MoveOnly &&) = delete; // eraseif must not need assignment

    MoveOnly (MoveOnly &&rhs) noexcept : value (rhs.value), destroy_count (rhs.destroy_count) {
      rhs.destroy_count = nullptr; // moved-from element must not double-destroy
    }

    ~MoveOnly () {
      if (destroy_count) {
        ++*destroy_count;
      }
    }
  };

  int destroyed[5] = {};

  size_t removed = 0;

  {
    Array<MoveOnly> arr;

    for (int i = 0; i < 5; ++i) {
      arr.emplace (i, &destroyed[i]);
    }

    // drop the even values; compacting must move-construct into dead slots only
    removed = arr.erase_if ([] (const MoveOnly &m) {
      return (m.value % 2) == 0;
    });

    REQUIRE (removed == 3u);
    REQUIRE (arr.size () == 2u);
    REQUIRE (arr[0].value == 1);
    REQUIRE (arr[1].value == 3);
  }

  for (int i = 0; i < 5; ++i) {
    REQUIRE (destroyed[i] == 1); // exactly-once destruction, no leaks, no double-destroy
  }
}

// insert must copy, not
TEST_CASE ("Array insert from const Array copies without disturbing source [array]") {
  Array<String> a, b;
  b.push (String ("x"));
  b.push (String ("y"));

  const Array<String> &cb = b;
  REQUIRE (a.insert (0u, cb));
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == "x");
  REQUIRE (a[1] == "y");
  REQUIRE (b.size () == 2u); // source intact, not moved-from
  REQUIRE (b[0] == "x");
  REQUIRE (b[1] == "y");
}

// single-element
TEST_CASE ("Array insert and unshift copy lvalue sources [array]") {
  Array<String> a;
  String s ("hello");

  REQUIRE (a.insert (0u, s));
  REQUIRE (s == "hello"); // lvalue must not be moved from
  REQUIRE (a[0] == "hello");

  String u ("world");
  REQUIRE (a.unshift (u));
  REQUIRE (u == "world"); // lvalue must not be moved from
  REQUIRE (a[0] == "world");
  REQUIRE (a[1] == "hello");

  REQUIRE (a.insert (0u, String ("tmp"))); // rvalue still accepted
  REQUIRE (a[0] == "tmp");
}

// remove with a foreign
TEST_CASE ("Array remove with foreign reference returns false [array]") {
  Array<int> a { 10, 20, 30 };
  int foreign = 20;

  REQUIRE_FALSE (a.remove (foreign));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 20);
  REQUIRE (a[2] == 30);

  REQUIRE (a.remove (a[1])); // own element still removed by identity
  REQUIRE (a.size () == 2u);
  REQUIRE (a[0] == 10);
  REQUIRE (a[1] == 30);
}

// push/set accept
TEST_CASE ("Array push and set support move-only types [array]") {
  Array<UniquePtr<int>> a;
  auto p = ystl::make_unique<int> (42);

  REQUIRE (a.push (ystl::move (p)));
  REQUIRE (a.size () == 1u);
  REQUIRE (*a[0] == 42);

  auto q = ystl::make_unique<int> (7);
  REQUIRE (a.set (0u, ystl::move (q)));
  REQUIRE (*a[0] == 7);
}

TEST_CASE ("Array reserve zero never allocates [array]") {
  Array<int> a;
  REQUIRE (a.reserve (0u));
  REQUIRE (a.capacity () == 0u);

  Array<int> b (0u);
  REQUIRE (b.capacity () == 0u);

  Array<int> c { 1, 2, 3 };
  const auto cap = c.capacity ();
  REQUIRE (c.reserve (0u));
  REQUIRE (c.capacity () == cap);
}

TEST_CASE ("Array erase zero count is a no-op [array]") {
  Array<String> a { String ("alpha"), String ("beta"), String ("gamma") };
  REQUIRE (a.erase (1u, 0u));
  REQUIRE (a.size () == 3u);
  REQUIRE (a[0] == "alpha");
  REQUIRE (a[1] == "beta");
  REQUIRE (a[2] == "gamma");
}

TEST_CASE ("Array insert supports move-only types [array]") {
  Array<UniquePtr<int>> a;
  auto p = ystl::make_unique<int> (1);
  auto q = ystl::make_unique<int> (2);

  REQUIRE (a.push (ystl::move (p)));
  REQUIRE (a.insert (0u, ystl::move (q)));
  REQUIRE (a.size () == 2u);
  REQUIRE (*a[0] == 2);
  REQUIRE (*a[1] == 1);
}

TEST_CASE ("Array move is unconditionally noexcept like vector [array]") {
  struct ThrowMove {
    ThrowMove () = default;
    ThrowMove (ThrowMove &&) {}
  };

  // unconditional on purpose: conditional would cycle on recursive types (array of self)
  static_assert (noexcept (Array<int> (ystl::declval<Array<int> &&> ())));
  static_assert (noexcept (Array<ThrowMove> (ystl::declval<Array<ThrowMove> &&> ())));
}

// move/copy counter for assign tests, file scope (no static members in local structs)
struct AssignCounted {
  static inline int copies = 0;
  static inline int moves = 0;

  int value = 0;

  AssignCounted () = default;
  explicit AssignCounted (int v) : value (v) {}
  AssignCounted (const AssignCounted &other) : value (other.value) {
    ++copies;
  }
  AssignCounted (AssignCounted &&other) noexcept : value (other.value) {
    ++moves;
  }
  AssignCounted &operator= (const AssignCounted &) = default;
};

TEST_CASE ("Array assign moves from rvalues and copies lvalues [array]") {
  Array<AssignCounted> src;
  src.emplace (1);
  src.emplace (2);

  AssignCounted::copies = 0;
  AssignCounted::moves = 0;

  Array<AssignCounted> moved;
  REQUIRE (moved.assign (ystl::move (src)));
  REQUIRE (AssignCounted::moves == 2);
  REQUIRE (AssignCounted::copies == 0);
  REQUIRE (moved.size () == 2u);
  REQUIRE (moved[0].value == 1);

  AssignCounted::copies = 0;
  AssignCounted::moves = 0;

  Array<AssignCounted> copied;
  REQUIRE (copied.assign (moved));
  REQUIRE (AssignCounted::copies == 2);
  REQUIRE (AssignCounted::moves == 0);
}

TEST_CASE ("Array self insert duplicates through the stash [array]") {
  Array<int> a { 1, 2, 3 };
  REQUIRE (a.insert (1u, a));
  REQUIRE (a.size () == 6u);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 1);
  REQUIRE (a[2] == 2);
  REQUIRE (a[3] == 3);
  REQUIRE (a[4] == 2);
  REQUIRE (a[5] == 3);
}
