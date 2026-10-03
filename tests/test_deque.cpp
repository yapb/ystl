// test_deque.cpp - tests for ystl/deque.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction / empty
TEST_CASE ("Deque default construction is empty [deque]") {
  Deque<int> d;
  REQUIRE (d.empty ());
  REQUIRE (d.size () == 0u);
}

// emplacelast /
TEST_CASE ("Deque emplaceLast appends to the back [deque]") {
  Deque<int> d;
  d.emplace_last (10);
  d.emplace_last (20);
  d.emplace_last (30);

  REQUIRE (d.size () == 3u);
  REQUIRE (d.front () == 10);
  REQUIRE (d.last () == 30);
}

TEST_CASE ("Deque emplaceFront prepends to the front [deque]") {
  Deque<int> d;
  d.emplace_front (30);
  d.emplace_front (20);
  d.emplace_front (10);

  REQUIRE (d.size () == 3u);
  REQUIRE (d.front () == 10);
  REQUIRE (d.last () == 30);
}

TEST_CASE ("Deque mixed emplace front and back [deque]") {
  Deque<int> d;
  d.emplace_last (2);
  d.emplace_front (1);
  d.emplace_last (3);

  REQUIRE (d.size () == 3u);
  REQUIRE (d.front () == 1);
  REQUIRE (d.last () == 3);
}

// popfront / poplast
TEST_CASE ("Deque popFront removes and returns front element [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  REQUIRE (d.pop_front () == 1);
  REQUIRE (d.size () == 2u);
  REQUIRE (d.front () == 2);
}

TEST_CASE ("Deque popLast removes and returns back element [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  REQUIRE (d.pop_last () == 3);
  REQUIRE (d.size () == 2u);
  REQUIRE (d.last () == 2);
}

// discardfront /
TEST_CASE ("Deque discardFront removes front without returning it [deque]") {
  Deque<int> d;
  d.emplace_last (5);
  d.emplace_last (6);
  d.discard_front ();

  REQUIRE (d.size () == 1u);
  REQUIRE (d.front () == 6);
}

TEST_CASE ("Deque discardLast removes back without returning it [deque]") {
  Deque<int> d;
  d.emplace_last (5);
  d.emplace_last (6);
  d.discard_last ();

  REQUIRE (d.size () == 1u);
  REQUIRE (d.last () == 5);
}

// clear
TEST_CASE ("Deque clear resets length to zero [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.clear ();

  REQUIRE (d.empty ());
  REQUIRE (d.size () == 0u);
}

// move constructor / move
TEST_CASE ("Deque move constructor transfers state [deque]") {
  Deque<int> a;
  a.emplace_last (10);
  a.emplace_last (20);

  Deque<int> b (ystl::move (a));
  REQUIRE (b.size () == 2u);
  REQUIRE (b.front () == 10);
  REQUIRE (a.empty ());
}

TEST_CASE ("Deque move assignment transfers state [deque]") {
  Deque<int> a;
  a.emplace_last (7);
  a.emplace_last (8);

  Deque<int> b;
  b = ystl::move (a);
  REQUIRE (b.size () == 2u);
  REQUIRE (b.last () == 8);
  REQUIRE (a.empty ());
}

// clear with non-trivial
TEST_CASE ("Deque clear calls destructors for non-trivial element type [deque]") {
  int destructions = 0;

  struct DestructCount {
    int *count;
    explicit DestructCount (int *c) : count (c) {}
    ~DestructCount () {
      ++(*count);
    }
  };

  {
    Deque<DestructCount> d;
    d.emplace_last (&destructions);
    d.emplace_last (&destructions);
    d.emplace_last (&destructions);
    REQUIRE (d.size () == 3u);

    d.clear ();
    REQUIRE (d.empty ());
    REQUIRE (destructions == 3); // all 3 must be destructed by clear()
  }
  // d's destructor runs on an already-empty deque - must not double-destruct
  REQUIRE (destructions == 3);
}

TEST_CASE ("Deque is reusable after clear [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.clear ();

  d.emplace_last (10);
  d.emplace_last (20);
  REQUIRE (d.size () == 2u);
  REQUIRE (d.front () == 10);
  REQUIRE (d.last () == 20);
}

// growth - insert many
TEST_CASE ("Deque handles large numbers of front/back insertions correctly [deque]") {
  Deque<int> d;
  for (int i = 0; i < 100; ++i) {
    d.emplace_last (i);
  }
  REQUIRE (d.size () == 100u);
  REQUIRE (d.front () == 0);
  REQUIRE (d.last () == 99);

  for (int i = 0; i < 100; ++i) {
    REQUIRE (d.pop_front () == i);
  }
  REQUIRE (d.empty ());
}

// iterator - basic
TEST_CASE ("Deque iteration visits all elements in order [deque]") {
  Deque<int> d;
  d.emplace_last (10);
  d.emplace_last (20);
  d.emplace_last (30);

  int total = 0;
  int count = 0;
  for (int v : d) {
    total += v;
    ++count;
  }
  REQUIRE (count == 3);
  REQUIRE (total == 60);
}

TEST_CASE ("Deque iteration works with empty deque [deque]") {
  Deque<int> d;
  int count = 0;
  for (int v : d) {
    (void)v;
    ++count;
  }
  REQUIRE (count == 0);
}

TEST_CASE ("Deque begin/end return valid iterators [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);

  auto it = d.begin ();
  REQUIRE (*it == 1);
  ++it;
  REQUIRE (*it == 2);
  ++it;
  REQUIRE (it == d.end ());
}

// iterator -
TEST_CASE ("Deque const_iterator iterates all elements [deque]") {
  Deque<int> d;
  d.emplace_last (5);
  d.emplace_last (10);
  d.emplace_last (15);

  const auto &cd = d;
  int total = 0;
  for (int v : cd) {
    total += v;
  }
  REQUIRE (total == 30);
}

TEST_CASE ("Deque cbegin/cend work correctly [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  int sum = 0;
  for (auto it = d.cbegin (); it != d.cend (); ++it) {
    sum += *it;
  }
  REQUIRE (sum == 6);
}

// iterator -
TEST_CASE ("Deque iterator pre-increment advances correctly [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  auto it = d.begin ();
  REQUIRE (*++it == 2);
  REQUIRE (*++it == 3);
}

TEST_CASE ("Deque iterator post-increment returns old position [deque]") {
  Deque<int> d;
  d.emplace_last (10);
  d.emplace_last (20);

  auto it = d.begin ();
  auto old = it++;
  REQUIRE (*old == 10);
  REQUIRE (*it == 20);
}

TEST_CASE ("Deque iterator pre-decrement works [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  auto it = d.end ();
  --it;
  REQUIRE (*it == 3);
  --it;
  REQUIRE (*it == 2);
  --it;
  REQUIRE (*it == 1);
}

TEST_CASE ("Deque iterator post-decrement works [deque]") {
  Deque<int> d;
  d.emplace_last (100);
  d.emplace_last (200);

  auto it = d.end ();
  --it;
  auto old = it--;
  REQUIRE (*old == 200);
  REQUIRE (*it == 100);
}

// iterator - comparison
TEST_CASE ("Deque iterator equality comparison works [deque]") {
  Deque<int> d;
  d.emplace_last (1);

  auto it1 = d.begin ();
  auto it2 = d.begin ();
  REQUIRE (it1 == it2);

  ++it1;
  REQUIRE_FALSE (it1 == it2);
}

TEST_CASE ("Deque iterator inequality comparison works [deque]") {
  Deque<int> d;
  d.emplace_last (1);

  auto it1 = d.begin ();
  auto it2 = d.end ();
  REQUIRE (it1 != it2);
}

// iterator - wrap-around
TEST_CASE ("Deque iterator works after front insertions cause wrap-around [deque]") {
  Deque<int> d;
  for (int i = 0; i < 20; ++i) {
    d.emplace_last (i);
  }
  for (int i = 0; i < 15; ++i) {
    d.discard_front ();
  }
  for (int i = 0; i < 10; ++i) {
    d.emplace_front (-i);
  }

  int count = 0;
  int expected[] = { -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 15, 16, 17, 18, 19 };
  for (int v : d) {
    REQUIRE (v == expected[count++]);
  }
  REQUIRE (count == 15);
}

TEST_CASE ("Deque iterator survives many grow cycles [deque]") {
  Deque<int> d;
  for (int i = 0; i < 50; ++i) {
    d.emplace_last (i);
  }

  int count = 0;
  int expected = 0;
  for (int v : d) {
    REQUIRE (v == expected++);
    ++count;
  }
  REQUIRE (count == 50);
}

// iterator - modify
TEST_CASE ("Deque iterator allows modification of elements [deque]") {
  Deque<int> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  for (int &v : d) {
    v *= 10;
  }

  REQUIRE (d.front () == 10);
  REQUIRE (d.last () == 30);

  auto it = d.begin ();
  ++it;
  REQUIRE (*it == 20);
}

TEST_CASE ("Deque contains checks membership [deque]") {
  Deque<int32_t> d;
  d.emplace_last (1);
  d.emplace_last (2);
  d.emplace_last (3);

  REQUIRE (d.contains (2));
  REQUIRE (!d.contains (4));

  // check wrap-around case: pop from front, push again
  d.discard_front ();
  d.emplace_last (4);

  REQUIRE (d.contains (4));
  REQUIRE (!d.contains (1));
}
