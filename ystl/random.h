// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/singleton.h>
#include <ystl/traits.h>

namespace ystl {

namespace detail {
class SplitMix64 final {
  uint64_t state_ {};

public:
  explicit SplitMix64 (uint64_t seed) noexcept : state_ (seed ^ 0x9e3779b97f4a7c15ULL) {}

public:
  uint64_t next () noexcept {
    uint64_t z = (state_ += 0x9e3779b97f4a7c15ULL);

    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;

    return z ^ (z >> 31);
  }
};
}

// xoshiro random number generator
class RWrand final : public Singleton<RWrand> {
// note: SIZE_MAX stays here, the preprocessor cannot evaluate numeric_limits (and this covers rv64)
#if defined(YSTL_ARCH_X64) || defined(YSTL_ARCH_ARM64) || (SIZE_MAX > 0xffffffffu)
  using state_t = uint64_t;
  static constexpr int kShiftT = 17;
  static constexpr int kRotS3 = 45;
#else
  using state_t = uint32_t;
  static constexpr int kShiftT = 9;
  static constexpr int kRotS3 = 11;
#endif

  mutable state_t s0_ = 0, s1_ = 0, s2_ = 0, s3_ = 0;

#if defined(YSTL_TESTS)
  // test-only determinism overrides (YSTL_TESTS builds only)
  mutable bool chance_forced_ {};
  mutable bool chance_value_ {};
  mutable bool (*chance_hook_) (int32_t) {};
  mutable bool int_forced_ {};
  mutable int32_t int_value_ {};
  mutable bool float_forced_ {};
  mutable float float_value_ {};
#endif

  [[nodiscard]] static state_t rotl (state_t x, int k) noexcept {
    return (x << k) | (x >> (sizeof (state_t) * 8 - static_cast<size_t> (k)));
  }

  [[nodiscard]] state_t next () const noexcept {
    const state_t result = rotl (s1_ * 5, 7) * 9;
    const state_t t = s1_ << kShiftT;

    s2_ ^= s0_;
    s3_ ^= s1_;
    s1_ ^= s2_;
    s0_ ^= s3_;

    s2_ ^= t;
    s3_ = rotl (s3_, kRotS3);

    return result;
  }

  [[nodiscard]] uint32_t next32 () const noexcept {
    if constexpr (sizeof (state_t) == 8) {
      return static_cast<uint32_t> (static_cast<uint64_t> (next ()) >> 32);
    }
    else {
      return next ();
    }
  }

  [[nodiscard]] uint64_t get_seed (uint64_t value = 0) noexcept {
    uint64_t seed = static_cast<uint64_t> (time (nullptr));

    seed ^= static_cast<uintptr_t> (plat.pid ());
    seed ^= static_cast<uint64_t> (clock ());
    seed ^= static_cast<uint64_t> (reinterpret_cast<uintptr_t> (&seed));
    seed ^= 0xdeadbeefULL;

    if (value > 0) {
      seed ^= value;
    }
    return seed;
  }

#if defined(YSTL_TESTS)
public:
  void force_chance (bool value) const noexcept {
    chance_forced_ = true;
    chance_value_ = value;
    chance_hook_ = nullptr;
  }

  void set_chance_hook (bool (*hook) (int32_t percent)) const noexcept {
    chance_hook_ = hook;
    chance_forced_ = false;
  }

  void force_int (int32_t value) const noexcept {
    int_forced_ = true;
    int_value_ = value;
  }

  void force_float (float value) const noexcept {
    float_forced_ = true;
    float_value_ = value;
  }

  void clear_forced () const noexcept {
    chance_forced_ = int_forced_ = float_forced_ = false;
    chance_hook_ = nullptr;
  }
#endif

public:
  RWrand () noexcept {
    seed ();
  }

  void seed () noexcept {
    const uint64_t seed = get_seed ();
    detail::SplitMix64 smix (seed);

    s0_ = static_cast<state_t> (smix.next ());
    s1_ = static_cast<state_t> (smix.next ());
    s2_ = static_cast<state_t> (smix.next ());
    s3_ = static_cast<state_t> (smix.next ());
  }

  void seed (uint64_t value) noexcept {
    detail::SplitMix64 smix (value);

    s0_ = static_cast<state_t> (smix.next ());
    s1_ = static_cast<state_t> (smix.next ());
    s2_ = static_cast<state_t> (smix.next ());
    s3_ = static_cast<state_t> (smix.next ());
  }

  [[nodiscard]] int32_t get (int32_t low, int32_t high) const noexcept {
#if defined(YSTL_TESTS)
    if (int_forced_) {
      return int_value_;
    }
#endif
    if (low >= high) {
      return low;
    }
    const uint32_t range = static_cast<uint32_t> (high) - static_cast<uint32_t> (low) + 1u;

    if (range == 0) [[unlikely]] {
      return static_cast<int32_t> (next32 ());
    }
    const uint64_t m = static_cast<uint64_t> (next32 ()) * range;

    return low + static_cast<int32_t> (m >> 32);
  }

  [[nodiscard]] float get (float low, float high) const noexcept {
#if defined(YSTL_TESTS)
    if (float_forced_) {
      return float_value_;
    }
#endif
    constexpr float scale = 1.0f / 16777216.0f;
    return low + (high - low) * (static_cast<float> (next32 () >> 8) * scale);
  }

  [[nodiscard]] int32_t operator() (int32_t low, int32_t high) const noexcept {
    return get (low, high);
  }

  [[nodiscard]] float operator() (float low, float high) const noexcept {
    return get (low, high);
  }

  [[nodiscard]] bool chance (int32_t percent) const noexcept {
#if defined(YSTL_TESTS)
    if (chance_hook_ != nullptr) {
      return chance_hook_ (percent);
    }

    if (chance_forced_) {
      return chance_value_;
    }
#endif
    return get (0, 99) < percent;
  }
};

// expose global
YSTL_EXPOSE_GLOBAL_SINGLETON (RWrand, rg);

}
