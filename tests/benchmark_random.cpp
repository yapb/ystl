// benchmark_random.cpp - benchmark ystl::rwrand vs std rngs
#include <ystl/ystl.h>
#include <random>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumIterations = 100000;

TEST_CASE ("Random number generator benchmark [benchmark][random]") {
  SECTION ("ystl::RWrand") {
    BENCHMARK ("integer range") {
      rg.seed (42);
      volatile int sink = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = rg.get (0, 9999);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("float range") {
      rg.seed (42);
      volatile float sink = 0.0f;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = rg.get (0.0f, 1.0f);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("chance") {
      rg.seed (42);
      volatile int count = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        if (rg.chance (50)) {
          count = count + 1;
        }
      }
      ystl::benchmark::deoptimize_value (count);
    }
    BENCHMARK_END;
  }

  SECTION ("std::mt19937") {
    BENCHMARK ("integer range") {
      std::mt19937 gen (42);
      std::uniform_int_distribution<int> dist (0, 9999);
      volatile int sink = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = dist (gen);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("float range") {
      std::mt19937 gen (42);
      std::uniform_real_distribution<float> dist (0.0f, 1.0f);
      volatile float sink = 0.0f;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = dist (gen);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("chance") {
      std::mt19937 gen (42);
      std::uniform_int_distribution<int> dist (0, 99);
      volatile int count = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        if (dist (gen) < 50) {
          count = count + 1;
        }
      }
      ystl::benchmark::deoptimize_value (count);
    }
    BENCHMARK_END;
  }

  SECTION ("std::minstd_rand") {
    BENCHMARK ("integer range") {
      std::minstd_rand gen (42);
      std::uniform_int_distribution<int> dist (0, 9999);
      volatile int sink = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = dist (gen);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("float range") {
      std::minstd_rand gen (42);
      std::uniform_real_distribution<float> dist (0.0f, 1.0f);
      volatile float sink = 0.0f;
      for (size_t i = 0; i < kNumIterations; ++i) {
        sink = dist (gen);
      }
      ystl::benchmark::deoptimize_value (sink);
    }
    BENCHMARK_END;

    BENCHMARK ("chance") {
      std::minstd_rand gen (42);
      std::uniform_int_distribution<int> dist (0, 99);
      volatile int count = 0;
      for (size_t i = 0; i < kNumIterations; ++i) {
        if (dist (gen) < 50) {
          count = count + 1;
        }
      }
      ystl::benchmark::deoptimize_value (count);
    }
    BENCHMARK_END;
  }
}
