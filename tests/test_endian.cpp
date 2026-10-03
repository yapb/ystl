// test_endian.cpp - tests for ystl/endian.h, aware of both LE and BE targets
#include <ystl/ystl.h>
#include <ystl/test.h>

#include <cstdint>

using namespace ystl;

TEST_CASE ("ByteOrder::isLittleEndian matches the platform macros [endian]") {
#if defined(YSTL_ARCH_CPU_BIG_ENDIAN)
  REQUIRE (ByteOrder::is_little_endian () == false);
#else
  REQUIRE (ByteOrder::is_little_endian () == true);
#endif
}

// byteorder::swap16 / swap32 / swap64 are noops on LE and byte-swap on BE
TEST_CASE ("ByteOrder::swap16 follows platform endianness [endian]") {
  REQUIRE (ByteOrder::swap16 (static_cast<uint16_t> (0x1234u)) == (ByteOrder::is_little_endian () ? 0x1234u : 0x3412u));
}

TEST_CASE ("ByteOrder::swap32 follows platform endianness [endian]") {
  REQUIRE (ByteOrder::swap32 (0x12345678u) == (ByteOrder::is_little_endian () ? 0x12345678u : 0x78563412u));
}

TEST_CASE ("ByteOrder::swap64 follows platform endianness [endian]") {
  REQUIRE (ByteOrder::swap64 (0x123456789ABCDEF0ull) == (ByteOrder::is_little_endian () ? 0x123456789ABCDEF0ull : 0xF0DEBC9A78563412ull));
}

// byteorder::tonative - round-trips through le encoding on every platform
TEST_CASE ("ByteOrder::toNative with 8-bit types is NOOP [endian]") {
  REQUIRE (ByteOrder::to_native (static_cast<int8_t> (42)) == 42);
  REQUIRE (ByteOrder::to_native (static_cast<uint8_t> (255u)) == 255u);
  REQUIRE (ByteOrder::to_native (static_cast<char> ('A')) == 'A');
}

TEST_CASE ("ByteOrder::toNative with 16-bit types [endian]") {
  // tonative converts le to native. provide le-encoded input
  REQUIRE (ByteOrder::to_native (static_cast<int16_t> (ByteOrder::to_le<uint16_t> (0x1234u))) == 0x1234);
  REQUIRE (ByteOrder::to_native (ByteOrder::to_le<uint16_t> (0xABCDu)) == 0xABCDu);
}

TEST_CASE ("ByteOrder::toNative with 32-bit types [endian]") {
  REQUIRE (ByteOrder::to_native (ByteOrder::to_le<uint32_t> (0x12345678u)) == 0x12345678u);
  REQUIRE (ByteOrder::to_native (ByteOrder::to_le<uint32_t> (0xDEADBEEFu)) == 0xDEADBEEFu);
}

TEST_CASE ("ByteOrder::toNative with 64-bit types [endian]") {
  REQUIRE (ByteOrder::to_native (ByteOrder::to_le<uint64_t> (0x123456789ABCDEF0LL)) == 0x123456789ABCDEF0LL);
  REQUIRE (ByteOrder::to_native (ByteOrder::to_le<uint64_t> (0xFEDCBA9876543210ull)) == 0xFEDCBA9876543210ull);
}

// byteorder::tonative
TEST_CASE ("ByteOrder::toNative with float [endian]") {
  float input = 3.14159f;
  // tonative converts le to native. round-trip via tole to get le encoding
  uint32_t le_bits = ByteOrder::to_le<uint32_t> (*reinterpret_cast<const uint32_t *> (&input));
  float le_value;
  memcpy (&le_value, &le_bits, sizeof (float));
  float result = ByteOrder::to_native (le_value);
  REQUIRE (result == input);
}

TEST_CASE ("ByteOrder::toNative with double [endian]") {
  double input = 2.718281828459045;
  uint64_t le_bits = ByteOrder::to_le<uint64_t> (*reinterpret_cast<const uint64_t *> (&input));
  double le_value;
  memcpy (&le_value, &le_bits, sizeof (double));
  double result = ByteOrder::to_native (le_value);
  REQUIRE (result == input);
}

// byteorder::fromle / tole are noops on LE and swap on BE
TEST_CASE ("ByteOrder::fromLE follows platform endianness [endian]") {
  REQUIRE (ByteOrder::from_le (0x12345678u) == (ByteOrder::is_little_endian () ? 0x12345678u : 0x78563412u));
  REQUIRE (ByteOrder::from_le (3.14f) == (ByteOrder::is_little_endian () ? 3.14f : ByteOrder::swap_scalar (3.14f)));
}

TEST_CASE ("ByteOrder::toLE follows platform endianness [endian]") {
  REQUIRE (ByteOrder::to_le (0x12345678u) == (ByteOrder::is_little_endian () ? 0x12345678u : 0x78563412u));
  REQUIRE (ByteOrder::to_le (2.71) == (ByteOrder::is_little_endian () ? 2.71 : ByteOrder::swap_scalar (2.71)));
}

// byteorder::frombe / tobe swap on LE and are noops on BE
TEST_CASE ("ByteOrder::fromBE follows platform endianness [endian]") {
  REQUIRE (ByteOrder::from_be (0x12345678u) == (ByteOrder::is_little_endian () ? 0x78563412u : 0x12345678u));
}

TEST_CASE ("ByteOrder::toBE follows platform endianness [endian]") {
  REQUIRE (ByteOrder::to_be (0x12345678u) == (ByteOrder::is_little_endian () ? 0x78563412u : 0x12345678u));
}

// byteorder::readle / readbe interpret the buffer as little / big endian
TEST_CASE ("ByteOrder::readLE follows platform endianness [endian]") {
  uint32_t value = 0x12345678u;
  REQUIRE (ByteOrder::read_le<uint32_t> (&value) == (ByteOrder::is_little_endian () ? 0x12345678u : 0x78563412u));
}

TEST_CASE ("ByteOrder::readBE follows platform endianness [endian]") {
  uint32_t value = 0x12345678u;
  REQUIRE (ByteOrder::read_be<uint32_t> (&value) == (ByteOrder::is_little_endian () ? 0x78563412u : 0x12345678u));
}

TEST_CASE ("ByteOrder::writeLE follows platform endianness [endian]") {
  uint32_t buffer = 0;
  ByteOrder::write_le (&buffer, 0x12345678u);
  REQUIRE (buffer == (ByteOrder::is_little_endian () ? 0x12345678u : 0x78563412u));
}

TEST_CASE ("ByteOrder::writeBE follows platform endianness [endian]") {
  uint32_t buffer = 0;
  ByteOrder::write_be (&buffer, 0x12345678u);
  REQUIRE (buffer == (ByteOrder::is_little_endian () ? 0x78563412u : 0x12345678u));
}

// float byte swapping
TEST_CASE ("ByteOrder handles float byte swapping [endian]") {
  // tonative converts le to native. provide le-encoded float
  float original = 1.5f;
  uint32_t le_bits = ByteOrder::to_le<uint32_t> (*reinterpret_cast<const uint32_t *> (&original));
  float le_value;
  memcpy (&le_value, &le_bits, sizeof (float));
  REQUIRE (ByteOrder::to_native (le_value) == original);

  // zero is le-safe (bswap(0) = 0)
  REQUIRE (ByteOrder::to_native (0.0f) == 0.0f);
}

TEST_CASE ("ByteOrder handles double byte swapping [endian]") {
  double original = 1.5;
  uint64_t le_bits = ByteOrder::to_le<uint64_t> (*reinterpret_cast<const uint64_t *> (&original));
  double le_value;
  memcpy (&le_value, &le_bits, sizeof (double));
  REQUIRE (ByteOrder::to_native (le_value) == original);

  // zero is le-safe
  REQUIRE (ByteOrder::to_native (0.0) == 0.0);
}

// compile-time constexpr
TEST_CASE ("ByteOrder functions are constexpr-capable [endian]") {
  // verify islittleendian is constexpr
  static_assert (ByteOrder::is_little_endian () == true || ByteOrder::is_little_endian () == false, "isLittleEndian should be constexpr");
}

// edge cases
TEST_CASE ("ByteOrder handles zero values [endian]") {
  REQUIRE (ByteOrder::to_native (static_cast<uint16_t> (0u)) == 0u);
  REQUIRE (ByteOrder::to_native (static_cast<uint32_t> (0u)) == 0u);
  REQUIRE (ByteOrder::to_native (static_cast<uint64_t> (0u)) == 0u);
  REQUIRE (ByteOrder::to_native (0.0f) == 0.0f);
  REQUIRE (ByteOrder::to_native (0.0) == 0.0);
}

TEST_CASE ("ByteOrder handles max values [endian]") {
  REQUIRE (ByteOrder::to_native (numeric_limits<uint16_t>::max ()) == numeric_limits<uint16_t>::max ());
  REQUIRE (ByteOrder::to_native (numeric_limits<uint32_t>::max ()) == numeric_limits<uint32_t>::max ());
  REQUIRE (ByteOrder::to_native (numeric_limits<uint64_t>::max ()) == numeric_limits<uint64_t>::max ());
}

// buffer operations with floats
TEST_CASE ("ByteOrder::readLE/readBE with float buffer [endian]") {
  float buffer = 3.14159f;

  if constexpr (ByteOrder::is_little_endian ()) {
    REQUIRE (ByteOrder::read_le<float> (&buffer) == buffer);
  }
  else {
    REQUIRE (ByteOrder::read_le<float> (&buffer) == ByteOrder::swap_scalar (buffer));
    REQUIRE (ByteOrder::read_be<float> (&buffer) == buffer);
  }
}

TEST_CASE ("ByteOrder::writeLE/writeBE with float buffer [endian]") {
  float out_buffer = 0.0f;
  float value = 2.71828f;

  ByteOrder::write_le (&out_buffer, value);

  if constexpr (ByteOrder::is_little_endian ()) {
    REQUIRE (out_buffer == value);
  }
  else {
    REQUIRE (out_buffer == ByteOrder::swap_scalar (value));
  }

  ByteOrder::write_be (&out_buffer, value);

  if constexpr (ByteOrder::is_little_endian ()) {
    REQUIRE (out_buffer == ByteOrder::swap_scalar (value));
  }
  else {
    REQUIRE (out_buffer == value);
  }
}
