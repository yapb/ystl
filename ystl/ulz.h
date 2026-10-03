// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/algorithm.h>
#include <ystl/array.h>
#include <ystl/endian.h>
#include <ystl/mathlib.h>
#include <ystl/span.h>
#include <ystl/utility.h>

namespace ystl {

// fast lz-family compressor with a 128 kib search window
class ULZ final {
public:
  enum : int32_t {
    Excess = 16, // small slack for caller-provided output buffers
    UncompressFailure = -1
  };

public:
  // conservative upper bound for the compressed size of `n` input bytes
  static constexpr int32_t max_compressed_size (int32_t n) {
    return n + n / 64 + 64;
  }

private:
  enum : int32_t {
    WindowBits = 17,
    WindowSize = ystl::bit (WindowBits), // search window: 128 kib
    WindowMask = WindowSize - 1,

    MinMatch = 4, // shortest match worth encoding
    MaxChain = ystl::bit (5), // max hash-chain hops per candidate

    // 17-bit hash keeps the table at 512 kib (vs 2 mib for 19 bits) with a
    // negligible ratio hit; maxchain compensates. internal-only: the stream
    // format does not depend on hash parameters
    HashBits = 17,
    HashLength = ystl::bit (HashBits), // hash table: 128k slots
    EmptyHash = -1, // hash-slot sentinel (no candidate)

    LiteralRunMax = 7, // 3-bit literal-run field in the token
    TokenRunShift = 5, // run occupies token bits 5..7
    TokenMatchBits = 4, // match length occupies token bits 0..3
    TokenDistBit = 16, // token bit 4 = high bit of the distance
    MatchLengthFieldMax = 15, // 4-bit length field, then a varint follows
    VarIntThreshold = 128, // varint continuation bit
  };

  SmallArray<int32_t> hash_table_ {};
  SmallArray<int32_t> prev_table_ {};

private:
  // distances are stored little-endian so archives are byte-order portable
  static uint32_t load16 (const uint8_t *ptr) {
    return ystl::ByteOrder::read_le<uint16_t> (ptr);
  }

  static void store16 (uint8_t *ptr, uint16_t val) {
    ystl::ByteOrder::write_le<uint16_t> (ptr, val);
  }

  // used only for the internal hash, never part of the on-disk format
  static uint32_t load32 (const uint8_t *ptr) {
    uint32_t ret;
    memcpy (&ret, ptr, sizeof (uint32_t));

    return ret;
  }

  static void copy8 (uint8_t *dst, const uint8_t *src) {
    memcpy (dst, src, sizeof (uint64_t));
  }

  static void wild_copy (uint8_t *dst, const uint8_t *src, int32_t count) {
    int32_t i = 0;
    for (; i + 8 <= count; i += 8) {
      copy8 (dst + i, src + i);
    }

    for (; i < count; ++i) {
      dst[i] = src[i];
    }
  }

  static uint32_t hash32 (const uint8_t *ptr) {
    return (load32 (ptr) * 0x9e3779b9) >> (32 - HashBits);
  }

  static void emit_byte (uint8_t *&dst, int32_t val) {
    *dst++ = static_cast<uint8_t> (val);
  }

  // 7-bit varint encoding with high-bit continuation flag
  static void encode_var_int (uint8_t *&ptr, uint32_t val) {
    while (val >= VarIntThreshold) {
      val -= VarIntThreshold;

      *ptr++ = VarIntThreshold + (val & (VarIntThreshold - 1));
      val >>= 7;
    }
    *ptr++ = static_cast<uint8_t> (val);
  }

  static uint32_t decode_var_int (const uint8_t *&ptr, const uint8_t *end) {
    uint32_t val = 0;

    // 5 bytes max (i <= 28): covers any uint32 that encodevarint can emit
    for (int32_t i = 0; i <= 28; i += 7) {
      if (ptr >= end) {
        return val;
      }
      const uint32_t cur = *ptr++;
      val += cur << i;

      if (cur < VarIntThreshold) {
        break;
      }
    }
    return val;
  }

  void update_chain (const uint8_t *in, int32_t pos) {
    const auto hash = hash32 (&in[pos]);

    // slots alias via window mask and get reused past window size
    prev_table_[pos & WindowMask] = hash_table_[hash];
    hash_table_[hash] = pos;
  }

  // walks the hash chain (at most maxchain hops) looking for the longest match at in[cur]
  int32_t find_best_match (const uint8_t *in, int32_t cur, int32_t max_match, int32_t stop_at, int32_t &dist) const {
    const auto limit = ystl::max<int32_t> (cur - WindowSize, EmptyHash);

    int32_t chain_length = MaxChain;
    int32_t lookup = hash_table_[hash32 (&in[cur])];
    int32_t best_length = 0;

    while (lookup > limit) {
      if (in[lookup + best_length] == in[cur + best_length] && load32 (&in[lookup]) == load32 (&in[cur])) {
        int32_t length = MinMatch;

        while (length < max_match && in[lookup + length] == in[cur + length]) {
          ++length;
        }

        if (length > best_length) {
          best_length = length;
          dist = cur - lookup;

          if (stop_at > 0 && length >= stop_at) {
            return length;
          }
          if (length == max_match) {
            break;
          }
        }
      }

      if (--chain_length == 0) {
        break;
      }
      lookup = prev_table_[lookup & WindowMask];
    }
    return best_length;
  }

  // returns true if position `next` holds a match of at least `target` bytes
  bool has_lazy_match (const uint8_t *in, int32_t next, int32_t target, int32_t input_length) const {
    if (next + target > input_length || next > input_length - MinMatch) {
      return false;
    }
    int32_t dist = 0;
    return find_best_match (in, next, target, target, dist) == target;
  }

  void emit_literals (uint8_t *&op, const uint8_t *in, int32_t anchor, int32_t cur, int32_t token) {
    const auto run = cur - anchor;

    if (run >= LiteralRunMax) {
      emit_byte (op, (LiteralRunMax << TokenRunShift) + token);
      encode_var_int (op, static_cast<uint32_t> (run - LiteralRunMax));
    }
    else {
      emit_byte (op, (run << TokenRunShift) + token);
    }
    wild_copy (op, &in[anchor], run);
    op += run;
  }

  // encoder tables allocated lazily and released after compress
  void acquire_tables () {
    if (hash_table_.empty ()) {
      hash_table_.resize (HashLength);
      prev_table_.resize (WindowSize);
    }
  }

  void release_tables () {
    // move-assign from a fresh sbo-backed array to free the heap buffers
    hash_table_ = SmallArray<int32_t> {};
    prev_table_ = SmallArray<int32_t> {};
  }

public:
  ULZ () = default;

  ~ULZ () = default;

public:
  int32_t compress (const uint8_t *in, int32_t input_length, uint8_t *out) {
    acquire_tables ();

    ystl::fill (hash_table_.data (), hash_table_.size (), EmptyHash);
    ystl::fill (prev_table_.data (), prev_table_.size (), EmptyHash);

    auto op = out;

    int32_t anchor = 0;
    int32_t cur = 0;

    while (cur < input_length) {
      const int32_t max_match = input_length - cur;

      int32_t dist = 0;
      int32_t best_length = 0;

      if (max_match >= MinMatch) {
        best_length = find_best_match (in, cur, max_match, 0, dist);
      }

      // a 4-byte match is not worth it once the pending literal run already needs a varint continuation byte
      if (best_length == MinMatch && (cur - anchor) >= (LiteralRunMax + VarIntThreshold)) {
        best_length = 0;
      }

      // lazy matching: prefer a longer match at the next position, except for the exact-6-literal-run case
      if (best_length >= MinMatch && best_length < max_match && (cur - anchor) != 6) {
        if (has_lazy_match (in, cur + 1, best_length + 1, input_length)) {
          best_length = 0;
        }
      }

      if (best_length >= MinMatch) {
        const auto length = best_length - MinMatch;
        const auto token = ((dist >> 12) & TokenDistBit) + ystl::min<int32_t> (length, MatchLengthFieldMax);

        if (anchor != cur) {
          emit_literals (op, in, anchor, cur, token);
        }
        else {
          emit_byte (op, token);
        }

        if (length >= MatchLengthFieldMax) {
          encode_var_int (op, static_cast<uint32_t> (length - MatchLengthFieldMax));
        }
        store16 (op, static_cast<uint16_t> (dist));
        op += 2;

        while (best_length-- != 0) {
          if (cur <= input_length - MinMatch) {
            update_chain (in, cur);
          }
          ++cur;
        }
        anchor = cur;
      }
      else {
        if (cur <= input_length - MinMatch) {
          update_chain (in, cur);
        }
        ++cur;
      }
    }

    if (anchor != cur) {
      emit_literals (op, in, anchor, cur, 0);
    }
    release_tables ();
    return static_cast<int32_t> (op - out);
  }

  int32_t uncompress (const uint8_t *in, int32_t input_length, uint8_t *out, int32_t out_length) {
    auto op = out;
    auto ip = in;

    const auto op_end = op + out_length;
    const auto ip_end = ip + input_length;

    while (ip < ip_end) {
      const auto token = *ip++;

      // token bits 5..7 hold the literal-run length (0 = no literals)
      if ((token >> TokenRunShift) != 0) {
        auto run = static_cast<int32_t> (token >> TokenRunShift);

        if (run == LiteralRunMax) {
          run += static_cast<int32_t> (decode_var_int (ip, ip_end));
        }

        if ((op_end - op) < run || (ip_end - ip) < run) {
          return UncompressFailure;
        }
        wild_copy (op, ip, run);

        op += run;
        ip += run;

        // a well-formed stream may end right after the final literal run
        if (ip >= ip_end) {
          break;
        }
      }
      auto length = static_cast<int32_t> ((token & MatchLengthFieldMax) + MinMatch);

      if (length == (MatchLengthFieldMax + MinMatch)) {
        length += static_cast<int32_t> (decode_var_int (ip, ip_end));
      }

      if ((ip_end - ip) < 2) {
        return UncompressFailure;
      }

      if ((op_end - op) < length) {
        return UncompressFailure;
      }
      const auto dist = static_cast<int32_t> (static_cast<uint32_t> ((token & TokenDistBit) << 12) + load16 (ip));
      ip += 2;

      if (dist == 0 || (op - out) < dist) {
        return UncompressFailure;
      }
      auto cp = op - dist;

      if (dist >= 8) {
        wild_copy (op, cp, length);
        op += length;
      }
      else {
        *op++ = *cp++;
        *op++ = *cp++;
        *op++ = *cp++;
        *op++ = *cp++;

        while (length-- != 4) {
          *op++ = *cp++;
        }
      }
    }
    // success needs fully consumed input and fully filled output
    return (ip == ip_end && op == op_end) ? static_cast<int32_t> (op - out) : UncompressFailure;
  }

  // span overloads
  int32_t compress (const ystl::Span<const uint8_t> &in, const ystl::Span<uint8_t> &out) {
    return compress (in.data (), static_cast<int32_t> (in.size ()), out.data ());
  }

  int32_t uncompress (const ystl::Span<const uint8_t> &in, const ystl::Span<uint8_t> &out) {
    return uncompress (in.data (), static_cast<int32_t> (in.size ()), out.data (), static_cast<int32_t> (out.size ()));
  }
};

}
