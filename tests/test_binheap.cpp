// test_binheap.cpp - tests for ystl/binheap.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("BinaryHeap default construction is empty [binheap]") {
  BinaryHeap<int> heap;
  REQUIRE (heap.empty ());
  REQUIRE (heap.size () == 0u);
}

// push / top / pop -
TEST_CASE ("BinaryHeap push maintains heap ordering [binheap]") {
  BinaryHeap<int> heap;
  heap.push (30);
  heap.push (10);
  heap.push (20);

  REQUIRE (heap.size () == 3u);
  REQUIRE (heap.top () == 10); // min-heap
}

TEST_CASE ("BinaryHeap pop removes the minimum element [binheap]") {
  BinaryHeap<int> heap;
  heap.push (5);
  heap.push (1);
  heap.push (3);
  heap.push (2);
  heap.push (4);

  REQUIRE (heap.pop () == 1);
  REQUIRE (heap.pop () == 2);
  REQUIRE (heap.pop () == 3);
  REQUIRE (heap.pop () == 4);
  REQUIRE (heap.pop () == 5);
  REQUIRE (heap.empty ());
}

TEST_CASE ("BinaryHeap pop on single element empties the heap [binheap]") {
  BinaryHeap<int> heap;
  heap.push (42);
  REQUIRE (heap.pop () == 42);
  REQUIRE (heap.empty ());
}

// emplace
TEST_CASE ("BinaryHeap emplace constructs element in place [binheap]") {
  BinaryHeap<int> heap;
  heap.emplace (100);
  heap.emplace (50);
  heap.emplace (75);

  REQUIRE (heap.top () == 50);
  REQUIRE (heap.size () == 3u);
}

// clear
TEST_CASE ("BinaryHeap clear empties all elements [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);
  heap.push (3);
  heap.clear ();

  REQUIRE (heap.empty ());
  REQUIRE (heap.size () == 0u);
}

// move constructor
TEST_CASE ("BinaryHeap move constructor transfers state [binheap]") {
  BinaryHeap<int> a;
  a.push (3);
  a.push (1);
  a.push (2);

  BinaryHeap<int> b (ystl::move (a));
  REQUIRE (b.size () == 3u);
  REQUIRE (b.top () == 1);
  REQUIRE (a.empty ());
}

// large insertion test
TEST_CASE ("BinaryHeap push lvalue copies and leaves original intact [binheap]") {
  // lvalue push must copy and leave caller value intact
  BinaryHeap<int> heap;
  int s = 42;
  heap.push (s); // lvalue - must copy, not move
  REQUIRE (s == 42); // original must be unchanged
  REQUIRE (heap.top () == 42);
}

TEST_CASE ("BinaryHeap push rvalue moves efficiently [binheap]") {
  BinaryHeap<int> heap;
  heap.push (100); // rvalue - must move (not copy)
  REQUIRE (heap.top () == 100);
}

// large insertion test
TEST_CASE ("BinaryHeap sorts elements extracted in order [binheap]") {
  BinaryHeap<int> heap;
  const int values[] = { 9, 4, 7, 1, 8, 3, 6, 2, 5 };
  for (auto v : values) {
    heap.push (v);
  }
  REQUIRE (heap.size () == 9u);

  int prev = heap.pop ();
  while (!heap.empty ()) {
    int cur = heap.pop ();
    REQUIRE (prev <= cur);
    prev = cur;
  }
}

// iterator - basic
TEST_CASE ("BinaryHeap iteration visits all elements [binheap]") {
  BinaryHeap<int> heap;
  heap.push (10);
  heap.push (20);
  heap.push (30);

  int count = 0;
  for (int v : heap) {
    (void)v;
    ++count;
  }
  REQUIRE (count == 3);
}

TEST_CASE ("BinaryHeap iteration works with empty heap [binheap]") {
  BinaryHeap<int> heap;
  int count = 0;
  for (int v : heap) {
    (void)v;
    ++count;
  }
  REQUIRE (count == 0);
}

TEST_CASE ("BinaryHeap begin/end return valid iterators [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);

  auto it = heap.begin ();
  REQUIRE (*it == 1);
  ++it;
  REQUIRE (*it == 2);
  ++it;
  REQUIRE (it == heap.end ());
}

// iterator -
TEST_CASE ("BinaryHeap const_iterator iterates all elements [binheap]") {
  BinaryHeap<int> heap;
  heap.push (5);
  heap.push (10);
  heap.push (15);

  const auto &ch = heap;
  int count = 0;
  for (int v : ch) {
    (void)v;
    ++count;
  }
  REQUIRE (count == 3);
}

TEST_CASE ("BinaryHeap cbegin/cend work correctly [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);
  heap.push (3);

  int sum = 0;
  for (auto it = heap.cbegin (); it != heap.cend (); ++it) {
    sum += *it;
  }
  REQUIRE (sum == 6);
}

// iterator -
TEST_CASE ("BinaryHeap iterator pre-increment advances correctly [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);
  heap.push (3);

  auto it = heap.begin ();
  REQUIRE (*++it == 2);
  REQUIRE (*++it == 3);
}

TEST_CASE ("BinaryHeap iterator post-increment returns old position [binheap]") {
  BinaryHeap<int> heap;
  heap.push (10);
  heap.push (20);

  auto it = heap.begin ();
  auto old = it++;
  REQUIRE (*old == 10);
  REQUIRE (*it == 20);
}

TEST_CASE ("BinaryHeap iterator pre-decrement works [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);
  heap.push (3);

  auto it = heap.end ();
  --it;
  REQUIRE (*it == 3);
  --it;
  REQUIRE (*it == 2);
  --it;
  REQUIRE (*it == 1);
}

TEST_CASE ("BinaryHeap iterator post-decrement works [binheap]") {
  BinaryHeap<int> heap;
  heap.push (100);
  heap.push (200);

  auto it = heap.end ();
  --it;
  auto old = it--;
  REQUIRE (*old == 200);
  REQUIRE (*it == 100);
}

// iterator - comparison
TEST_CASE ("BinaryHeap iterator equality comparison works [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);

  auto it1 = heap.begin ();
  auto it2 = heap.begin ();
  REQUIRE (it1 == it2);

  ++it1;
  REQUIRE_FALSE (it1 == it2);
}

TEST_CASE ("BinaryHeap iterator inequality comparison works [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);

  auto it1 = heap.begin ();
  auto it2 = heap.end ();
  REQUIRE (it1 != it2);
}

// iterator - modify
TEST_CASE ("BinaryHeap iterator allows modification of elements [binheap]") {
  BinaryHeap<int> heap;
  heap.push (1);
  heap.push (2);
  heap.push (3);

  for (int &v : heap) {
    v *= 10;
  }

  auto it = heap.begin ();
  REQUIRE (*it == 10);
  ++it;
  REQUIRE (*it == 20);
  ++it;
  REQUIRE (*it == 30);
}

// pop into output variable
TEST_CASE ("BinaryHeap pop into out-param returns minimum [binheap]") {
  BinaryHeap<int> heap;
  heap.push (2);
  heap.push (1);

  int out = heap.top ();
  heap.pop (out);
  REQUIRE (out == 1);
  REQUIRE (heap.top () == 2);
}
