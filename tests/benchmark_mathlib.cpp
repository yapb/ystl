// benchmark_mathlib.cpp - benchmark ystl math functions vs std::math
#include <ystl/ystl.h>
#include <cmath>
#include <vector>
#include <random>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kNumIterations = 10000;

// fixed seed so every run compares identical data
static std::vector<float> generate_random_floats (size_t count, float min_val = -1000.0f, float max_val = 1000.0f) {
  std::mt19937 gen (42);
  std::uniform_real_distribution<float> dist (min_val, max_val);

  std::vector<float> values (count);
  for (size_t i = 0; i < count; ++i) {
    values[i] = dist (gen);
  }
  return values;
}

static std::vector<float> generate_random_angles (size_t count) {
  return generate_random_floats (count, -720.0f, 720.0f);
}

// sink forces compiler to keep buffer stores and feeding math
template <typename T> static void sink (T &&value) {
  ystl::benchmark::deoptimize_value (std::forward<T> (value));
}

// basic math functions benchmarks

TEST_CASE ("abs benchmark [benchmark][mathlib][abs]") {
  auto values = generate_random_floats (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::abs") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::abs (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::fabs") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::fabsf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("sqrf benchmark [benchmark][mathlib][sqrf]") {
  auto values = generate_random_floats (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::sqrf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::sqrf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::pow") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::pow (values[i], 2.0f);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// min/max/clamp benchmarks

TEST_CASE ("min benchmark [benchmark][mathlib][min]") {
  auto values_a = generate_random_floats (kNumIterations);
  auto values_b = generate_random_floats (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::min") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::min (values_a[i], values_b[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::min") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::min (values_a[i], values_b[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("max benchmark [benchmark][mathlib][max]") {
  auto values_a = generate_random_floats (kNumIterations);
  auto values_b = generate_random_floats (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::max") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::max (values_a[i], values_b[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::max") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::max (values_a[i], values_b[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("clamp benchmark [benchmark][mathlib][clamp]") {
  auto values = generate_random_floats (kNumIterations, -200.0f, 200.0f);
  constexpr float lo = 0.0f;
  constexpr float hi = 100.0f;
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::clamp") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::clamp (values[i], lo, hi);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::clamp") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::clamp (values[i], lo, hi);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// square root benchmarks

TEST_CASE ("sqrtf benchmark [benchmark][mathlib][sqrtf]") {
  auto values = generate_random_floats (kNumIterations, 0.0f, 10000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::sqrtf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::sqrtf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sqrtf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::sqrtf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("rsqrtf benchmark [benchmark][mathlib][rsqrtf]") {
  auto values = generate_random_floats (kNumIterations, 0.1f, 10000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::rsqrtf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::rsqrtf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("1.0f / sqrtf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = 1.0f / ::sqrtf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// trigonometric functions benchmarks

TEST_CASE ("sinf benchmark [benchmark][mathlib][sinf]") {
  auto values = generate_random_floats (kNumIterations, -2.0f * kMathPi, 2.0f * kMathPi);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::sinf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::sinf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sinf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::sinf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("cosf benchmark [benchmark][mathlib][cosf]") {
  auto values = generate_random_floats (kNumIterations, -2.0f * kMathPi, 2.0f * kMathPi);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::cosf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::cosf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::cosf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::cosf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("tanf benchmark [benchmark][mathlib][tanf]") {
  auto values = generate_random_floats (kNumIterations, -kMathPi / 2.0f + 0.1f, kMathPi / 2.0f - 0.1f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::tanf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::tanf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::tanf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::tanf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("atan2f benchmark [benchmark][mathlib][atan2f]") {
  auto values_y = generate_random_floats (kNumIterations, -1000.0f, 1000.0f);
  auto values_x = generate_random_floats (kNumIterations, -1000.0f, 1000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::atan2f") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::atan2f (values_y[i], values_x[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::atan2f") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::atan2f (values_y[i], values_x[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("sincosf benchmark [benchmark][mathlib][sincosf]") {
  auto values = generate_random_floats (kNumIterations, -2.0f * kMathPi, 2.0f * kMathPi);
  std::vector<std::pair<float, float>> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::sincosf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        float s, c;
        ystl::sincosf (values[i], s, c);
        results[i] = { s, c };
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("sinf + cosf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = { ::sinf (values[i]), ::cosf (values[i]) };
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// power and logarithm benchmarks

TEST_CASE ("powf benchmark [benchmark][mathlib][powf]") {
  auto bases = generate_random_floats (kNumIterations, 0.1f, 10.0f);
  auto exponents = generate_random_floats (kNumIterations, -2.0f, 5.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::powf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::powf (bases[i], exponents[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::powf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::powf (bases[i], exponents[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("log10f benchmark [benchmark][mathlib][log10f]") {
  auto values = generate_random_floats (kNumIterations, 0.001f, 10000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::log10f") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::log10f (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::log10f") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::log10f (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// rounding functions benchmarks

TEST_CASE ("roundf benchmark [benchmark][mathlib][roundf]") {
  auto values = generate_random_floats (kNumIterations, -1000.0f, 1000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::roundf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::roundf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::roundf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::roundf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("ceilf benchmark [benchmark][mathlib][ceilf]") {
  auto values = generate_random_floats (kNumIterations, -1000.0f, 1000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::ceilf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::ceilf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::ceilf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::ceilf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("floorf benchmark [benchmark][mathlib][floorf]") {
  auto values = generate_random_floats (kNumIterations, -1000.0f, 1000.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::floorf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::floorf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::floorf") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ::floorf (values[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// angle utility functions benchmarks

TEST_CASE ("deg2rad benchmark [benchmark][mathlib][deg2rad]") {
  auto degrees = generate_random_floats (kNumIterations, -720.0f, 720.0f);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::deg2rad") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::deg2rad (degrees[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("manual deg2rad") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = degrees[i] * kDegreeToRadians;
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("rad2deg benchmark [benchmark][mathlib][rad2deg]") {
  auto radians = generate_random_floats (kNumIterations, -4.0f * kMathPi, 4.0f * kMathPi);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::rad2deg") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::rad2deg (radians[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("manual rad2deg") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = radians[i] * kRadiansToDegree;
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("wrapAngle benchmark [benchmark][mathlib][wrapAngle]") {
  auto angles = generate_random_angles (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::wrapAngle") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::wrap_angle (angles[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("fmod approach") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        float a = angles[i];
        float result = ::fmodf (a + 180.0f, 360.0f);
        if (result < 0.0f)
          result += 360.0f;
        results[i] = result - 180.0f;
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("wrapAngle360 benchmark [benchmark][mathlib][wrapAngle360]") {
  auto angles = generate_random_angles (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::wrapAngle360") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::wrap_angle360 (angles[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("fmod approach") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        float a = angles[i];
        float result = ::fmodf (a, 360.0f);
        if (result < 0.0f)
          result += 360.0f;
        results[i] = result;
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("anglesDifference benchmark [benchmark][mathlib][anglesDifference]") {
  auto angles_a = generate_random_angles (kNumIterations);
  auto angles_b = generate_random_angles (kNumIterations);
  std::vector<float> results (kNumIterations);

  BENCHMARK_ADVANCED ("ystl::anglesDifference") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        results[i] = ystl::angles_difference (angles_a[i], angles_b[i]);
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("manual") {
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        float diff = angles_a[i] - angles_b[i];
        float result = ::fmodf (diff + 180.0f, 360.0f);
        if (result < 0.0f)
          result += 360.0f;
        results[i] = result - 180.0f;
      }
      sink (results.data ());
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

// float comparison functions benchmarks

TEST_CASE ("fzero benchmark [benchmark][mathlib][fzero]") {
  // mix of values near zero and far from zero
  std::vector<float> values;
  values.reserve (kNumIterations);
  for (size_t i = 0; i < kNumIterations; ++i) {
    if (i % 2 == 0) {
      values.push_back (static_cast<float> (i % 100) * 0.001f); // near zero
    }
    else {
      values.push_back (static_cast<float> (i)); // far from zero
    }
  }

  BENCHMARK_ADVANCED ("ystl::fzero") {
    size_t hits = 0;
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        hits += static_cast<size_t> (ystl::fzero (values[i]));
      }
      sink (hits);
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("manual fabs check") {
    size_t hits = 0;
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        hits += static_cast<size_t> (::fabsf (values[i]) < kFloatOnEpsilon);
      }
      sink (hits);
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("fequal benchmark [benchmark][mathlib][fequal]") {
  auto values_a = generate_random_floats (kNumIterations, 0.0f, 100.0f);
  auto values_b = generate_random_floats (kNumIterations, 0.0f, 100.0f);

  BENCHMARK_ADVANCED ("ystl::fequal") {
    size_t hits = 0;
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        hits += static_cast<size_t> (ystl::fequal (values_a[i], values_b[i]));
      }
      sink (hits);
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("absolute epsilon only") {
    size_t hits = 0;
    meter.measure_indexed ([&] (int run) {
      for (size_t i = 0; i < kNumIterations; ++i) {
        hits += static_cast<size_t> (::fabsf (values_a[i] - values_b[i]) < kFloatEqualEpsilon);
      }
      sink (hits);
      return run;
    });
  }
  BENCHMARK_ADVANCED_END;
}
