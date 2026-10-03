// SPDX-License-Identifier: Unlicense

#pragma once

#include <stddef.h>

#include <ystl/string.h>
#include <ystl/singleton.h>
#include <ystl/traits.h>

namespace ystl {

// limited utf-8 codec + case folding toolkit
class Utf8Tools : public Singleton<Utf8Tools> {
private:
  struct Utf8Table {
    int32_t cmask, cval, shift;
    int32_t lmask, lval;

    constexpr Utf8Table (int32_t cmask_in, int32_t cval_in, int32_t shift_in, int32_t lmask_in, int32_t lval_in) :
      cmask (cmask_in), cval (cval_in), shift (shift_in), lmask (lmask_in), lval (lval_in) {}
  };

  struct FoldRange {
    int32_t lo, hi, delta;
    bool strided; // true: only lo, lo + 2, ... hi map (parity runs like latin extended)
  };

  // sorted ranges for unicode-aware case folding via upper-casing
  static constexpr FoldRange kFoldRanges[] = {
    { 0x0061, 0x007A, -32,  false },
    { 0x00E0, 0x00F6, -32,  false },
    { 0x00F8, 0x00FE, -32,  false },
    { 0x0101, 0x012F, -1,   true  },
    { 0x0133, 0x0137, -1,   true  },
    { 0x013A, 0x0148, -1,   true  },
    { 0x014B, 0x0177, -1,   true  },
    { 0x017A, 0x017E, -1,   true  },
    { 0x0183, 0x0185, -1,   true  },
    { 0x01A1, 0x01A5, -1,   true  },
    { 0x01B4, 0x01B6, -1,   true  },
    { 0x01CE, 0x01DC, -1,   true  },
    { 0x01DF, 0x01EF, -1,   true  },
    { 0x01F9, 0x021F, -1,   true  },
    { 0x0223, 0x0233, -1,   true  },
    { 0x0256, 0x0257, -205, false },
    { 0x028A, 0x028B, -217, false },
    { 0x03AD, 0x03AF, -37,  false },
    { 0x03B1, 0x03B8, -32,  false },
    { 0x03BA, 0x03BB, -32,  false },
    { 0x03BD, 0x03C1, -32,  false },
    { 0x03C3, 0x03CB, -32,  false },
    { 0x03CD, 0x03CE, -63,  false },
    { 0x03D9, 0x03EF, -1,   true  },
    { 0x0430, 0x044F, -32,  false },
    { 0x0450, 0x045F, -80,  false },
    { 0x0461, 0x0481, -1,   true  },
    { 0x048B, 0x04BF, -1,   true  },
    { 0x04C2, 0x04CE, -1,   true  },
    { 0x04D1, 0x04F5, -1,   true  },
    { 0x0501, 0x050F, -1,   true  },
    { 0x0561, 0x0586, -48,  false },
    { 0x1E01, 0x1E95, -1,   true  },
    { 0x1EA1, 0x1EF9, -1,   true  },
    { 0x1F00, 0x1F07, +8,   false },
    { 0x1F10, 0x1F15, +8,   false },
    { 0x1F20, 0x1F27, +8,   false },
    { 0x1F30, 0x1F37, +8,   false },
    { 0x1F40, 0x1F45, +8,   false },
    { 0x1F51, 0x1F57, +8,   true  },
    { 0x1F60, 0x1F67, +8,   false },
    { 0x1F70, 0x1F71, +74,  false },
    { 0x1F72, 0x1F75, +86,  false },
    { 0x1F76, 0x1F77, +100, false },
    { 0x1F78, 0x1F79, +128, false },
    { 0x1F7A, 0x1F7B, +112, false },
    { 0x1F7C, 0x1F7D, +126, false },
    { 0x1F80, 0x1F87, +8,   false },
    { 0x1F90, 0x1F97, +8,   false },
    { 0x1FA0, 0x1FA7, +8,   false },
    { 0x1FB0, 0x1FB1, +8,   false },
    { 0x1FD0, 0x1FD1, +8,   false },
    { 0x1FE0, 0x1FE1, +8,   false },
    { 0x2170, 0x217F, -16,  false },
    { 0x24D0, 0x24E9, -26,  false },
    { 0xFF41, 0xFF5A, -32,  false }
  };

  struct FoldSingle {
    int32_t from, to;
  };

  // leftovers that fit no range (odd deltas like turkish dotless i)
  static constexpr FoldSingle kFoldSingles[] = {
    { 0x00FF, 0x0178 },
    { 0x0131, 0x0049 },
    { 0x0188, 0x0187 },
    { 0x018C, 0x018B },
    { 0x0192, 0x0191 },
    { 0x0195, 0x01F6 },
    { 0x0199, 0x0198 },
    { 0x019E, 0x0220 },
    { 0x01A8, 0x01A7 },
    { 0x01AD, 0x01AC },
    { 0x01B0, 0x01AF },
    { 0x01B9, 0x01B8 },
    { 0x01BD, 0x01BC },
    { 0x01BF, 0x01F7 },
    { 0x01C6, 0x01C4 },
    { 0x01C9, 0x01C7 },
    { 0x01CC, 0x01CA },
    { 0x01DD, 0x018E },
    { 0x01F3, 0x01F1 },
    { 0x01F5, 0x01F4 },
    { 0x0253, 0x0181 },
    { 0x0254, 0x0186 },
    { 0x0259, 0x018F },
    { 0x025B, 0x0190 },
    { 0x0260, 0x0193 },
    { 0x0263, 0x0194 },
    { 0x0268, 0x0197 },
    { 0x0269, 0x0196 },
    { 0x026F, 0x019C },
    { 0x0272, 0x019D },
    { 0x0275, 0x019F },
    { 0x0280, 0x01A6 },
    { 0x0283, 0x01A9 },
    { 0x0288, 0x01AE },
    { 0x0292, 0x01B7 },
    { 0x03AC, 0x0386 },
    { 0x03B9, 0x0345 },
    { 0x03BC, 0x00B5 },
    { 0x03CC, 0x038C },
    { 0x03F2, 0x03F9 },
    { 0x03F8, 0x03F7 },
    { 0x03FB, 0x03FA },
    { 0x04F9, 0x04F8 },
    { 0x1FB3, 0x1FBC },
    { 0x1FC3, 0x1FCC },
    { 0x1FE5, 0x1FEC },
    { 0x1FF3, 0x1FFC }
  };

  // sorted ranges for lower-casing (inverse of the fold table)
  static constexpr FoldRange kFoldLowerRanges[] = {
    { 0x0041, 0x005A, +32,  false },
    { 0x00C0, 0x00D6, +32,  false },
    { 0x00D8, 0x00DE, +32,  false },
    { 0x0100, 0x012E, +1,   true  },
    { 0x0132, 0x0136, +1,   true  },
    { 0x0139, 0x0147, +1,   true  },
    { 0x014A, 0x0176, +1,   true  },
    { 0x0179, 0x017D, +1,   true  },
    { 0x0182, 0x0184, +1,   true  },
    { 0x0189, 0x018A, +205, false },
    { 0x01A0, 0x01A4, +1,   true  },
    { 0x01B1, 0x01B2, +217, false },
    { 0x01B3, 0x01B5, +1,   true  },
    { 0x01CD, 0x01DB, +1,   true  },
    { 0x01DE, 0x01EE, +1,   true  },
    { 0x01F8, 0x021E, +1,   true  },
    { 0x0222, 0x0232, +1,   true  },
    { 0x0388, 0x038A, +37,  false },
    { 0x038E, 0x038F, +63,  false },
    { 0x0391, 0x0398, +32,  false },
    { 0x039A, 0x039B, +32,  false },
    { 0x039D, 0x03A1, +32,  false },
    { 0x03A3, 0x03AB, +32,  false },
    { 0x03D8, 0x03EE, +1,   true  },
    { 0x0400, 0x040F, +80,  false },
    { 0x0410, 0x042F, +32,  false },
    { 0x0460, 0x0480, +1,   true  },
    { 0x048A, 0x04BE, +1,   true  },
    { 0x04C1, 0x04CD, +1,   true  },
    { 0x04D0, 0x04F4, +1,   true  },
    { 0x0500, 0x050E, +1,   true  },
    { 0x0531, 0x0556, +48,  false },
    { 0x1E00, 0x1E94, +1,   true  },
    { 0x1EA0, 0x1EF8, +1,   true  },
    { 0x1F08, 0x1F0F, -8,   false },
    { 0x1F18, 0x1F1D, -8,   false },
    { 0x1F28, 0x1F2F, -8,   false },
    { 0x1F38, 0x1F3F, -8,   false },
    { 0x1F48, 0x1F4D, -8,   false },
    { 0x1F59, 0x1F5F, -8,   true  },
    { 0x1F68, 0x1F6F, -8,   false },
    { 0x1F88, 0x1F8F, -8,   false },
    { 0x1F98, 0x1F9F, -8,   false },
    { 0x1FA8, 0x1FAF, -8,   false },
    { 0x1FB8, 0x1FB9, -8,   false },
    { 0x1FBA, 0x1FBB, -74,  false },
    { 0x1FC8, 0x1FCB, -86,  false },
    { 0x1FD8, 0x1FD9, -8,   false },
    { 0x1FDA, 0x1FDB, -100, false },
    { 0x1FE8, 0x1FE9, -8,   false },
    { 0x1FEA, 0x1FEB, -112, false },
    { 0x1FF8, 0x1FF9, -128, false },
    { 0x1FFA, 0x1FFB, -126, false },
    { 0x2160, 0x216F, +16,  false },
    { 0x24B6, 0x24CF, +26,  false },
    { 0xFF21, 0xFF3A, +32,  false }
  };

  // lower-case leftovers that fit no range
  static constexpr FoldSingle kFoldLowerSingles[] = {
    { 0x00B5, 0x03BC },
    { 0x0178, 0x00FF },
    { 0x0181, 0x0253 },
    { 0x0186, 0x0254 },
    { 0x0187, 0x0188 },
    { 0x018B, 0x018C },
    { 0x018E, 0x01DD },
    { 0x018F, 0x0259 },
    { 0x0190, 0x025B },
    { 0x0191, 0x0192 },
    { 0x0193, 0x0260 },
    { 0x0194, 0x0263 },
    { 0x0196, 0x0269 },
    { 0x0197, 0x0268 },
    { 0x0198, 0x0199 },
    { 0x019C, 0x026F },
    { 0x019D, 0x0272 },
    { 0x019F, 0x0275 },
    { 0x01A6, 0x0280 },
    { 0x01A7, 0x01A8 },
    { 0x01A9, 0x0283 },
    { 0x01AC, 0x01AD },
    { 0x01AE, 0x0288 },
    { 0x01AF, 0x01B0 },
    { 0x01B7, 0x0292 },
    { 0x01B8, 0x01B9 },
    { 0x01BC, 0x01BD },
    { 0x01C4, 0x01C6 },
    { 0x01C7, 0x01C9 },
    { 0x01CA, 0x01CC },
    { 0x01F1, 0x01F3 },
    { 0x01F4, 0x01F5 },
    { 0x01F6, 0x0195 },
    { 0x01F7, 0x01BF },
    { 0x0220, 0x019E },
    { 0x0345, 0x03B9 },
    { 0x0386, 0x03AC },
    { 0x038C, 0x03CC },
    { 0x03F7, 0x03F8 },
    { 0x03F9, 0x03F2 },
    { 0x03FA, 0x03FB },
    { 0x04F8, 0x04F9 },
    { 0x1FBC, 0x1FB3 },
    { 0x1FCC, 0x1FC3 },
    { 0x1FEC, 0x1FE5 },
    { 0x1FFC, 0x1FF3 }
  };

private:
  // single binary-search body shared by toUpper/toLower over any fold tables
  template <size_t RangeCount, size_t SingleCount>
  static constexpr wchar_t fold_lookup (wchar_t ch, const FoldRange (&ranges)[RangeCount], const FoldSingle (&singles)[SingleCount]) noexcept {
    size_t bottom = 0;
    size_t top = RangeCount;

    while (bottom < top) {
      const auto mid = bottom + (top - bottom) / 2;
      const auto &range = ranges[mid];

      if (ch < range.lo) {
        top = mid;
        continue;
      }

      if (ch > range.hi) {
        bottom = mid + 1;
        continue;
      }

      if (!range.strided || ((ch - range.lo) & 1) == 0) {
        return static_cast<wchar_t> (ch + range.delta);
      }
      break; // inside the span but wrong parity: unmapped
    }
    bottom = 0;
    top = SingleCount;

    while (bottom < top) {
      const auto mid = bottom + (top - bottom) / 2;
      const auto cur = static_cast<wchar_t> (singles[mid].from);

      if (ch == cur) {
        return static_cast<wchar_t> (singles[mid].to);
      }

      if (ch > cur) {
        bottom = mid + 1;
      }
      else {
        top = mid;
      }
    }
    return ch;
  }

  // shared in-place case mapper over any fold function, output never grows input
  template <wchar_t (*FoldFn) (wchar_t) noexcept> String &map_case (StringRef in, String &dest) {
    dest.assign (in.chars (), in.size ());

    const auto input_len = dest.size ();
    size_t pos = 0;
    size_t out_pos = 0;

    while (pos < input_len) {
      wchar_t wide = 0;

      const auto consumed = multi_byte_to_wide_char (&wide, dest.chars () + pos);

      if (consumed < 0) {
        dest[out_pos++] = dest[pos++];
        continue;
      }
      const auto produced = wide_char_to_multi_byte (&dest[out_pos], FoldFn (wide));

      out_pos += produced > 0 ? static_cast<size_t> (produced) : static_cast<size_t> (consumed);
      pos += static_cast<size_t> (consumed);
    }

    // shrink the length when the folded form got shorter (like 0x0131 to 0x0049)
    if (out_pos < input_len) {
      dest.erase (out_pos, input_len - out_pos);
    }
    dest[out_pos] = kNullChar;

    return dest;
  }

  SmallArray<Utf8Table> utf_table_;

private:
  void build_table () {
    utf_table_.emplace (0x80, 0x00, 0 * 6, 0x7f, 0); // 1 byte sequence
    utf_table_.emplace (0xe0, 0xc0, 1 * 6, 0x7ff, 0x80); // 2 byte sequence
    utf_table_.emplace (0xf0, 0xe0, 2 * 6, 0xffff, 0x800); // 3 byte sequence
    utf_table_.emplace (0xf8, 0xf0, 3 * 6, 0x1fffff, 0x10000); // 4 byte sequence
    utf_table_.emplace (0xfc, 0xf8, 4 * 6, 0x3ffffff, 0x200000); // 5 byte sequence
    utf_table_.emplace (0xfe, 0xfc, 5 * 6, 0x7fffffff, 0x4000000); // 6 byte sequence
  }

  int32_t multi_byte_to_wide_char (wchar_t *wide, const char *mbs) {
    int32_t len = 0;

    auto ch = *mbs;
    auto lval = static_cast<int> (ch);

    for (const auto &table : utf_table_) {
      len++;

      if ((ch & table.cmask) == table.cval) {
        lval &= table.lmask;

        if (lval < table.lval) {
          return -1;
        }
        *wide = static_cast<wchar_t> (lval);
        return len;
      }
      mbs++;
      auto test = (*mbs ^ 0x80) & 0xff;

      if (test & 0xc0) {
        return -1;
      }
      lval = (lval << 6) | test;
    }
    return -1;
  }

  int32_t wide_char_to_multi_byte (char *mbs, wchar_t wide) {
    if (!mbs) {
      return 0;
    }
    int32_t lmask = static_cast<int32_t> (wide);
    int32_t len = 0;

    for (const auto &table : utf_table_) {
      len++;

      if (lmask <= table.lmask) {
        auto ch = table.shift;
        *mbs = static_cast<char> (table.cval | (lmask >> ch));

        while (ch > 0) {
          ch -= 6;
          mbs++;

          *mbs = static_cast<char> (0x80 | ((lmask >> ch) & 0x3F));
        }
        return len;
      }
    }
    return -1;
  }

public:
  Utf8Tools () {
    build_table ();
  }

  ~Utf8Tools () = default;

public:
  // fold one code point to upper-case, identity when no mapping exists
  static constexpr wchar_t to_upper (wchar_t ch) noexcept {
    return fold_lookup (ch, kFoldRanges, kFoldSingles);
  }

  // fold one code point to lower-case, identity when no mapping exists (I folds to i)
  static constexpr wchar_t to_lower (wchar_t ch) noexcept {
    return fold_lookup (ch, kFoldLowerRanges, kFoldLowerSingles);
  }

  // reuse the in-place overload, so sso inputs need no heap traffic
  String str_to_upper (StringRef in) {
    String result;
    str_to_upper (in, result);

    return result;
  }

  // upper-case into dest reusing its buffer, output never grows input
  String &str_to_upper (StringRef in, String &dest) {
    return map_case<to_upper> (in, dest);
  }

  // reuse the in-place overload, so sso inputs need no heap traffic
  String str_to_lower (StringRef in) {
    String result;
    str_to_lower (in, result);

    return result;
  }

  // lower-case into dest reusing its buffer, output never grows input
  String &str_to_lower (StringRef in, String &dest) {
    return map_case<to_lower> (in, dest);
  }

  // utf-8 continuation byte: 10xxxxxx
  static constexpr bool is_continuation (char ch) noexcept {
    return (static_cast<uint8_t> (ch) & 0xC0) == 0x80;
  }

  // shrink len so [offset, offset + len) never cuts a multibyte sequence;
  // always keeps at least one byte, so chunk loops terminate even on invalid input
  static size_t clamp_chunk_len (StringRef str, size_t offset, size_t max_len) {
    const auto total = str.size ();

    if (offset >= total || max_len == 0) {
      return 0;
    }
    auto len = ystl::min (max_len, total - offset);

    while (len > 1 && offset + len < total && is_continuation (str[offset + len])) {
      --len;
    }
    return len;
  }

  // iterate utf-8 safe chunks of at most maxLen bytes
  template <typename Fn> static void for_each_chunk (StringRef str, size_t max_len, Fn &&fn) {
    auto offset = static_cast<size_t> (0);

    while (offset < str.size ()) {
      const auto len = clamp_chunk_len (str, offset, max_len);

      if (len == 0) {
        break; // maxLen zero: nothing to emit
      }
      fn (str.substr (offset, len));
      offset += len;
    }
  }
};

// expose global utf8 tools
YSTL_EXPOSE_GLOBAL_SINGLETON (Utf8Tools, utf8tools);

}
