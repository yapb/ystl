// test_span.cpp - tests for ystl/span.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Span default construction is empty [span]") {
  Span<int> s;
  REQUIRE (s.empty ());
  REQUIRE (s.size () == 0);
  REQUIRE (s.size () == 0);
  REQUIRE (s.data () == nullptr);
}

TEST_CASE ("Span from pointer and length [span]") {
  int data[] = { 1, 2, 3, 4, 5 };
  Span<int> s (data, 5);

  REQUIRE (!s.empty ());
  REQUIRE (s.size () == 5);
  REQUIRE (s.data () == data);
}

TEST_CASE ("Span from fixed-size array [span]") {
  int data[] = { 10, 20, 30 };
  Span<int> s = data; // deduction guide picks span<int>

  REQUIRE (s.size () == 3);
  REQUIRE (s[0] == 10);
  REQUIRE (s[2] == 30);
}

TEST_CASE ("Span from container with data() and length() [span]") {
  FixedArray<int, 4> arr {};
  arr[0] = 7;
  arr[3] = 9;

  Span<int> s (arr);
  REQUIRE (s.size () == 4);
  REQUIRE (s[0] == 7);
  REQUIRE (s[3] == 9);
}

TEST_CASE ("Span<const T> from const container [span]") {
  const FixedArray<int, 3> arr { 1, 2, 3 };

  Span<const int> s (arr);
  REQUIRE (s.size () == 3);
  REQUIRE (s[1] == 2);
}

// element access
TEST_CASE ("Span element access and iteration [span]") {
  int data[] = { 1, 2, 3 };
  Span<int> s (data, 3);

  REQUIRE (s.front () == 1);
  REQUIRE (s.back () == 3);
  REQUIRE (s.at (1) == 2);

  int sum = 0;
  for (auto value : s) {
    sum += value;
  }
  REQUIRE (sum == 6);
}

TEST_CASE ("Span allows mutation of underlying data [span]") {
  int data[] = { 1, 2, 3 };
  Span<int> s (data, 3);

  s[1] = 42;
  REQUIRE (data[1] == 42);
}

// subviews
TEST_CASE ("Span first/last [span]") {
  int data[] = { 1, 2, 3, 4, 5 };
  Span<int> s (data, 5);

  REQUIRE (s.first (2).size () == 2);
  REQUIRE (s.first (2)[1] == 2);
  REQUIRE (s.last (2).size () == 2);
  REQUIRE (s.last (2)[0] == 4);

  // clamping
  REQUIRE (s.first (100).size () == 5);
  REQUIRE (s.last (100).size () == 5);
}

TEST_CASE ("Span subspan [span]") {
  int data[] = { 1, 2, 3, 4, 5 };
  Span<int> s (data, 5);

  auto tail = s.subspan (2);
  REQUIRE (tail.size () == 3);
  REQUIRE (tail.front () == 3);

  auto middle = s.subspan (1, 2);
  REQUIRE (middle.size () == 2);
  REQUIRE (middle[0] == 2);
  REQUIRE (middle[1] == 3);

  // out of range offsets produce empty spans
  REQUIRE (s.subspan (10).empty ());
  REQUIRE (s.subspan (10, 2).empty ());
}

// lengthbytes / bytes
TEST_CASE ("Span lengthBytes and bytes view [span]") {
  uint32_t data[] = { 1, 2, 3, 4 };
  Span<uint32_t> s (data, 4);

  REQUIRE (s.length_bytes () == 16);

  auto raw = s.bytes ();
  REQUIRE (raw.size () == 16);
  REQUIRE (raw.data () == reinterpret_cast<const uint8_t *> (data));
}

// interaction with
TEST_CASE ("Span of mutable data from Array-like types [span]") {
  Array<int> arr;
  arr.push (1);
  arr.push (2);
  arr.push (3);

  Span<int> s (arr);
  REQUIRE (s.size () == 3);

  s[0] = 100;
  REQUIRE (arr[0] == 100);
}
