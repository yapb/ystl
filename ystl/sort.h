// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/algorithm.h>
#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/traits.h>

namespace ystl {

namespace detail {

// detect if compare can be called with (const t&, const t&)
template <typename Compare, typename T, typename = void> struct ComparatorTakesRefs : false_type {};

template <typename Compare, typename T>
struct ComparatorTakesRefs<Compare, T, void_t<decltype (ystl::declval<Compare> () (ystl::declval<const T &> (), ystl::declval<const T &> ()))>>
  : true_type {};

template <typename Compare, typename T> inline constexpr bool ComparatorTakesRefs_v = ComparatorTakesRefs<Compare, T>::value;

// check if range [first, last) is sorted; early-out on first disorder
template <typename T, typename Compare> bool is_sorted_range (T *first, T *last, Compare comp) {
  if (first == last || first + 1 == last) {
    return true;
  }
  for (auto i = first + 1; i != last; ++i) {
    if (comp (*i, *(i - 1))) {
      return false;
    }
  }
  return true;
}

// check if range [first, last) is reverse-sorted; early-out on first order
template <typename T, typename Compare> bool is_reverse_sorted_range (T *first, T *last, Compare comp) {
  if (first == last || first + 1 == last) {
    return true;
  }
  for (auto i = first + 1; i != last; ++i) {
    if (comp (*(i - 1), *i)) {
      return false;
    }
  }
  return true;
}

// insertion sort for small ranges - optimized
template <typename T, typename Compare> YSTL_FORCE_INLINE void insertion_sort (T *first, T *last, Compare comp) {
  if (first == last) [[unlikely]] {
    return;
  }

  for (auto i = first + 1; i != last; ++i) {
    if (comp (*i, *(i - 1))) [[unlikely]] {
      T key = ystl::move (*i);
      auto j = i;

      while (j != first) {
        if (comp (key, *(j - 1))) [[unlikely]] {
          *j = ystl::move (*(j - 1));
          --j;
        }
        else {
          break;
        }
      }
      *j = ystl::move (key);
    }
  }
}

// median-of-three pivot selection
template <typename T, typename Compare> YSTL_FORCE_INLINE T *median_of_three (T *a, T *b, T *c, Compare comp) {
  if (comp (*b, *a)) {
    if (comp (*c, *b)) {
      return b;
    }
    return comp (*c, *a) ? c : a;
  }
  if (comp (*c, *a)) {
    return a;
  }
  return comp (*c, *b) ? b : c;
}

// median-of-three pivot selection and partition
template <typename T, typename Compare> T *partition (T *first, T *last, Compare comp) {
  auto mid = first + (last - first) / 2;
  auto pivot_it = median_of_three (first, mid, last - 1, comp);

  // move pivot to end
  ystl::swap (*pivot_it, *(last - 1));

  auto store = first;
  for (auto i = first; i < last - 1; ++i) {
    if (comp (*i, *(last - 1))) [[likely]] {
      ystl::swap (*i, *store);
      ++store;
    }
  }
  ystl::swap (*store, *(last - 1));
  return store;
}

// sift-down for heap sort
template <typename T, typename Compare> YSTL_FORCE_INLINE void heap_sift_down (T *first, size_t hole, size_t length, Compare comp) {
  T value = ystl::move (first[hole]);

  for (;;) {
    size_t child = hole * 2 + 1;

    if (child >= length) [[unlikely]] {
      break;
    }
    const size_t right = child + 1;

    if (right < length && comp (first[child], first[right])) {
      // child precedes right, so right should move up for max-heap
      if (!comp (value, first[right])) [[likely]] {
        break;
      }
      first[hole] = ystl::move (first[right]);
      hole = right;
    }
    else {
      // right precedes child (or no right), so child should move up for max-heap
      if (!comp (value, first[child])) [[likely]] {
        break;
      }
      first[hole] = ystl::move (first[child]);
      hole = child;
    }
  }
  first[hole] = ystl::move (value);
}

// heap sort - optimized
template <typename T, typename Compare> void heap_sort (T *first, size_t length, Compare comp) {
  if (length < 2) [[unlikely]] {
    return;
  }

  // build max-heap
  for (size_t i = (length - 1) / 2 + 1; i > 0; --i) {
    heap_sift_down (first, i - 1, length, comp);
  }

  // extract elements
  for (size_t i = length - 1; i > 0; --i) {
    ystl::swap (first[0], first[i]);
    heap_sift_down (first, 0, i, comp);
  }
}

// introsort: quicksort with heapsort fallback
template <typename T, typename Compare> void intro_sort (T *first, T *last, size_t depth, Compare comp) {
  while (last - first > static_cast<ptrdiff_t> (16)) {
    if (depth == 0) [[unlikely]] {
      heap_sort (first, static_cast<size_t> (last - first), comp);
      return;
    }
    --depth;

    auto pivot = partition (first, last, comp);

    if (pivot - first < last - pivot - 1) {
      intro_sort (first, pivot, depth, comp);
      first = pivot + 1;
    }
    else {
      intro_sort (pivot + 1, last, depth, comp);
      last = pivot;
    }
  }

  if (last - first > 1) {
    insertion_sort (first, last, comp);
  }
}

// compute initial depth limit
inline size_t depth_limit (size_t n) {
  size_t depth = 0;
  while (n > 1) {
    n >>= 1;
    ++depth;
  }
  return depth * 2;
}

// shared entry point
template <typename T, typename Compare> void do_sort (T *first, size_t count, Compare comp) {
  if (count < 2) [[unlikely]] {
    return;
  }

  if (is_sorted_range (first, first + count, comp)) [[unlikely]] {
    return;
  }

  if (is_reverse_sorted_range (first, first + count, comp)) [[unlikely]] {
    ystl::reverse (first, count);
    return;
  }
  intro_sort (first, first + count, depth_limit (count), comp);
}
}

// sort: works with containers (begin/end), c arrays, or raw pointers
template <typename Container> void sort (Container &container) {
  using T = remove_reference_t<decltype (*container.begin ())>;
  detail::do_sort (container.begin (), static_cast<size_t> (container.end () - container.begin ()), Less<T> {});
}

template <typename Container, typename Compare> void sort (Container &container, Compare comp) {
  detail::do_sort (container.begin (), static_cast<size_t> (container.end () - container.begin ()), comp);
}

template <typename T, size_t N> void sort (T (&array)[N]) {
  detail::do_sort (array, N, Less<T> {});
}

template <typename T, size_t N, typename Compare> void sort (T (&array)[N], Compare comp) {
  detail::do_sort (array, N, comp);
}

template <typename T> void sort (T *first, size_t count) {
  detail::do_sort (first, count, Less<T> {});
}

template <typename T, typename Compare> void sort (T *first, size_t count, Compare comp) {
  detail::do_sort (first, count, comp);
}

// bubblesort: stable sort for small arrays, supports parallel array sorting
template <typename T, typename Compare> void bubble_sort (T *first, size_t count, Compare comp) {
  if (count < 2) [[unlikely]] {
    return;
  }

  bool swapped = false;

  do {
    swapped = false;

    for (size_t i = 0; i < count - 1; ++i) {
      if (comp (first[i + 1], first[i])) [[unlikely]] {
        ystl::swap (first[i], first[i + 1]);
        swapped = true;
      }
    }
  } while (swapped);
}

// bubblesort with parallel array support - sorts first array and keeps second in sync
template <typename T, typename U, typename Compare> void bubble_sort (T *first, U *parallel, size_t count, Compare comp) {
  if (count < 2) [[unlikely]] {
    return;
  }
  bool swapped = false;

  do {
    swapped = false;

    for (size_t i = 0; i < count - 1; ++i) {
      if (comp (first[i + 1], first[i])) [[unlikely]] {
        ystl::swap (first[i], first[i + 1]);
        ystl::swap (parallel[i], parallel[i + 1]);
        swapped = true;
      }
    }
  } while (swapped);
}

// bubblesort for containers
template <typename Container, typename Compare> void bubble_sort (Container &container, Compare comp) {
  bubble_sort (container.begin (), static_cast<size_t> (container.end () - container.begin ()), comp);
}

template <typename Container> void bubble_sort (Container &container) {
  using T = remove_reference_t<decltype (*container.begin ())>;
  bubble_sort (container.begin (), static_cast<size_t> (container.end () - container.begin ()), Less<T> {});
}

// bubblesort for c arrays
template <typename T, size_t N, typename Compare> void bubble_sort (T (&array)[N], Compare comp) {
  bubble_sort (array, N, comp);
}

template <typename T, size_t N> void bubble_sort (T (&array)[N]) {
  bubble_sort (array, N, Less<T> {});
}

// bubblesort for raw pointers (without comparator)
template <typename T> void bubble_sort (T *first, size_t count) {
  bubble_sort (first, count, Less<T> {});
}

// bubblesort parallel arrays with c arrays
template <typename T, typename U, typename Compare, size_t N> void bubble_sort (T (&first)[N], U (&parallel)[N], Compare comp) {
  bubble_sort (first, parallel, N, comp);
}

}
