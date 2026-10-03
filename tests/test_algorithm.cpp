// test_algorithm.cpp - tests for ystl/algorithm.h (fill)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

TEST_CASE ("fill assigns the value to every element [algorithm]") {
  int32_t data[257] {};
  fill (data, 257, -1);

  for (size_t i = 0; i < 257; ++i) {
    REQUIRE (data[i] == -1);
  }
}

TEST_CASE ("fill uses assignment path for non-uniform byte values [algorithm]") {
  constexpr int32_t value = 0x01020304;
  int32_t data[64] {};
  fill (data, 64, value);

  for (size_t i = 0; i < 64; ++i) {
    REQUIRE (data[i] == value);
  }
}

TEST_CASE ("fill handles single-byte types [algorithm]") {
  uint8_t data[128] {};
  fill (data, 128, static_cast<uint8_t> (0xAB));

  for (size_t i = 0; i < 128; ++i) {
    REQUIRE (data[i] == 0xAB);
  }
}

TEST_CASE ("fill converts the value to the element type [algorithm]") {
  int32_t data[16] {};
  fill (data, 16, 7);

  for (size_t i = 0; i < 16; ++i) {
    REQUIRE (data[i] == 7);
  }
}

TEST_CASE ("fill ignores null pointers and zero counts [algorithm]") {
  fill (static_cast<int32_t *> (nullptr), 16, 1);

  int32_t data[8] {};
  fill (data, 0, 7);

  for (size_t i = 0; i < 8; ++i) {
    REQUIRE (data[i] == 0);
  }
}

TEST_CASE ("fill works with containers [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (100));

  fill (container, 42);

  for (size_t i = 0; i < container.size (); ++i) {
    REQUIRE (container[i] == 42);
  }
}

TEST_CASE ("fill works with c arrays via overload [algorithm]") {
  int data[33] {};
  fill (data, 5);

  for (size_t i = 0; i < 33; ++i) {
    REQUIRE (data[i] == 5);
  }
}

TEST_CASE ("fill works with non-trivially-copyable types [algorithm]") {
  struct Widget {
    int value {};
    ~Widget () {}
    Widget &operator= (const Widget &rhs) {
      value = rhs.value;
      return *this;
    }
  };

  Widget data[10] {};
  Widget value {};
  value.value = 9;

  fill (data, 10, value);

  for (size_t i = 0; i < 10; ++i) {
    REQUIRE (data[i].value == 9);
  }
}

TEST_CASE ("fill wipes tables of ULZ size correctly [algorithm]") {
  // sanity check at ulz hash-table scale (512k int32 slots)
  constexpr size_t length = 512 * 1024;
  auto data = make_unique<int32_t[]> (length);
  fill (data.get (), length, -1);

  for (size_t i = 0; i < length; i += 4099) {
    REQUIRE (data[i] == -1);
  }
}

// filln
TEST_CASE ("fillN behaves like fill [algorithm]") {
  int32_t data[16] {};
  fill_n (data, 16, 42);

  for (size_t i = 0; i < 16; ++i) {
    REQUIRE (data[i] == 42);
  }
}

// copy / copyn
TEST_CASE ("copy copies trivially copyable elements [algorithm]") {
  int32_t src[32] {};
  int32_t dest[32] {};

  for (size_t i = 0; i < 32; ++i) {
    src[i] = static_cast<int32_t> (i * 3);
  }
  const auto end = copy (dest, src, 32);

  REQUIRE (end == dest + 32);

  for (size_t i = 0; i < 32; ++i) {
    REQUIRE (dest[i] == src[i]);
  }
}

TEST_CASE ("copy handles non-trivially-copyable types [algorithm]") {
  struct Widget {
    int value {};
    ~Widget () {}
    Widget &operator= (const Widget &rhs) {
      value = rhs.value;
      return *this;
    }
  };

  Widget src[8] {};
  Widget dest[8] {};

  for (size_t i = 0; i < 8; ++i) {
    src[i].value = static_cast<int> (i) + 1;
  }
  copy (dest, src, 8);

  for (size_t i = 0; i < 8; ++i) {
    REQUIRE (dest[i].value == src[i].value);
  }
}

TEST_CASE ("copy is safe for overlapping ranges [algorithm]") {
  // shifting a range backwards within the same buffer
  int32_t data[8] { 0, 1, 2, 3, 4, 5, 6, 7 };
  copy (data, data + 2, 6);

  for (size_t i = 0; i < 6; ++i) {
    REQUIRE (data[i] == static_cast<int32_t> (i + 2));
  }
  REQUIRE (data[6] == 6);
  REQUIRE (data[7] == 7);
}

TEST_CASE ("copyN is an alias for copy [algorithm]") {
  int32_t src[8] { 1, 2, 3, 4, 5, 6, 7, 8 };
  int32_t dest[8] {};

  copy_n (dest, src, 8);

  for (size_t i = 0; i < 8; ++i) {
    REQUIRE (dest[i] == src[i]);
  }
}

// foreach
TEST_CASE ("forEach applies the function to all elements [algorithm]") {
  int32_t data[5] { 1, 2, 3, 4, 5 };

  int32_t sum = 0;
  for_each (data, 5, [&sum] (const int32_t &v) {
    sum += v;
  });

  REQUIRE (sum == 15);
}

TEST_CASE ("forEach works with containers [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (4));

  fill (container, 3);

  int32_t sum = 0;
  for_each (container, [&sum] (const int32_t &v) {
    sum += v;
  });

  REQUIRE (sum == 12);
}

// find / findif
TEST_CASE ("find returns pointer to the first matching element [algorithm]") {
  int32_t data[6] { 4, 8, 15, 16, 23, 42 };

  const auto found = find (data, 6, 15);
  REQUIRE (found != nullptr);
  REQUIRE (*found == 15);
  REQUIRE (found == data + 2);

  const auto missing = find (data, 6, 99);
  REQUIRE (missing == nullptr);
}

TEST_CASE ("find works with const pointers [algorithm]") {
  const int32_t data[4] { 1, 2, 3, 2 };

  const auto found = find (data, 4, 2);
  REQUIRE (found == data + 1);

  const auto missing = find (data, 4, 7);
  REQUIRE (missing == nullptr);
}

TEST_CASE ("find works with containers [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (3));
  container[0] = 10;
  container[1] = 20;
  container[2] = 30;

  const auto found = find (container, 20);
  REQUIRE (found != nullptr);
  REQUIRE (*found == 20);

  const auto missing = find (container, 40);
  REQUIRE (missing == nullptr);
}

TEST_CASE ("findIf returns first element matching the predicate [algorithm]") {
  const int32_t data[6] { 1, 3, 4, 6, 7, 9 };

  const auto even = find_if (data, 6, [] (int32_t v) {
    return (v % 2) == 0;
  });
  REQUIRE (even != nullptr);
  REQUIRE (*even == 4);

  const auto missing = find_if (data, 6, [] (int32_t v) {
    return v > 100;
  });
  REQUIRE (missing == nullptr);
}

// count / countif
TEST_CASE ("count counts equal elements [algorithm]") {
  const int32_t data[8] { 1, 2, 2, 3, 2, 4, 2, 5 };

  REQUIRE (count (data, 8, 2) == 4);
  REQUIRE (count (data, 8, 9) == 0);
}

TEST_CASE ("countIf counts elements matching the predicate [algorithm]") {
  const int32_t data[8] { 1, 2, 2, 3, 2, 4, 2, 5 };

  REQUIRE (count_if (data, 8, [] (int32_t v) {
    return v > 2;
  }) == 3);
  REQUIRE (count_if (data, 8, [] (int32_t) {
    return false;
  }) == 0);
}

// reverse
TEST_CASE ("reverse reverses element order in place [algorithm]") {
  int32_t even[4] { 1, 2, 3, 4 };
  reverse (even, 4);
  REQUIRE (even[0] == 4);
  REQUIRE (even[1] == 3);
  REQUIRE (even[2] == 2);
  REQUIRE (even[3] == 1);

  int32_t odd[5] { 1, 2, 3, 4, 5 };
  reverse (odd, 5);
  REQUIRE (odd[0] == 5);
  REQUIRE (odd[2] == 3);
  REQUIRE (odd[4] == 1);

  int32_t single[1] { 7 };
  reverse (single, 1);
  REQUIRE (single[0] == 7);
}

TEST_CASE ("reverse works with containers and c arrays [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (3));
  container[0] = 1;
  container[1] = 2;
  container[2] = 3;

  reverse (container);
  REQUIRE (container[0] == 3);
  REQUIRE (container[2] == 1);

  int data[3] { 1, 2, 3 };
  reverse (data);
  REQUIRE (data[0] == 3);
  REQUIRE (data[2] == 1);
}

// transform
TEST_CASE ("transform applies unary operation [algorithm]") {
  int32_t src[4] { 1, 2, 3, 4 };
  int32_t dest[4] {};

  transform (dest, src, 4, [] (int32_t v) {
    return v * v;
  });

  REQUIRE (dest[0] == 1);
  REQUIRE (dest[1] == 4);
  REQUIRE (dest[2] == 9);
  REQUIRE (dest[3] == 16);
}

TEST_CASE ("transform supports in-place modification [algorithm]") {
  int32_t data[3] { 1, 2, 3 };

  transform (data, data, 3, [] (int32_t v) {
    return v + 10;
  });

  REQUIRE (data[0] == 11);
  REQUIRE (data[1] == 12);
  REQUIRE (data[2] == 13);
}

TEST_CASE ("transform applies binary operation [algorithm]") {
  int32_t a[3] { 1, 2, 3 };
  int32_t b[3] { 10, 20, 30 };
  int32_t dest[3] {};

  transform (dest, a, b, 3, [] (int32_t x, int32_t y) {
    return x * y;
  });

  REQUIRE (dest[0] == 10);
  REQUIRE (dest[1] == 40);
  REQUIRE (dest[2] == 90);
}

// minelement / maxelement
TEST_CASE ("minElement and maxElement find extremes [algorithm]") {
  const int32_t data[6] { 5, -3, 12, 0, 12, 7 };

  const auto min = min_element (data, 6);
  REQUIRE (min != nullptr);
  REQUIRE (*min == -3);

  const auto max = max_element (data, 6);
  REQUIRE (max != nullptr);
  REQUIRE (*max == 12);
  // first occurrence wins
  REQUIRE (max == data + 2);
}

TEST_CASE ("minElement and maxElement return nullptr for empty ranges [algorithm]") {
  REQUIRE (min_element (static_cast<const int32_t *> (nullptr), 0) == nullptr);
  REQUIRE (max_element (static_cast<const int32_t *> (nullptr), 0) == nullptr);
}

TEST_CASE ("minElement and maxElement respect custom comparators [algorithm]") {
  const int32_t data[5] { 5, -3, 12, 0, 7 };

  // with greater, "smallest" is the largest value
  const auto inverted_min = min_element (data, 5, Greater<int32_t> {});
  REQUIRE (*inverted_min == 12);

  const auto inverted_max = max_element (data, 5, Greater<int32_t> {});
  REQUIRE (*inverted_max == -3);
}

TEST_CASE ("minElement and maxElement work with containers [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (4));
  container[0] = 9;
  container[1] = -1;
  container[2] = 4;
  container[3] = 2;

  REQUIRE (*min_element (container) == -1);
  REQUIRE (*max_element (container) == 9);
}

// lowerbound / upperbound
TEST_CASE ("lowerBound finds first element not less than value [algorithm]") {
  const int32_t data[7] { 1, 3, 3, 5, 7, 9, 11 };

  const auto lower = lower_bound (data, 7, 5);
  REQUIRE (lower != nullptr);
  REQUIRE (*lower == 5);
  REQUIRE (lower == data + 3);

  // duplicated values: points at the first occurrence
  const auto dup = lower_bound (data, 7, 3);
  REQUIRE (dup == data + 1);

  // value smaller than everything: range start
  const auto front = lower_bound (data, 7, 0);
  REQUIRE (front == data);

  // value larger than everything: one-past-the-end
  const auto past_end = lower_bound (data, 7, 12);
  REQUIRE (past_end == data + 7);
}

TEST_CASE ("upperBound finds first element greater than value [algorithm]") {
  const int32_t data[7] { 1, 3, 3, 5, 7, 9, 11 };

  const auto upper = upper_bound (data, 7, 3);
  REQUIRE (upper != nullptr);
  REQUIRE (*upper == 5);
  REQUIRE (upper == data + 3);

  const auto past_end = upper_bound (data, 7, 11);
  REQUIRE (past_end == data + 7);
}

TEST_CASE ("lowerBound and upperBound support custom comparators [algorithm]") {
  // descending order: compare with greater
  const int32_t data[5] { 11, 9, 7, 5, 3 };

  const auto lower = lower_bound (data, 5, 7, Greater<int32_t> {});
  REQUIRE (*lower == 7);
  REQUIRE (lower == data + 2);

  const auto upper = upper_bound (data, 5, 7, Greater<int32_t> {});
  REQUIRE (*upper == 5);
  REQUIRE (upper == data + 3);
}

TEST_CASE ("lowerBound and upperBound work with containers [algorithm]") {
  Array<int32_t> container;
  REQUIRE (container.resize (5));

  const int32_t sorted[] = { 2, 4, 6, 8, 10 };
  for (size_t i = 0; i < 5; ++i) {
    container[i] = sorted[i];
  }

  REQUIRE (*lower_bound (container, 6) == 6);
  REQUIRE (*upper_bound (container, 6) == 8);
}

// equal
TEST_CASE ("equal compares ranges element-wise [algorithm]") {
  const int32_t a[5] { 1, 2, 3, 4, 5 };
  const int32_t b[5] { 1, 2, 3, 4, 5 };
  const int32_t c[5] { 1, 2, 3, 4, 6 };

  REQUIRE (equal (a, b, 5));
  REQUIRE (!equal (a, c, 5));
  REQUIRE (equal (a, c, 4)); // differ only in the last element
  REQUIRE (equal (a, b, 0)); // empty ranges are always equal
}

TEST_CASE ("equal works with containers [algorithm]") {
  Array<int32_t> lhs;
  Array<int32_t> rhs;
  REQUIRE (lhs.resize (3));
  REQUIRE (rhs.resize (3));

  fill (lhs, 7);
  fill (rhs, 7);
  REQUIRE (equal (lhs, rhs));

  rhs[2] = 8;
  REQUIRE (!equal (lhs, rhs));

  REQUIRE (rhs.resize (4));
  REQUIRE (!equal (lhs, rhs)); // different lengths are never equal
}
