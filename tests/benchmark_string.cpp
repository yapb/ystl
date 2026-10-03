// benchmark_string.cpp - benchmark string vs std::string
#include <ystl/ystl.h>
#include <string>
#include <vector>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumElements = 1000;
static constexpr const char *kTestString =
  "test_item_value_with_a_padding_to_my_item_value_for_test_test_value_prevent_sso_of_std_string_or_whatever_sso_stuff";
static constexpr const char *kFindPattern = "my_item_value_for_test_test_value";
// miss pattern matches find length but stays absent to keep miss path live
static constexpr const char *kMissPattern = "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";

TEST_CASE ("String benchmark [benchmark][string]") {
  SECTION ("ystl::String") {
    BENCHMARK ("construct") {
      Array<String> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push (String (kTestString));
      }
      ystl::benchmark::deoptimize_value (strings);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("concat") {
      Array<String> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push (String (kTestString));
      }
      meter.measure ([&] {
        String combined;
        for (size_t i = 0; i < kNumElements / 10; ++i) {
          combined += strings[i];
        }
        return combined;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("char access") {
      Array<String> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push (String (kTestString));
      }
      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += strings[i][0];
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("find") {
      Array<String> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push (String (kTestString));
      }
      // alternate hits and misses so pattern cannot be folded
      auto search = [&] {
        size_t found = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          const size_t pos = strings[i].find (i & 1 ? kMissPattern : kFindPattern);
          ystl::benchmark::deoptimize_value (pos);
          found += (pos != String::InvalidIndex);
        }
        ystl::benchmark::deoptimize_value (found);
        return found;
      };
      REQUIRE (search () == kNumElements / 2); // correctness anchor + warmup
      meter.measure (search);
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("std::string") {
    BENCHMARK ("construct") {
      std::vector<std::string> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push_back (std::string (kTestString));
      }
      ystl::benchmark::deoptimize_value (strings);
    }
    BENCHMARK_END;

    BENCHMARK_ADVANCED ("concat") {
      std::vector<std::string> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push_back (std::string (kTestString));
      }
      meter.measure ([&] {
        std::string combined;
        for (size_t i = 0; i < kNumElements / 10; ++i) {
          combined += strings[i];
        }
        return combined;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("char access") {
      std::vector<std::string> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push_back (std::string (kTestString));
      }
      meter.measure ([&] {
        int sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          sum += strings[i][0];
        }
        ystl::benchmark::deoptimize_value (sum);
        return sum;
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("find") {
      std::vector<std::string> strings;
      for (size_t i = 0; i < kNumElements; ++i) {
        strings.push_back (std::string (kTestString));
      }
      // same hit/miss protocol as the ystl::string side above
      auto search = [&] {
        size_t found = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
          const size_t pos = strings[i].find (i & 1 ? kMissPattern : kFindPattern);
          ystl::benchmark::deoptimize_value (pos);
          found += (pos != std::string::npos);
        }
        ystl::benchmark::deoptimize_value (found);
        return found;
      };
      REQUIRE (search () == kNumElements / 2); // correctness anchor + warmup
      meter.measure (search);
    }
    BENCHMARK_ADVANCED_END;
  }
}
