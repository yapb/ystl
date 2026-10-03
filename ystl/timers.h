// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/mathlib.h>
#include <ystl/singleton.h>

namespace ystl {

// set the storage time function
class TimerSource final : public Singleton<TimerSource> {
private:
  float *timefn_ {};

public:
  explicit TimerSource () = default;
  ~TimerSource () = default;

public:
  void set_time_address (float *timefn) {
    timefn_ = timefn;
  }

public:
  float value () const {
    return *timefn_;
  }
};

// expose global timer storage
YSTL_EXPOSE_GLOBAL_SINGLETON (TimerSource, timer_source);

// invalid timer value
namespace detail {
constexpr float kInvalidTimerValue = -1.0f;
constexpr float kMaxTimerValue = 1.0e9f;
}

// simple class for counting down a short interval of time
class CountdownTimer {
private:
  float duration_ { 0.0f };
  float timestamp_ { detail::kInvalidTimerValue };

public:
  CountdownTimer () = default;
  explicit CountdownTimer (const float duration) {
    start (duration);
  }

public:
  void reset () {
    timestamp_ = timer_source.value () + duration_;
  }

  void start (const float duration) {
    duration_ = duration;
    reset ();
  }

  void invalidate () {
    timestamp_ = detail::kInvalidTimerValue;
  }

public:
  bool started () const {
    return !ystl::fequal (timestamp_, detail::kInvalidTimerValue);
  }

  bool elapsed () const {
    return started () ? (timestamp_ < timer_source.value ()) : true;
  }

  float elapsed_time () const {
    return started () ? (timer_source.value () - timestamp_ + duration_) : detail::kMaxTimerValue;
  }

  float timestamp () const {
    return timestamp_;
  }

  float remaining_time () const {
    return started () ? (timestamp_ - timer_source.value ()) : -detail::kMaxTimerValue;
  }

  float countdown_duration () const {
    return started () ? duration_ : 0.0f;
  }
};

// simple class for tracking intervals of time
class IntervalTimer {
private:
  float timestamp_ { detail::kInvalidTimerValue };

public:
  IntervalTimer () = default;

public:
  void reset () {
    timestamp_ = timer_source.value ();
  }

  void start () {
    timestamp_ = timer_source.value ();
  }

  void invalidate () {
    timestamp_ = detail::kInvalidTimerValue;
  }

public:
  bool started () const {
    return !ystl::fequal (timestamp_, detail::kInvalidTimerValue);
  }

  float elapsed_time () const {
    return started () ? (timer_source.value () - timestamp_) : detail::kMaxTimerValue;
  }

  bool less_than (const float duration) const {
    return elapsed_time () < duration;
  }

  bool greater_than (const float duration) const {
    return elapsed_time () > duration;
  }
};

// high-resolution monotonic stopwatch for profiling and benchmarking
class Stopwatch final {
private:
  int64_t start_ {};

  // current monotonic clock value in native ticks (nanoseconds on posix)
  [[nodiscard]] static int64_t now () noexcept {
#if defined(YSTL_WINDOWS)
    LARGE_INTEGER counter {};
    QueryPerformanceCounter (&counter);
    return counter.QuadPart;
#elif defined(CLOCK_MONOTONIC)
    timespec ts {};
    clock_gettime (CLOCK_MONOTONIC, &ts);

    return static_cast<int64_t> (ts.tv_sec) * 1000000000ll + static_cast<int64_t> (ts.tv_nsec);
#else
    return static_cast<int64_t> (clock ()) * (1000000000ll / CLOCKS_PER_SEC);
#endif
  }

  // converts native ticks to nanoseconds (1.0 when now () already counts ns)
  [[nodiscard]] static double ticks_to_ns () noexcept {
#if defined(YSTL_WINDOWS)
    static const double ns_per_tick = [] {
      LARGE_INTEGER freq {};
      QueryPerformanceFrequency (&freq);

      return freq.QuadPart > 0 ? (1e9 / static_cast<double> (freq.QuadPart)) : 1.0;
    }();
    return ns_per_tick;
#else
    return 1.0;
#endif
  }

public:
  Stopwatch () noexcept : start_ (now ()) {}

  void start () noexcept {
    start_ = now ();
  }

  void reset () noexcept {
    start_ = now ();
  }

  // elapsed nanoseconds since start ()
  [[nodiscard]] double elapsed_ns () const noexcept {
    return static_cast<double> (now () - start_) * ticks_to_ns ();
  }

  // elapsed microseconds since start ()
  [[nodiscard]] double elapsed_us () const noexcept {
    return elapsed_ns () * 1e-3;
  }

  // elapsed milliseconds since start ()
  [[nodiscard]] double elapsed_ms () const noexcept {
    return elapsed_ns () * 1e-6;
  }

  // elapsed seconds since start ()
  [[nodiscard]] double elapsed () const noexcept {
    return elapsed_ns () * 1e-9;
  }
};

}
