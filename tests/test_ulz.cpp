// test_ulz.cpp - tests for ystl/ulz.h (ulz compression)
#include <ystl/ystl.h>
#include <ystl/test.h>

#include <string.h>

using namespace ystl;

// helper: compress then decompress, verify round-trip
static ULZ &test_ulz () {
  static ULZ ulz;
  return ulz;
}

static bool round_trip (const uint8_t *src, int32_t srcLen, uint8_t *tmp_buf, uint8_t *out_buf, int32_t out_buf_len) {
  auto &ulz = test_ulz ();

  int32_t comp_len = ulz.compress (const_cast<uint8_t *> (src), srcLen, tmp_buf);
  if (comp_len <= 0) {
    return false;
  }
  int32_t decomp_len = ulz.uncompress (tmp_buf, comp_len, out_buf, out_buf_len);
  if (decomp_len != srcLen) {
    return false;
  }
  return memcmp (src, out_buf, srcLen) == 0;
}

// separate instances work
TEST_CASE ("ULZ instances are independent [ulz]") {
  ULZ a;
  ULZ b;

  const uint8_t input[8] = { 't', 'e', 's', 't', 'd', 'a', 't', 'a' };
  uint8_t out_a[16 + ULZ::Excess] = {};
  uint8_t out_b[16 + ULZ::Excess] = {};

  const int32_t len_a = a.compress (input, sizeof (input), out_a);
  const int32_t len_b = b.compress (input, sizeof (input), out_b);

  REQUIRE (len_a > 0);
  REQUIRE (len_a == len_b);
  REQUIRE (memcmp (out_a, out_b, static_cast<size_t> (len_a)) == 0);
}

// compress + uncompress
TEST_CASE ("ULZ compresses and decompresses a short repetitive buffer [ulz]") {
  // highly compressible: 1024 'a' bytes
  const int32_t data_len = 1024;
  uint8_t input[data_len];
  memset (input, 'A', data_len);

  // compressed output buffer: ulz::excess extra bytes guaranteed
  uint8_t compressed[data_len + ULZ::Excess];
  uint8_t restored[data_len];

  REQUIRE (round_trip (input, data_len, compressed, restored, data_len));
}

TEST_CASE ("ULZ compresses and decompresses ASCII text [ulz]") {
  const char *text = "The quick brown fox jumps over the lazy dog. "
                     "Pack my box with five dozen liquor jugs. "
                     "How vexingly quick daft zebras jump! ";
  int32_t data_len = static_cast<int32_t> (strlen (text));

  uint8_t compressed[2048 + ULZ::Excess];
  uint8_t restored[2048];

  REQUIRE (round_trip (reinterpret_cast<const uint8_t *> (text), data_len, compressed, restored, data_len));
}

TEST_CASE ("ULZ compresses and decompresses pseudo-random binary data [ulz]") {
  // deterministic pseudo-random data (less compressible)
  const int32_t data_len = 512;
  uint8_t input[data_len];
  uint32_t state = 0xdeadbeef;
  for (int32_t i = 0; i < data_len; ++i) {
    state = state * 1664525u + 1013904223u;
    input[i] = static_cast<uint8_t> (state >> 24);
  }

  uint8_t compressed[data_len * 2 + ULZ::Excess];
  uint8_t restored[data_len];

  REQUIRE (round_trip (input, data_len, compressed, restored, data_len));
}

TEST_CASE ("ULZ compresses repeated byte patterns efficiently [ulz]") {
  // pattern: abababab... (2-byte repeat)
  const int32_t data_len = 2048;
  uint8_t input[data_len];
  for (int32_t i = 0; i < data_len; ++i) {
    input[i] = (i % 2 == 0) ? 'A' : 'B';
  }

  uint8_t compressed[data_len + ULZ::Excess];
  uint8_t restored[data_len];

  int32_t comp_len = test_ulz ().compress (input, data_len, compressed);
  REQUIRE (comp_len > 0);
  // good compression expected for repeating patterns
  REQUIRE (comp_len < data_len);

  int32_t decomp_len = test_ulz ().uncompress (compressed, comp_len, restored, data_len);
  REQUIRE (decomp_len == data_len);
  REQUIRE (memcmp (input, restored, data_len) == 0);
}

// uncompress failure
TEST_CASE ("ULZ uncompress returns failure for truncated input [ulz]") {
  const int32_t data_len = 512;
  uint8_t input[data_len];
  memset (input, 'X', data_len);

  uint8_t compressed[data_len + ULZ::Excess];
  uint8_t restored[data_len];

  int32_t comp_len = test_ulz ().compress (input, data_len, compressed);
  REQUIRE (comp_len > 0);

  // feed only half the compressed data -> should fail
  if (comp_len > 2) {
    int32_t result = test_ulz ().uncompress (compressed, comp_len / 2, restored, data_len);
    REQUIRE (result == ULZ::UncompressFailure);
  }
}

TEST_CASE ("ULZ uncompress returns failure when output buffer is too small [ulz]") {
  const int32_t data_len = 256;
  uint8_t input[data_len];
  memset (input, 'Y', data_len);

  uint8_t compressed[data_len + ULZ::Excess];
  uint8_t tiny_output[16];

  int32_t comp_len = test_ulz ().compress (input, data_len, compressed);
  REQUIRE (comp_len > 0);

  int32_t result = test_ulz ().uncompress (compressed, comp_len, tiny_output, sizeof (tiny_output));
  REQUIRE (result == ULZ::UncompressFailure);
}

// large buffer round-trip
TEST_CASE ("ULZ uncompress returns failure on truncated distance bytes [ulz]") {
  // a single match token (value 0x00 = 4-byte match, no literals) with no distance bytes following
  uint8_t malformed[] = { 0x00 };
  uint8_t out[16] {};

  int32_t result = test_ulz ().uncompress (malformed, 1, out, sizeof (out));
  REQUIRE (result == ULZ::UncompressFailure);
}

// regression: copy
TEST_CASE ("ULZ decompresses correctly into exact-size output buffer (no slack) [ulz]") {
  // compress run ending with short match or literal
  const int32_t data_len = 64;
  uint8_t input[data_len];
  // pattern chosen so the last few bytes form a short match near the end
  for (int32_t i = 0; i < data_len; ++i) {
    input[i] = static_cast<uint8_t> ((i < 32) ? 'A' : 'B');
  }

  uint8_t compressed[data_len + ULZ::Excess];
  uint8_t restored[data_len]; // no extra slack

  int32_t comp_len = test_ulz ().compress (input, data_len, compressed);
  REQUIRE (comp_len > 0);

  int32_t decomp_len = test_ulz ().uncompress (compressed, comp_len, restored, data_len);
  REQUIRE (decomp_len == data_len);
  REQUIRE (memcmp (input, restored, data_len) == 0);
}

// large buffer round-trip
TEST_CASE ("ULZ handles a larger buffer (16 KB) [ulz]") {
  const int32_t data_len = 16 * 1024;
  auto input_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len));
  auto comp_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len + ULZ::Excess));
  auto restore_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len));

  // fill with a walking pattern
  for (int32_t i = 0; i < data_len; ++i) {
    input_buf[i] = static_cast<uint8_t> (i & 0xFF);
  }

  REQUIRE (round_trip (input_buf.get (), data_len, comp_buf.get (), restore_buf.get (), data_len));
}

// edge cases
TEST_CASE ("ULZ compress with zero-length input [ulz]") {
  uint8_t input[1] = { 0 };
  uint8_t compressed[ULZ::Excess];

  int32_t comp_len = test_ulz ().compress (input, 0, compressed);
  REQUIRE (comp_len == 0);
}

TEST_CASE ("ULZ compress with 1-byte input [ulz]") {
  uint8_t input[] = { 'X' };
  uint8_t compressed[sizeof (input) + ULZ::Excess];
  uint8_t restored[sizeof (input)];

  REQUIRE (round_trip (input, 1, compressed, restored, 1));
}

TEST_CASE ("ULZ compress with 3-byte input (less than MinMatch) [ulz]") {
  uint8_t input[] = { 'A', 'B', 'C' };
  uint8_t compressed[sizeof (input) + ULZ::Excess];
  uint8_t restored[sizeof (input)];

  REQUIRE (round_trip (input, 3, compressed, restored, 3));
}

TEST_CASE ("ULZ uncompress with zero-length input [ulz]") {
  uint8_t input[] = { 0 };
  uint8_t output[16];

  // empty stream into non-empty target must fail
  int32_t result = test_ulz ().uncompress (input, 0, output, sizeof (output));
  REQUIRE (result == ULZ::UncompressFailure);

  // ...and an empty stream into an empty target round-trips
  result = test_ulz ().uncompress (input, 0, output, 0);
  REQUIRE (result == 0);
}

TEST_CASE ("ULZ uncompress fails when a literal run is truncated short of outLength [ulz]") {
  // token 0x40: literal run of 2, no continuation
  uint8_t truncated[] = { 0x40, 'A', 'B' };
  uint8_t out[8] {};

  int32_t result = test_ulz ().uncompress (truncated, sizeof (truncated), out, sizeof (out));
  REQUIRE (result == ULZ::UncompressFailure);
}

TEST_CASE ("ULZ uncompress with invalid distance > current position [ulz]") {
  // create a compressed buffer with distance larger than output position token: 0x00 distance: 0x0001
  uint8_t malformed[] = { 0x00, 0x01, 0x00 }; // token + distance bytes
  uint8_t out[16] = { 0 };

  int32_t result = test_ulz ().uncompress (malformed, sizeof (malformed), out, sizeof (out));
  REQUIRE (result == ULZ::UncompressFailure);
}

TEST_CASE ("ULZ uncompress with distance = 0 should fail [ulz]") {
  // distance 0 is invalid (would cause infinite loop)
  uint8_t malformed[] = { 0x00, 0x00, 0x00 }; // token + distance = 0
  uint8_t out[16] = { 0 };

  int32_t result = test_ulz ().uncompress (malformed, sizeof (malformed), out, sizeof (out));
  REQUIRE (result == ULZ::UncompressFailure);
}

TEST_CASE ("ULZ uncompress with overlapping copy (dist < length) [ulz]") {
  // cover overlapping copy with valid compressed data
  const char *pattern = "ABCDABCDABCDABCD";
  int32_t data_len = static_cast<int32_t> (strlen (pattern));

  uint8_t compressed[256];
  uint8_t restored[256];

  int32_t comp_len = test_ulz ().compress (reinterpret_cast<const uint8_t *> (pattern), data_len, compressed);
  REQUIRE (comp_len > 0);

  int32_t decomp_len = test_ulz ().uncompress (compressed, comp_len, restored, data_len);
  REQUIRE (decomp_len == data_len);
  REQUIRE (memcmp (pattern, restored, data_len) == 0);
}

TEST_CASE ("ULZ compress with data at window size boundary [ulz]") {
  // test with data size around window size (65536)
  const int32_t window_size = 65536;
  const int32_t data_len = window_size + 100; // slightly larger than window

  auto input_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len));
  auto comp_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len + ULZ::Excess));
  auto restore_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len));

  // fill with pattern that should compress well
  for (int32_t i = 0; i < data_len; ++i) {
    input_buf[i] = static_cast<uint8_t> ((i / 100) & 0xFF); // repeating pattern every 100 bytes
  }

  REQUIRE (round_trip (input_buf.get (), data_len, comp_buf.get (), restore_buf.get (), data_len));
}

TEST_CASE ("ULZ constants have correct values [ulz]") {
  REQUIRE (ULZ::Excess == 16);
  REQUIRE (ULZ::UncompressFailure == -1);
}

TEST_CASE ("ULZ maxCompressedSize is a sufficient buffer bound for pseudo-random data [ulz]") {
  const int32_t data_len = 100000;
  auto input_buf = make_unique<uint8_t[]> (static_cast<size_t> (data_len));
  auto comp_buf = make_unique<uint8_t[]> (static_cast<size_t> (ULZ::max_compressed_size (data_len)));

  uint32_t state = 0xfeedface;
  for (int32_t i = 0; i < data_len; ++i) {
    state = state * 1664525u + 1013904223u;
    input_buf[i] = static_cast<uint8_t> (state >> 24);
  }

  int32_t comp_len = test_ulz ().compress (input_buf.get (), data_len, comp_buf.get ());
  REQUIRE (comp_len > 0);
  REQUIRE (comp_len <= ULZ::max_compressed_size (data_len));
}

TEST_CASE ("ULZ compress returns positive length for compressible data [ulz]") {
  uint8_t input[100];
  memset (input, 'A', sizeof (input));
  uint8_t compressed[sizeof (input) + ULZ::Excess];

  int32_t comp_len = test_ulz ().compress (input, sizeof (input), compressed);
  REQUIRE (comp_len > 0);
  REQUIRE (comp_len < static_cast<int32_t> (sizeof (input))); // should compress
}

TEST_CASE ("ULZ uncompress with exact match at end of buffer [ulz]") {
  // create data where a match ends exactly at buffer end
  uint8_t input[128];
  for (int32_t i = 0; i < 128; ++i) {
    input[i] = static_cast<uint8_t> (i & 0x7F); // values 0-127
  }

  uint8_t compressed[256];
  uint8_t restored[128];

  int32_t comp_len = test_ulz ().compress (input, 128, compressed);
  REQUIRE (comp_len > 0);

  int32_t decomp_len = test_ulz ().uncompress (compressed, comp_len, restored, 128);
  REQUIRE (decomp_len == 128);
  REQUIRE (memcmp (input, restored, 128) == 0);
}
