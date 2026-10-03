// benchmark_hashmap.cpp - benchmark hashmap vs std::unordered_map
#include <ystl/ystl.h>
#include <unordered_map>
#include <vector>
#include <numeric>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumElements = 1000;

TEST_CASE ("HashMap benchmark [benchmark][hashmap]") {
  std::vector<int> keys (kNumElements);
  std::iota (keys.begin (), keys.end (), 0);

  SECTION ("ystl::HashMap") {
    BENCHMARK_ADVANCED ("insert") {
      meter.measure ([&] {
        HashMap<int, int> map;
        map.reserve (kNumElements * 2);
        for (size_t i = 0; i < kNumElements; ++i) {
          map.insert (keys[i], keys[i] * 2);
        }
        return map;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("lookup") {
      HashMap<int, int> map;
      map.reserve (kNumElements * 2);
      for (size_t i = 0; i < kNumElements; ++i) {
        map.insert (keys[i], keys[i] * 2);
      }
      // even i hits, odd i misses: both probe paths stay live, and the alternating pattern can't be folded
      constexpr int kExpected = 4 * (499 * 500 / 2) - 500;
      auto lookup = [&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          const int key = (i & 1) ? keys[i] + static_cast<int> (kNumElements) : keys[i];
          auto *val = map.find (key);
          ystl::benchmark::deoptimize_value (val);
          sum += val ? *val : -1;
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      };
      REQUIRE (lookup () == kExpected); // correctness anchor + warmup
      meter.measure (lookup);
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("erase half") {
      std::vector<HashMap<int, int>> maps;
      maps.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        maps.emplace_back ();
        for (size_t i = 0; i < kNumElements; ++i) {
          maps.back ().insert (keys[i], keys[i] * 2);
        }
      }
      // erase consumes each map so use single timed pass
      meter.measure_indexed_once ([&] (int i) {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          maps[i].erase (keys[j]);
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("std::unordered_map") {
    BENCHMARK_ADVANCED ("insert") {
      meter.measure ([&] {
        std::unordered_map<int, int> map;
        for (size_t i = 0; i < kNumElements; ++i) {
          map.insert ({ keys[i], keys[i] * 2 });
        }
        return map;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("lookup") {
      std::unordered_map<int, int> map;
      for (size_t i = 0; i < kNumElements; ++i) {
        map.insert ({ keys[i], keys[i] * 2 });
      }
      // same hit/miss protocol as the ystl::hashmap side above
      constexpr int kExpected = 4 * (499 * 500 / 2) - 500;
      auto lookup = [&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          const int key = (i & 1) ? keys[i] + static_cast<int> (kNumElements) : keys[i];
          auto it = map.find (key);
          ystl::benchmark::deoptimize_value (it);
          sum += it != map.end () ? it->second : -1;
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      };
      REQUIRE (lookup () == kExpected); // correctness anchor + warmup
      meter.measure (lookup);
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("erase half") {
      std::vector<std::unordered_map<int, int>> maps (static_cast<size_t> (meter.runs ()));
      for (auto &m : maps) {
        for (size_t i = 0; i < kNumElements; ++i) {
          m.insert ({ keys[i], keys[i] * 2 });
        }
      }
      // stateful: see the ystl::hashmap side above
      meter.measure_indexed_once ([&] (int i) {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          maps[i].erase (keys[j]);
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }
}
