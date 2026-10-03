// test_wavehelper.cpp - tests for ystl/wavehelper.h
#include <ystl/ystl.h>
#include <ystl/test.h>

#include <string.h>

using namespace ystl;

// wavehelper - tests the
TEST_CASE ("WaveHelper::read16 returns value with platform byte order [wavehelper]") {
  WaveHelper wh;
  // wav is little-endian
  REQUIRE (wh.read16<uint16_t> (ByteOrder::to_le<uint16_t> (0x1234u)) == 0x1234u);
  REQUIRE (wh.read16<int16_t> (ByteOrder::to_le<uint16_t> (1u)) == 1);
  REQUIRE (wh.read16<uint16_t> (0u) == 0u);
}

TEST_CASE ("WaveHelper::read32 returns value with platform byte order [wavehelper]") {
  WaveHelper wh;
  // wav is little-endian
  REQUIRE (wh.read32<uint32_t> (ByteOrder::to_le<uint32_t> (0x12345678u)) == 0x12345678u);
  REQUIRE (wh.read32<int32_t> (ByteOrder::to_le<uint32_t> (100u)) == 100);
}

TEST_CASE ("WaveHelper::isWave returns true for WAVE identifier [wavehelper]") {
  WaveHelper wh;
  char wave[4] = { 'W', 'A', 'V', 'E' };
  REQUIRE (wh.is_wave (wave));
}

TEST_CASE ("WaveHelper::isWave returns false for non-WAVE identifier [wavehelper]") {
  WaveHelper wh;
  char data[4] = { 'R', 'I', 'F', 'F' };
  REQUIRE_FALSE (wh.is_wave (data));

  char zeros[4] = { 0, 0, 0, 0 };
  REQUIRE_FALSE (wh.is_wave (zeros));
}

// header struct sanity
TEST_CASE ("WaveHelper::Header has expected size (44 bytes) [wavehelper]") {
  // standard pcm wav header is 44 bytes
  REQUIRE (sizeof (WaveHelper::Header) == 44u);
}

TEST_CASE ("WaveHelper reads a synthesised WAV header correctly [wavehelper]") {
  WaveHelper wh;

  // construct a header with le-encoded fields (as wav is stored on disk)
  WaveHelper::Header hdr {};
  memcpy (hdr.riff, "RIFF", 4);
  memcpy (hdr.wave, "WAVE", 4);
  memcpy (hdr.fmt, "fmt ", 4);
  hdr.subchunk1_size = ByteOrder::to_le<uint32_t> (16u);
  hdr.audio_format = ByteOrder::to_le<uint16_t> (1u); // pcm
  hdr.num_channels = ByteOrder::to_le<uint16_t> (2u); // stereo
  hdr.sample_rate = ByteOrder::to_le<uint32_t> (44100u);
  hdr.byte_rate = ByteOrder::to_le<uint32_t> (176400u);
  hdr.block_align = ByteOrder::to_le<uint16_t> (4u);
  hdr.bits_per_sample = ByteOrder::to_le<uint16_t> (16u);
  memcpy (hdr.data_chunk_id, "data", 4);
  hdr.data_chunk_length = ByteOrder::to_le<uint32_t> (1024u);

  // read16/read32 convert le -> native
  REQUIRE (wh.read16<uint16_t> (hdr.audio_format) == 1u);
  REQUIRE (wh.read16<uint16_t> (hdr.num_channels) == 2u);
  REQUIRE (wh.read32<uint32_t> (hdr.sample_rate) == 44100u);
  REQUIRE (wh.read32<uint32_t> (hdr.byte_rate) == 176400u);
  REQUIRE (wh.read16<uint16_t> (hdr.bits_per_sample) == 16u);
  REQUIRE (wh.is_wave (hdr.wave));
}

// duration skips extra chunks between fmt and data
TEST_CASE ("WaveHelper get_duration skips extra chunks [wavehelper]") {
  const char *fname = "ystl_test_extra_chunk.tmp";
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  // riff + fmt(16) + LIST(4) + data(1024): 16-bit stereo 44100
  const uint8_t wav[] = { 'R', 'I', 'F', 'F', 0x30, 0x00, 0x00, 0x00, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0, 0x44,
    0xAC, 0, 0, 0x10, 0xB1, 0x02, 0x00, 4, 0, 16, 0, 'L', 'I', 'S', 'T', 4, 0, 0, 0, 'a', 'b', 'c', 'd', 'd', 'a', 't', 'a', 0x00, 0x04, 0x00,
    0x00 };
  {
    File fw (fname, "wb");
    REQUIRE (fw);
    REQUIRE (fw.write (wav, sizeof (wav)) == 1);
  }
  MemFile mf (fname);

  REQUIRE (mf);
  const auto duration = WaveHelper {}.get_duration (&mf);
  REQUIRE (duration > 0.005f);
  REQUIRE (duration < 0.006f);
  plat.remove_file (fname);
}
