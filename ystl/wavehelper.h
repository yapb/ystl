// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/endian.h>
#include <ystl/files.h>

namespace ystl {

// helper class for reading wave header
class WaveHelper final {
public:
  struct Header {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_chunk_id[4];
    uint32_t data_chunk_length;
  };

public:
  WaveHelper () = default;
  ~WaveHelper () = default;

public:
  template <typename U> U read16 (uint16_t value) const {
    return static_cast<U> (ByteOrder::swap16 (value));
  }

  template <typename U> U read32 (uint32_t value) const {
    return static_cast<U> (ByteOrder::swap32 (value));
  }

public:
  bool is_wave (const char *format) const {
    return memcmp (format, "WAVE", 4) == 0;
  }

  // calculates the duration of a wav file in seconds
  float get_duration (ystl::MemFile *fp) const {
    constexpr auto kZeroLength = 0.0f;

    if (!fp || !*fp) {
      return kZeroLength;
    }
    char riff[4] {};
    uint32_t riff_size = 0;
    char wave[4] {};

    if (fp->read (riff, sizeof (riff)) != 1 || fp->le_read (riff_size) != 1 || fp->read (wave, sizeof (wave)) != 1) {
      return kZeroLength;
    }

    if (memcmp (riff, "RIFF", 4) != 0 || !is_wave (wave)) {
      return kZeroLength;
    }
    uint16_t bits_per_sample = 0, num_channels = 0;
    uint32_t sample_rate = 0, data_length = 0;
    bool have_fmt = false;

    // extra chunks (LIST, fact, smpl, bext) may sit between fmt and data
    while (!fp->eof ()) {
      char id[4] {};
      uint32_t chunk_size = 0;

      if (fp->read (id, sizeof (id)) != 1 || fp->le_read (chunk_size) != 1) {
        break;
      }

      if (memcmp (id, "fmt ", 4) == 0) {
        uint8_t fmt[16] {};

        if (chunk_size < sizeof (fmt) || fp->read (fmt, sizeof (fmt)) != 1) [[unlikely]] {
          return kZeroLength;
        }
        num_channels = read16<uint16_t> (static_cast<uint16_t> (fmt[2] | (fmt[3] << 8)));
        sample_rate = read32<uint32_t> (static_cast<uint32_t> (fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | (fmt[7] << 24)));
        bits_per_sample = read16<uint16_t> (static_cast<uint16_t> (fmt[14] | (fmt[15] << 8)));
        have_fmt = true;

        if (chunk_size > sizeof (fmt) && !fp->seek (chunk_size - sizeof (fmt), SEEK_CUR)) {
          break;
        }
      }
      else if (memcmp (id, "data", 4) == 0) {
        data_length = chunk_size;
        break;
      }
      else {
        const auto skip = chunk_size + (chunk_size & 1u);

        if (skip < chunk_size || !fp->seek (skip, SEEK_CUR)) {
          break;
        }
      }
    }

    if (!have_fmt || data_length == 0) [[unlikely]] {
      return kZeroLength;
    }
    const auto bps = read16<float> (bits_per_sample) / 8.0f;
    const auto channels = read16<float> (num_channels);
    const auto rate = read32<float> (sample_rate);

    // prevent division by zero
    if (bps <= 0.0f || channels <= 0.0f || rate <= 0.0f) {
      return kZeroLength;
    }
    return static_cast<float> (data_length) / (bps * channels * rate);
  }
};
} // namespace ystl
