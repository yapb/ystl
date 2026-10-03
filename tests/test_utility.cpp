// test_utility.cpp tests for ystl utility.h with ystl first
#include <ystl/ystl.h>
#include <ystl/test.h>

// ystl::bufsize
TEST_CASE ("bufsize returns array length minus 1 [utility]") {
  char buf128[128];
  REQUIRE (ystl::bufsize (buf128) == 127u);

  char buf1[1];
  REQUIRE (ystl::bufsize (buf1) == 0u);

  char buf512[512];
  REQUIRE (ystl::bufsize (buf512) == 511u);
}

// ystl::bit
TEST_CASE ("bit shifts 1 by n positions [utility]") {
  REQUIRE (ystl::bit (0) == 1u);
  REQUIRE (ystl::bit (1) == 2u);
  REQUIRE (ystl::bit (4) == 16u);
  REQUIRE (ystl::bit (8) == 256u);
  REQUIRE (ystl::bit (10) == 1024u);
}

// ystl::bit with enum class
TEST_CASE ("bit works with enum class types [utility]") {
  enum class TestEnum : int32_t {
    Zero = 0,
    One = 1,
    Four = 4,
    Eight = 8
  };

  using U = std::underlying_type_t<TestEnum>;
  REQUIRE (static_cast<U> (ystl::bit (TestEnum::Zero)) == 1);
  REQUIRE (static_cast<U> (ystl::bit (TestEnum::One)) == 2);
  REQUIRE (static_cast<U> (ystl::bit (TestEnum::Four)) == 16);
  REQUIRE (static_cast<U> (ystl::bit (TestEnum::Eight)) == 256);
}

// ystl::bit_ceil
TEST_CASE ("bit_ceil returns next power of two [utility]") {
  REQUIRE (ystl::bit_ceil (0) == 1u);
  REQUIRE (ystl::bit_ceil (1) == 1u);
  REQUIRE (ystl::bit_ceil (2) == 2u);
  REQUIRE (ystl::bit_ceil (3) == 4u);
  REQUIRE (ystl::bit_ceil (4) == 4u);
  REQUIRE (ystl::bit_ceil (5) == 8u);
  REQUIRE (ystl::bit_ceil (7) == 8u);
  REQUIRE (ystl::bit_ceil (8) == 8u);
  REQUIRE (ystl::bit_ceil (9) == 16u);
  REQUIRE (ystl::bit_ceil (100) == 128u);
  REQUIRE (ystl::bit_ceil (1000) == 1024u);
  REQUIRE (ystl::bit_ceil (1024) == 1024u);
  REQUIRE (ystl::bit_ceil (1025) == 2048u);
}

// ystl::bit_ceil boundaries incl. saturation at the top of size_t range
TEST_CASE ("bit_ceil saturates near size_t max [utility]") {
  constexpr auto digits = ystl::numeric_limits<size_t>::digits;
  constexpr auto max = ystl::numeric_limits<size_t>::max ();

  REQUIRE (ystl::bit_ceil (max) == max);
  REQUIRE (ystl::bit_ceil (max - 1) == max);

  if constexpr (digits >= 64) {
    REQUIRE (ystl::bit_ceil (1ULL << 63) == (1ULL << 63));
    REQUIRE (ystl::bit_ceil ((1ULL << 63) + 1) == max);
    REQUIRE (ystl::bit_ceil ((1ULL << 63) - 1) == (1ULL << 63));
  }

  if constexpr (digits >= 32) {
    REQUIRE (ystl::bit_ceil (1ULL << 31) == (1ULL << 31));
    REQUIRE (ystl::bit_ceil ((1ULL << 31) + 1) == (digits > 32 ? (1ULL << 32) : max));
    REQUIRE (ystl::bit_ceil (0xFFFFFFFFULL) == (digits > 32 ? 0x100000000ULL : max));
  }
}
