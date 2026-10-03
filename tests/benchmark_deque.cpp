// benchmark_deque.cpp - benchmark deque vs std::deque
#include <ystl/ystl.h>
#include <deque>
#include <vector>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumElements = 1000;

TEST_CASE ("Deque benchmark [benchmark][deque]") {
  SECTION ("ystl::Deque") {
    BENCHMARK ("push back") {
      Deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.emplace_last (static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (dq);
    }
    BENCHMARK_END;

    BENCHMARK ("push front") {
      Deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.emplace_front (-static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (dq);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("front access + rotate") {
      Deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.emplace_last (static_cast<int> (i));
      }
      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += dq.front ();
          dq.discard_front ();
          dq.emplace_last (static_cast<int> (i));
        }
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    // pops are destructive so state is restored inside measured lambda
    BENCHMARK_ADVANCED ("pop back + push back") {
      Deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.emplace_last (static_cast<int> (i));
      }
      meter.measure ([&] {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.pop_last ();
        }
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.emplace_last (static_cast<int> (j));
        }
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("pop front + push front") {
      Deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.emplace_last (static_cast<int> (i));
      }
      meter.measure ([&] {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.pop_front ();
        }
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.emplace_front (static_cast<int> (j));
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("std::deque") {
    BENCHMARK ("push_back") {
      std::deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.push_back (static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (dq);
    }
    BENCHMARK_END;

    BENCHMARK ("push_front") {
      std::deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.push_front (-static_cast<int> (i));
      }
      ystl::benchmark::deoptimize_value (dq);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("front access + rotate") {
      std::deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.push_back (static_cast<int> (i));
      }
      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += dq.front ();
          dq.pop_front ();
          dq.push_back (static_cast<int> (i));
        }
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("pop_back + push_back") {
      std::deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.push_back (static_cast<int> (i));
      }
      meter.measure ([&] {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.pop_back ();
        }
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.push_back (static_cast<int> (j));
        }
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("pop_front + push_front") {
      std::deque<int> dq;
      for (size_t i = 0; i < kNumElements; ++i) {
        dq.push_back (static_cast<int> (i));
      }
      meter.measure ([&] {
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.pop_front ();
        }
        for (size_t j = 0; j < kNumElements / 2; ++j) {
          dq.push_front (static_cast<int> (j));
        }
      });
    }
    BENCHMARK_ADVANCED_END;
  }
}
