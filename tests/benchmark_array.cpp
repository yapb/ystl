// benchmark_array.cpp - benchmark array vs std::vector
#include <ystl/ystl.h>
#include <vector>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumElements = 1000;

TEST_CASE ("Array benchmark [benchmark][array]") {
  SECTION ("ystl::Array") {
    BENCHMARK ("push") {
      Array<int> arr;
      for (size_t i = 0; i < kNumElements; ++i) {
        arr.push (static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (arr);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("random access") {
      Array<int> arr;
      for (size_t i = 0; i < kNumElements; ++i) {
        arr.push (static_cast<int> (i));
      }

      Array<int> indices;
      for (size_t i = 0; i < kNumElements; ++i) {
        indices.push (static_cast<int> (i));
      }
      indices.shuffle ();

      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += arr[static_cast<size_t> (indices[i])];
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("iteration") {
      Array<int> arr;
      for (size_t i = 0; i < kNumElements; ++i) {
        arr.push (static_cast<int> (i));
      }
      meter.measure ([&] {
        int sum = 0;
        for (auto &v : arr) {
          sum += v;
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("erase half") {
      std::vector<Array<int>> arrs;
      arrs.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        arrs.emplace_back ();
        for (size_t i = 0; i < kNumElements; ++i) {
          arrs.back ().push (static_cast<int> (i));
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        arrs[i].erase (0, kNumElements / 2);
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("std::vector") {
    BENCHMARK ("push_back") {
      std::vector<int> vec;
      for (size_t i = 0; i < kNumElements; ++i) {
        vec.push_back (static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (vec);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("random access") {
      std::vector<int> vec;
      for (size_t i = 0; i < kNumElements; ++i) {
        vec.push_back (static_cast<int> (i));
      }

      Array<int> indices;
      for (size_t i = 0; i < kNumElements; ++i) {
        indices.push (static_cast<int> (i));
      }
      indices.shuffle ();

      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += vec[static_cast<size_t> (indices[i])];
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("iteration") {
      std::vector<int> vec;
      for (size_t i = 0; i < kNumElements; ++i) {
        vec.push_back (static_cast<int> (i));
      }
      meter.measure ([&] {
        int sum = 0;
        for (auto &v : vec) {
          sum += v;
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("erase half") {
      std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
      for (auto &v : vecs) {
        for (size_t i = 0; i < kNumElements; ++i) {
          v.push_back (static_cast<int> (i));
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        vecs[i].erase (vecs[i].begin (), vecs[i].begin () + kNumElements / 2);
      });
    }
    BENCHMARK_ADVANCED_END;
  }
}
