// test_sort.cpp - tests for ystl/sort.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// helpers
static bool is_sorted (const int *data, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    if (data[i] < data[i - 1]) {
      return false;
    }
  }
  return true;
}

// sort with containers
TEST_CASE ("sort empty array is a no-op [sort]") {
  Array<int> a;
  sort (a);
  REQUIRE (a.empty ());
  REQUIRE (a.size () == 0u);
}

TEST_CASE ("sort single element [sort]") {
  Array<int> a { 42 };
  sort (a);
  REQUIRE (a.size () == 1u);
  REQUIRE (a[0] == 42);
}

TEST_CASE ("sort two elements [sort]") {
  Array<int> a { 5, 3 };
  sort (a);
  REQUIRE (a[0] == 3);
  REQUIRE (a[1] == 5);
}

TEST_CASE ("sort two elements already sorted [sort]") {
  Array<int> a { 3, 5 };
  sort (a);
  REQUIRE (a[0] == 3);
  REQUIRE (a[1] == 5);
}

TEST_CASE ("sort ascending default [sort]") {
  Array<int> a { 5, 3, 8, 1, 9, 2, 7, 4, 6 };
  sort (a);
  REQUIRE (a.size () == 9u);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort already sorted array [sort]") {
  Array<int> a { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort reverse sorted array [sort]") {
  Array<int> a { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort all duplicates [sort]") {
  Array<int> a { 7, 7, 7, 7, 7 };
  sort (a);
  REQUIRE (a.size () == 5u);
  for (size_t i = 0; i < a.size (); ++i) {
    REQUIRE (a[i] == 7);
  }
}

TEST_CASE ("sort with some duplicates [sort]") {
  Array<int> a { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort with custom descending comparator [sort]") {
  Array<int> a { 1, 5, 3, 9, 2 };
  sort (a, Greater<int> {});
  REQUIRE (a[0] == 9);
  REQUIRE (a[1] == 5);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 2);
  REQUIRE (a[4] == 1);
}

TEST_CASE ("sort with lambda comparator [sort]") {
  Array<int> a { 10, 30, 20, 50, 40 };
  sort (a, [] (const int &a, const int &b) {
    return a > b;
  });
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] >= a[i + 1]);
  }
}

TEST_CASE ("sort preserves element count [sort]") {
  Array<int> a { 5, 3, 8, 1, 9, 2, 7, 4, 6, 0 };
  const auto len = a.size ();
  sort (a);
  REQUIRE (a.size () == len);
}

// sort with c arrays
TEST_CASE ("sort C array ascending [sort]") {
  int data[] = { 5, 3, 8, 1, 9, 2 };
  sort (data);
  REQUIRE (is_sorted (data, 6));
}

TEST_CASE ("sort C array with custom comparator [sort]") {
  int data[] = { 1, 5, 3, 9, 2 };
  sort (data, Greater<int> {});
  REQUIRE (data[0] == 9);
  REQUIRE (data[4] == 1);
}

// sort with raw pointers
TEST_CASE ("sort raw pointer ascending [sort]") {
  Array<int> buf { 5, 3, 8, 1, 9, 2 };
  sort (buf.data (), buf.size ());
  REQUIRE (is_sorted (buf.data (), buf.size ()));
}

TEST_CASE ("sort raw pointer with custom comparator [sort]") {
  Array<int> buf { 1, 5, 3, 9, 2 };
  sort (buf.data (), buf.size (), Greater<int> {});
  REQUIRE (buf[0] == 9);
  REQUIRE (buf[4] == 1);
}

TEST_CASE ("sort raw pointer count 0 [sort]") {
  Array<int> buf;
  buf.push (1);
  sort (buf.data (), static_cast<size_t> (0));
  REQUIRE (buf[0] == 1);
}

TEST_CASE ("sort raw pointer count 1 [sort]") {
  Array<int> buf;
  buf.push (42);
  sort (buf.data (), static_cast<size_t> (1));
  REQUIRE (buf[0] == 42);
}

// sort with non-trivial
TEST_CASE ("sort Array<String> ascending [sort]") {
  Array<String> a;
  a.push (String ("cherry"));
  a.push (String ("apple"));
  a.push (String ("banana"));

  sort (a, [] (const String &a, const String &b) {
    return strcmp (a.chars (), b.chars ()) < 0;
  });

  REQUIRE (a[0] == "apple");
  REQUIRE (a[1] == "banana");
  REQUIRE (a[2] == "cherry");
}

// sort with larger
TEST_CASE ("sort 100 elements [sort]") {
  Array<int> a;
  int val = 42;
  for (int i = 0; i < 100; ++i) {
    val = (val * 1103515245 + 12345) & 0x7fffffff;
    a.push (val % 1000);
  }

  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort 1000 elements exercises heapsort fallback [sort]") {
  Array<int> a;
  int val = 12345;
  for (int i = 0; i < 1000; ++i) {
    val = (val * 1103515245 + 12345) & 0x7fffffff;
    a.push (val);
  }

  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

// sort with float type
TEST_CASE ("sort floats ascending [sort]") {
  Array<float> a { 3.14f, 1.0f, 2.71f, 0.5f, 1.41f };
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

// boundary: two-element
TEST_CASE ("sort two equal elements [sort]") {
  Array<int> a { 5, 5 };
  sort (a);
  REQUIRE (a[0] == 5);
  REQUIRE (a[1] == 5);
}

// additional edge cases:
TEST_CASE ("sort all same elements large [sort]") {
  Array<int> a;
  for (int i = 0; i < 100; ++i) {
    a.push (42);
  }
  sort (a);
  for (size_t i = 0; i < a.size (); ++i) {
    REQUIRE (a[i] == 42);
  }
}

TEST_CASE ("sort alternating low high pattern [sort]") {
  Array<int> a;
  for (int i = 0; i < 50; ++i) {
    a.push (i);
    a.push (100 - i);
  }
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("sort organ pipe pattern [sort]") {
  Array<int> a;
  // 1,2,3,4,5,4,3,2,1 pattern
  for (int i = 1; i <= 5; ++i)
    a.push (i);
  for (int i = 4; i >= 1; --i)
    a.push (i);
  sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

// bubblesort tests
TEST_CASE ("bubbleSort empty array is a no-op [sort]") {
  Array<int> a;
  bubble_sort (a);
  REQUIRE (a.empty ());
}

TEST_CASE ("bubbleSort single element [sort]") {
  Array<int> a { 42 };
  bubble_sort (a);
  REQUIRE (a.size () == 1u);
  REQUIRE (a[0] == 42);
}

TEST_CASE ("bubbleSort two elements [sort]") {
  Array<int> a { 5, 3 };
  bubble_sort (a);
  REQUIRE (a[0] == 3);
  REQUIRE (a[1] == 5);
}

TEST_CASE ("bubbleSort two elements already sorted [sort]") {
  Array<int> a { 3, 5 };
  bubble_sort (a);
  REQUIRE (a[0] == 3);
  REQUIRE (a[1] == 5);
}

TEST_CASE ("bubbleSort two equal elements [sort]") {
  Array<int> a { 5, 5 };
  bubble_sort (a);
  REQUIRE (a[0] == 5);
  REQUIRE (a[1] == 5);
}

TEST_CASE ("bubbleSort ascending default [sort]") {
  Array<int> a { 5, 3, 8, 1, 9, 2, 7, 4, 6 };
  bubble_sort (a);
  REQUIRE (a.size () == 9u);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort already sorted array [sort]") {
  Array<int> a { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort reverse sorted array [sort]") {
  Array<int> a { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort all duplicates [sort]") {
  Array<int> a { 7, 7, 7, 7, 7 };
  bubble_sort (a);
  REQUIRE (a.size () == 5u);
  for (size_t i = 0; i < a.size (); ++i) {
    REQUIRE (a[i] == 7);
  }
}

TEST_CASE ("bubbleSort with some duplicates [sort]") {
  Array<int> a { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort with custom descending comparator [sort]") {
  Array<int> a { 1, 5, 3, 9, 2 };
  bubble_sort (a, Greater<int> {});
  REQUIRE (a[0] == 9);
  REQUIRE (a[1] == 5);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 2);
  REQUIRE (a[4] == 1);
}

TEST_CASE ("bubbleSort with lambda comparator [sort]") {
  Array<int> a { 10, 30, 20, 50, 40 };
  bubble_sort (a, [] (const int &a, const int &b) {
    return a > b;
  });
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] >= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort preserves element count [sort]") {
  Array<int> a { 5, 3, 8, 1, 9, 2, 7, 4, 6, 0 };
  const auto len = a.size ();
  bubble_sort (a);
  REQUIRE (a.size () == len);
}

// bubblesort with c
TEST_CASE ("bubbleSort C array ascending [sort]") {
  int data[] = { 5, 3, 8, 1, 9, 2 };
  bubble_sort (data);
  REQUIRE (is_sorted (data, 6));
}

TEST_CASE ("bubbleSort C array with custom comparator [sort]") {
  int data[] = { 1, 5, 3, 9, 2 };
  bubble_sort (data, Greater<int> {});
  REQUIRE (data[0] == 9);
  REQUIRE (data[4] == 1);
}

// bubblesort with raw
TEST_CASE ("bubbleSort raw pointer ascending [sort]") {
  Array<int> buf { 5, 3, 8, 1, 9, 2 };
  bubble_sort (buf.data (), buf.size ());
  REQUIRE (is_sorted (buf.data (), buf.size ()));
}

TEST_CASE ("bubbleSort raw pointer with custom comparator [sort]") {
  Array<int> buf { 1, 5, 3, 9, 2 };
  bubble_sort (buf.data (), buf.size (), Greater<int> {});
  REQUIRE (buf[0] == 9);
  REQUIRE (buf[4] == 1);
}

TEST_CASE ("bubbleSort raw pointer count 0 [sort]") {
  Array<int> buf;
  buf.push (1);
  bubble_sort (buf.data (), static_cast<size_t> (0));
  REQUIRE (buf[0] == 1);
}

TEST_CASE ("bubbleSort raw pointer count 1 [sort]") {
  Array<int> buf;
  buf.push (42);
  bubble_sort (buf.data (), static_cast<size_t> (1));
  REQUIRE (buf[0] == 42);
}

// bubblesort with string
TEST_CASE ("bubbleSort Array<String> ascending [sort]") {
  Array<String> a;
  a.push (String ("cherry"));
  a.push (String ("apple"));
  a.push (String ("banana"));

  bubble_sort (a, [] (const String &a, const String &b) {
    return strcmp (a.chars (), b.chars ()) < 0;
  });

  REQUIRE (a[0] == "apple");
  REQUIRE (a[1] == "banana");
  REQUIRE (a[2] == "cherry");
}

// bubblesort larger data
TEST_CASE ("bubbleSort 50 elements [sort]") {
  Array<int> a;
  int val = 42;
  for (int i = 0; i < 50; ++i) {
    val = (val * 1103515245 + 12345) & 0x7fffffff;
    a.push (val % 500);
  }

  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort floats ascending [sort]") {
  Array<float> a { 3.14f, 1.0f, 2.71f, 0.5f, 1.41f };
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort alternating pattern [sort]") {
  Array<int> a;
  for (int i = 0; i < 25; ++i) {
    a.push (i);
    a.push (50 - i);
  }
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

TEST_CASE ("bubbleSort organ pipe pattern [sort]") {
  Array<int> a;
  for (int i = 1; i <= 5; ++i)
    a.push (i);
  for (int i = 4; i >= 1; --i)
    a.push (i);
  bubble_sort (a);
  for (size_t i = 0; i < a.size () - 1; ++i) {
    REQUIRE (a[i] <= a[i + 1]);
  }
}

// bubblesort with
TEST_CASE ("bubbleSort FixedArray ascending [sort]") {
  FixedArray<int, 5> a = { 5, 3, 1, 4, 2 };
  bubble_sort (a);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 4);
  REQUIRE (a[4] == 5);
}

TEST_CASE ("bubbleSort FixedArray with custom comparator [sort]") {
  FixedArray<int, 5> a = { 1, 5, 3, 9, 2 };
  bubble_sort (a, Greater<int> {});
  REQUIRE (a[0] == 9);
  REQUIRE (a[1] == 5);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 2);
  REQUIRE (a[4] == 1);
}

TEST_CASE ("bubbleSort FixedArray already sorted [sort]") {
  FixedArray<int, 4> a = { 1, 2, 3, 4 };
  bubble_sort (a);
  REQUIRE (a[0] == 1);
  REQUIRE (a[1] == 2);
  REQUIRE (a[2] == 3);
  REQUIRE (a[3] == 4);
}

TEST_CASE ("bubbleSort FixedArray all same [sort]") {
  FixedArray<int, 4> a = { 7, 7, 7, 7 };
  bubble_sort (a);
  REQUIRE (a[0] == 7);
  REQUIRE (a[1] == 7);
  REQUIRE (a[2] == 7);
  REQUIRE (a[3] == 7);
}
