// benchmark_binheap.cpp - benchmark binaryheap vs std::priority_queue
#include <ystl/ystl.h>
#include <queue>
#include <vector>
#include <numeric>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumElements = 1000;

TEST_CASE ("BinaryHeap benchmark [benchmark][binheap]") {
  std::vector<int> values (kNumElements);
  std::iota (values.begin (), values.end (), 0);

  auto shuffled_values = [&] {
    Array<int> arr;
    for (size_t i = 0; i < kNumElements; ++i) {
      arr.push (values[i]);
    }
    arr.shuffle ();
    return arr;
  }();

  SECTION ("ystl::BinaryHeap") {
    BENCHMARK_ADVANCED ("push") {
      meter.measure ([&] {
        BinaryHeap<int> heap;
        for (size_t i = 0; i < kNumElements; ++i) {
          heap.push (values[i]);
        }
        return heap;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("top") {
      BinaryHeap<int> heap;
      for (const auto v : shuffled_values) {
        heap.push (v);
      }
      ystl::benchmark::deoptimize_value (heap);

      meter.measure ([&] {
        auto val = heap.top ();
        ystl::benchmark::deoptimize_value (val);
        return val;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("pop half") {
      std::vector<BinaryHeap<int>> heaps;
      heaps.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        heaps.emplace_back ();
        for (size_t i = 0; i < kNumElements; ++i) {
          heaps.back ().push (values[i]);
        }
      }
      // stateful: each heap is consumed by its pop pass (single timed pass)
      meter.measure_indexed_once ([&] (int i) {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          heaps[i].discard ();
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("std::priority_queue") {
    BENCHMARK_ADVANCED ("push") {
      meter.measure ([&] {
        std::priority_queue<int> heap;
        for (size_t i = 0; i < kNumElements; ++i) {
          heap.push (values[i]);
        }
        return heap;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("top") {
      std::priority_queue<int> heap;
      for (const auto v : shuffled_values) {
        heap.push (v);
      }
      ystl::benchmark::deoptimize_value (heap);

      meter.measure ([&] {
        auto val = heap.top ();
        ystl::benchmark::deoptimize_value (val);
        return val;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("pop half") {
      std::vector<std::priority_queue<int>> heaps (static_cast<size_t> (meter.runs ()));
      for (auto &h : heaps) {
        for (size_t i = 0; i < kNumElements; ++i) {
          h.push (values[i]);
        }
      }
      // stateful: see the ystl::binaryheap side above
      meter.measure_indexed_once ([&] (int i) {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          heaps[i].pop ();
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }
}
