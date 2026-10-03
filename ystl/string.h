// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <compare>

#include <ystl/mathlib.h>
#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/singleton.h>
#include <ystl/utility.h>

#include <ystl/uniqueptr.h>
#include <ystl/array.h>

#if defined(YSTL_USE_VENDORED_PRINTF)
  // vendored standalone snprintf (ext/ystl/printf), exposes snprintf_ directly
  #include <printf/printf.h>
#endif

namespace ystl {

class StringRef;
class String;

// helper for null-termination
static constexpr char kNullChar = '\0';

// overloaded version of snprintf to take string and stringref as char arrays
class SNPrintfWrap final {
public:
  enum : size_t {
    ScratchSlots = static_cast<size_t> (16), // max views per call, casts evaluate upfront
    ScratchSize = static_cast<size_t> (2048)
  };

public:
  explicit SNPrintfWrap () = default;
  ~SNPrintfWrap ();

private:
  template <typename U> U cast (U value) {
    return value;
  }

  const char *cast (const class String &value);
  const char *cast (const class StringRef &value);

  // copies a non-terminated view into a null-terminated scratch slot, so %s never reads past it
  YSTL_NOINLINE inline const char *terminate_scratch (const char *chars, const size_t length) noexcept;

private:
  FixedArray<FixedArray<char, ScratchSize + 1>, ScratchSlots> static_scratch_ {};
  FixedArray<char *, ScratchSlots> grown_scratch_ {};
  FixedArray<size_t, ScratchSlots> grown_capacity_ {};

  size_t rotate_ = 0;

private:
  // single backend switch for the whole tu: vendored eyalroz printf vs libc
  template <typename... Args> int32_t call_snprintf (char *buffer, size_t max_size, const char *fmt, Args &&...args) {
    // runtime fmt by design, rotating pool trips -Wrestrict on gcc
    YSTL_DISABLE_RUNTIME_FORMAT_WARNING
#if defined(YSTL_USE_VENDORED_PRINTF)
    const auto result = snprintf_ (buffer, max_size, fmt, cast (ystl::forward<Args> (args))...);
#else
    const auto result = snprintf (buffer, max_size, fmt, cast (ystl::forward<Args> (args))...);
#endif
    YSTL_RESTORE_RUNTIME_FORMAT_WARNING
    return result;
  }

  // va_list backend for same switch
  int32_t call_vsnprintf (char *buffer, size_t max_size, const char *fmt, va_list args) {
    // runtime fmt by design, see call_snprintf above
    YSTL_DISABLE_RUNTIME_FORMAT_WARNING
#if defined(YSTL_USE_VENDORED_PRINTF)
    const auto result = vsnprintf_ (buffer, max_size, fmt, args);
#else
    const auto result = vsnprintf (buffer, max_size, fmt, args);
#endif
    YSTL_RESTORE_RUNTIME_FORMAT_WARNING
    return result;
  }

public:
  template <typename... Args> int32_t exec (char *buffer, const size_t max_size, const char *fmt, Args &&...args) {
    if (buffer && max_size > 0) {
      buffer[0] = kNullChar;
    }
    return call_snprintf (buffer, max_size, fmt, ystl::forward<Args> (args)...);
  }

  int32_t vexec (char *buffer, size_t max_size, const char *fmt, va_list args) {
    if (buffer && max_size > 0) {
      buffer[0] = kNullChar;
    }
    return call_vsnprintf (buffer, max_size, fmt, args);
  }
};

inline SNPrintfWrap &fmtwrap () {
  // single-threaded by contract, scratch slots are shared mutable state (same for the Strings pool below)
  static SNPrintfWrap instance {};
  return instance;
}

// detail helpers for string searching
namespace detail {
static constexpr size_t kInvalidIndex = static_cast<size_t> (-1);

constexpr size_t find_char_impl (const char *src, size_t src_len, char pattern, size_t start) noexcept {
  if (start >= src_len) {
    return kInvalidIndex;
  }
  auto ptr = static_cast<const char *> (memchr (src + start, static_cast<uint8_t> (pattern), src_len - start));
  return ptr ? static_cast<size_t> (ptr - src) : kInvalidIndex;
}

constexpr size_t find_str_impl (const char *src, size_t src_len, const char *pat, size_t pat_len, size_t start) noexcept {
  if (pat_len > src_len || start > src_len) {
    return kInvalidIndex;
  }

  if (!pat_len) {
    return start;
  }

  if (pat_len == 1) {
    return find_char_impl (src, src_len, pat[0], start);
  }

  const auto s = reinterpret_cast<const uint8_t *> (src);
  const auto p = reinterpret_cast<const uint8_t *> (pat);

  const auto first = p[0];
  const auto last = p[pat_len - 1];
  const auto remaining = pat_len - 2;

  auto i = start;
  const auto max_start = src_len - pat_len;

  while (i <= max_start) {
    const void *ptr = memchr (s + i, first, max_start - i + 1);

    if (!ptr) {
      return kInvalidIndex;
    }
    i = static_cast<size_t> (static_cast<const uint8_t *> (ptr) - s);

    if (s[i + pat_len - 1] != last) {
      ++i;
      continue;
    }
    if (remaining > 0 && memcmp (s + i + 1, p + 1, remaining) != 0) {
      ++i;
      continue;
    }
    return i;
  }
  return kInvalidIndex;
}

constexpr size_t rfind_char_impl (const char *src, size_t src_len, char pattern) noexcept {
  for (size_t i = src_len; i-- > 0;) {
    if (src[i] == pattern) {
      return i;
    }
  }
  return kInvalidIndex;
}

constexpr size_t rfind_str_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  if (!pat_len) {
    return src_len;
  }

  if (pat_len > src_len) {
    return kInvalidIndex;
  }

  const auto s = reinterpret_cast<const uint8_t *> (src);
  const auto p = reinterpret_cast<const uint8_t *> (pat);
  const auto first = p[0];

  if (pat_len == 1) {
    return rfind_char_impl (src, src_len, static_cast<char> (first));
  }

  const auto last = p[pat_len - 1];
  const auto remaining = pat_len - 2;

  auto i = src_len - pat_len;

  while (true) {
    while (i != kInvalidIndex && s[i] != first) {
      --i;
    }

    if (i == kInvalidIndex) {
      break;
    }

    if (s[i + pat_len - 1] == last && (remaining == 0 || memcmp (s + i + 1, p + 1, remaining) == 0)) {
      return i;
    }

    if (i == 0) {
      break;
    }
    --i;
  }
  return kInvalidIndex;
}

constexpr size_t find_first_of_impl (const char *src, size_t src_len, const char *pat, size_t pat_len, size_t start) noexcept {
  for (size_t i = start; i < src_len; ++i) {
    for (size_t j = 0; j < pat_len; ++j) {
      if (src[i] == pat[j]) {
        return i;
      }
    }
  }
  return kInvalidIndex;
}

constexpr size_t find_last_of_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  for (size_t i = src_len; i-- > 0;) {
    for (size_t j = 0; j < pat_len; ++j) {
      if (src[i] == pat[j]) {
        return i;
      }
    }
  }
  return kInvalidIndex;
}

constexpr size_t find_first_not_of_impl (const char *src, size_t src_len, const char *pat, size_t pat_len, size_t start) noexcept {
  for (size_t i = start; i < src_len; ++i) {
    bool different = true;

    for (size_t j = 0; j < pat_len; ++j) {
      if (src[i] == pat[j]) {
        different = false;
        break;
      }
    }

    if (different) {
      return i;
    }
  }
  return kInvalidIndex;
}

constexpr size_t find_last_not_of_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  for (size_t i = src_len; i-- > 0;) {
    bool different = true;

    for (size_t j = 0; j < pat_len; ++j) {
      if (src[i] == pat[j]) {
        different = false;
        break;
      }
    }
    if (different) {
      return i;
    }
  }
  return kInvalidIndex;
}

constexpr bool starts_with_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  return pat_len <= src_len && memcmp (src, pat, pat_len) == 0;
}

constexpr bool ends_with_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  return pat_len <= src_len && memcmp (src + src_len - pat_len, pat, pat_len) == 0;
}

constexpr size_t count_char_impl (const char *src, size_t src_len, char ch) noexcept {
  size_t count = 0;
  for (size_t i = 0; i < src_len; ++i) {
    if (src[i] == ch) {
      ++count;
    }
  }
  return count;
}

constexpr size_t count_str_impl (const char *src, size_t src_len, const char *pat, size_t pat_len) noexcept {
  if (pat_len > src_len) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i <= src_len - pat_len; ++i) {
    if (memcmp (src + i, pat, pat_len) == 0) {
      ++count;
    }
  }
  return count;
}

// one fnv1a32 step per char for incremental hashing of any span
constexpr uint32_t fnv1a32_step (const uint32_t hash, const char ch) noexcept {
  constexpr uint32_t prime = 0x1000193;

  return (hash ^ static_cast<uint32_t> (static_cast<uint8_t> (ch))) * prime;
}

constexpr uint32_t fnv1a32_n (const char *str, size_t len) noexcept {
  constexpr uint32_t basis = 0x811c9dc5;

  auto hash = basis;

  for (size_t i = 0; i < len; ++i) {
    hash = fnv1a32_step (hash, str[i]);
  }
  return hash;
}

constexpr char to_lower_ascii (const char ch) noexcept {
  return (ch >= 'A' && ch <= 'Z') ? static_cast<char> (ch + ('a' - 'A')) : ch;
}

// length-bounded atoi with defined overflow behavior (saturates to int range)
inline int parse_int_n (const char *str, size_t len) noexcept {
  size_t i = 0;

  while (i < len && ::isspace (static_cast<uint8_t> (str[i]))) {
    ++i;
  }
  bool negative = false;

  if (i < len && (str[i] == '+' || str[i] == '-')) {
    negative = (str[i] == '-');
    ++i;
  }
  // atoi overflow is ub, strtol saturates: accumulate in unsigned and clamp
  constexpr unsigned kMax = 0x7fffffffu;
  const unsigned limit = negative ? kMax + 1u : kMax;

  unsigned acc = 0;
  bool any = false;

  for (; i < len; ++i) {
    const char c = str[i];

    if (c < '0' || c > '9') {
      break;
    }
    any = true;

    const auto digit = static_cast<unsigned> (c - '0');

    if (acc > (limit - digit) / 10u) {
      // skip leftover digits, then saturate instead of wrapping
      for (++i; i < len && str[i] >= '0' && str[i] <= '9'; ++i) {
      }
      return negative ? static_cast<int> (0x80000000) : static_cast<int> (kMax);
    }
    acc = acc * 10u + digit;
  }

  if (!any) {
    return 0;
  }

  if (negative) {
    if (acc == limit) {
      return static_cast<int> (0x80000000);
    }
    return -static_cast<int> (acc);
  }
  return static_cast<int> (acc);
}

// integer power of ten by squaring without the libm pow call
inline double pow10_int_n (int exp) noexcept {
  if (exp == 0) {
    return 1.0;
  }
  const bool negative = exp < 0;
  auto n = negative ? static_cast<unsigned> (-(exp + 1)) + 1u : static_cast<unsigned> (exp);

  double base = 10.0, result = 1.0;

  while (n != 0u) {
    if (n & 1u) {
      result *= base;
    }
    base *= base;
    n >>= 1;
  }
  return negative ? 1.0 / result : result;
}

// length-bounded strtof without allocation and without reading past len
inline float parse_float_n (const char *str, size_t len) noexcept {
  size_t i = 0;

  while (i < len && ::isspace (static_cast<uint8_t> (str[i]))) {
    ++i;
  }

  if (i >= len) {
    return 0.0f;
  }
  bool negative = false;

  if (str[i] == '+' || str[i] == '-') {
    negative = (str[i] == '-');
    ++i;
  }

  // inf / infinity / nan, case-insensitive like strtod ("infxyz" -> inf)
  if (i < len && (to_lower_ascii (str[i]) == 'i' || to_lower_ascii (str[i]) == 'n')) {
    const auto c0 = to_lower_ascii (str[i]);

    if (c0 == 'i' && i + 3 <= len && to_lower_ascii (str[i + 1]) == 'n' && to_lower_ascii (str[i + 2]) == 'f') {
      return negative ? -INFINITY : INFINITY; // optional "inity" tail is ignored, like strtod
    }

    if (c0 == 'n' && i + 3 <= len && to_lower_ascii (str[i + 1]) == 'a' && to_lower_ascii (str[i + 2]) == 'n') {
      const float nan = NAN;
      return negative ? -nan : nan; // optional "(...)" payload is ignored, trailing chars don't matter
    }
  }

  // keep only leading digits in the accumulator and fold rest into exp
  uint64_t sig = 0;
  int sig_digits = 0;
  int exp10 = 0;
  bool any = false;

  while (i < len && str[i] >= '0' && str[i] <= '9') {
    any = true;

    const auto digit = static_cast<uint64_t> (str[i] - '0');

    if (sig == 0 && digit == 0) {
      // skip leading integer zeros without wasting precision slots
    }
    else if (sig_digits < 18) {
      sig = sig * 10u + digit;
      ++sig_digits;
    }
    else {
      ++exp10; // extra integer digits shift the value up
    }
    ++i;
  }

  if (i < len && str[i] == '.') {
    ++i;

    while (i < len && str[i] >= '0' && str[i] <= '9') {
      any = true;

      const auto digit = static_cast<uint64_t> (str[i] - '0');

      if (sig == 0 && digit == 0 && sig_digits == 0) {
        --exp10; // leading fraction zeros ("0.00x") shift the value down
      }
      else if (sig_digits < 18) {
        sig = sig * 10u + digit;
        ++sig_digits;
        --exp10;
      }
      // further fraction digits are below float precision, drop them
      ++i;
    }
  }

  if (!any) {
    return 0.0f;
  }
  int exp_part = 0;

  if (i < len && (str[i] == 'e' || str[i] == 'E')) {
    size_t j = i + 1;
    bool exp_negative = false;

    if (j < len && (str[j] == '+' || str[j] == '-')) {
      exp_negative = (str[j] == '-');
      ++j;
    }
    int exp = 0;
    bool exp_any = false;

    while (j < len && str[j] >= '0' && str[j] <= '9') {
      exp_any = true;

      // clamp: exponents beyond +-1e9 are inf/zero regardless of mantissa
      if (exp < 100000000) {
        exp = exp * 10 + (str[j] - '0');
      }
      ++j;
    }

    // a bare e is no exponent, so advance only when digits follow
    if (exp_any) {
      i = j;
      exp_part = exp_negative ? -exp : exp;
    }
  }

  if (sig == 0) {
    return negative ? -0.0f : 0.0f; // preserve signed zero ("-0.0")
  }
  const auto final_exp = static_cast<long long> (exp10) + exp_part;

  // clamp the exponent for pow10, the rest is inf or zero as float
  if (final_exp > 310) {
    return negative ? -INFINITY : INFINITY;
  }

  if (final_exp < -320) {
    return negative ? -0.0f : 0.0f;
  }
  const double value = static_cast<double> (sig) * pow10_int_n (static_cast<int> (final_exp));
  const auto result = static_cast<float> (value);

  return negative ? -result : result;
}
}

// simple non-owning string class like std::string_view
class StringRef {
private:
  // byte loop for constant evaluation, strlen stays on the runtime path
  static constexpr size_t lenstr_loop (const char *str) noexcept {
    auto count = static_cast<size_t> (0);

    for (; *str != kNullChar; ++str) {
      ++count;
    }
    return count;
  }

  // runtime length: the crt strlen is ~2.5x faster than the loop above
  static constexpr size_t lenstr (const char *str) noexcept {
    if (ystl::is_constant_evaluated ()) {
      return lenstr_loop (str);
    }
    return strlen (str);
  }

public:
  static constexpr uint32_t fnv1a32 (const char *str) noexcept {
    constexpr uint32_t basis = 0x811c9dc5;

    auto hash = basis;

    for (; *str != kNullChar; ++str) {
      hash = detail::fnv1a32_step (hash, *str);
    }
    return hash;
  }

public:
  enum : size_t {
    InvalidIndex = static_cast<size_t> (-1)
  };

private:
  const char *chars_ = "";
  size_t size_ {};

public:
  constexpr StringRef () noexcept = default;

  constexpr StringRef (const char *chars) : chars_ (chars ? chars : ""), size_ (chars ? StringRef::lenstr (chars) : 0) {}

  constexpr StringRef (const char *chars, size_t length) : chars_ (chars ? chars : ""), size_ (chars ? length : 0) {}

  constexpr StringRef (nullptr_t) : chars_ (""), size_ (0) {}

  constexpr StringRef (nullptr_t, size_t) : chars_ (""), size_ (0) {}

public:
  StringRef (const String &str);

public:
  constexpr StringRef (const StringRef &) = default;
  constexpr StringRef &operator= (const StringRef &) = default;

  constexpr StringRef (StringRef &&) = default;
  constexpr StringRef &operator= (StringRef &&) = default;

public:
  constexpr bool operator== (const StringRef &rhs) const {
    return size_ == rhs.size_ && memcmp (chars_, rhs.chars_, size_) == 0;
  }

  // lexicographic ordering, mirrors std::string_view::operator<=>
  std::strong_ordering operator<=> (const StringRef &rhs) const {
    const size_t common = ystl::min (size_, rhs.size_);
    const int result = memcmp (chars_, rhs.chars_, common);

    if (result != 0) {
      return result <=> 0;
    }
    return size_ <=> rhs.size_;
  }

  bool operator== (const char *rhs) const {
    // contract: rhs must be null-terminated, a raw view passed as char* overreads
    if (!rhs) {
      return size_ == 0;
    }
    return strlen (rhs) == size_ && memcmp (chars_, rhs, size_) == 0;
  }

  constexpr bool operator!= (const StringRef &rhs) const {
    return !(*this == rhs);
  }

  bool operator!= (const char *rhs) const {
    return !(*this == rhs);
  }

  constexpr char operator[] (size_t index) const {
    return chars_[index];
  }

public:
  YSTL_FORCE_INLINE constexpr bool empty () const noexcept {
    return size_ == 0;
  }

  YSTL_FORCE_INLINE constexpr size_t size () const noexcept {
    return size_;
  }

  YSTL_FORCE_INLINE constexpr const char *chars () const noexcept {
    return chars_;
  }

  constexpr bool equals (const StringRef &rhs) const {
    return *this == rhs;
  }

  constexpr uint32_t hash () const {
    return detail::fnv1a32_n (chars_, size_);
  }

public:
  // note: not constexpr, as bounded parsing suits non-terminated views
  template <typename U> U as () const {
    if constexpr (ystl::is_same_v<U, float>) {
      return detail::parse_float_n (chars_, size_);
    }
    else if constexpr (ystl::is_same_v<U, int>) {
      return detail::parse_int_n (chars_, size_);
    }
    else {
      static_assert (ystl::is_same_v<U, float> || ystl::is_same_v<U, int>, "as<U>() only supports float and int");
    }
  }

  bool starts_with (StringRef prefix) const {
    return detail::starts_with_impl (chars_, size_, prefix.chars (), prefix.size ());
  }

  bool ends_with (StringRef suffix) const {
    return detail::ends_with_impl (chars_, size_, suffix.chars (), suffix.size ());
  }

  bool contains (StringRef rhs) const {
    return find (rhs) != InvalidIndex;
  }

  [[nodiscard]] bool equals_no_case (StringRef rhs) const {
    if (size_ != rhs.size_) {
      return false;
    }

    for (size_t i = 0; i < size_; ++i) {
      auto lhs = chars_[i];
      auto rhs_ch = rhs.chars_[i];

      if (lhs >= 'A' && lhs <= 'Z') {
        lhs += 'a' - 'A';
      }
      if (rhs_ch >= 'A' && rhs_ch <= 'Z') {
        rhs_ch += 'a' - 'A';
      }
      if (lhs != rhs_ch) {
        return false;
      }
    }
    return true;
  }

  size_t find (char pattern, size_t start = 0) const {
    return detail::find_char_impl (chars_, size_, pattern, start);
  }

  size_t find (StringRef pattern, size_t start = 0) const noexcept {
    return detail::find_str_impl (chars_, size_, pattern.chars (), pattern.size (), start);
  }

  constexpr size_t rfind (char pattern) const {
    return detail::rfind_char_impl (chars_, size_, pattern);
  }

  constexpr size_t rfind (StringRef pattern) const {
    return detail::rfind_str_impl (chars_, size_, pattern.chars (), pattern.size ());
  }

  constexpr size_t find_first_of (StringRef pattern, size_t start = 0) const {
    return detail::find_first_of_impl (chars_, size_, pattern.chars (), pattern.size (), start);
  }

  constexpr size_t find_last_of (StringRef pattern) const {
    return detail::find_last_of_impl (chars_, size_, pattern.chars (), pattern.size ());
  }

  constexpr size_t find_first_not_of (StringRef pattern, size_t start = 0) const {
    return detail::find_first_not_of_impl (chars_, size_, pattern.chars (), pattern.size (), start);
  }

  constexpr size_t find_last_not_of (StringRef pattern) const {
    return detail::find_last_not_of_impl (chars_, size_, pattern.chars (), pattern.size ());
  }

  constexpr size_t count_char (char ch) const {
    return detail::count_char_impl (chars_, size_, ch);
  }

  constexpr size_t count_str (StringRef pattern) const {
    return detail::count_str_impl (chars_, size_, pattern.chars (), pattern.size ());
  }

  constexpr StringRef substr (size_t start, size_t count = InvalidIndex) const {
    start = ystl::min (start, size_);

    if (count == InvalidIndex) {
      count = size_;
    }
    return { chars () + start, ystl::min (count, size () - start) };
  }

  template <typename U = StringRef> constexpr Array<U> split (StringRef delim) const {
    Array<U> tokens;

    // empty delimiter would match at every position without advancing -> guard
    if (delim.empty ()) {
      tokens.push (substr (0));
      return tokens;
    }
    size_t prev = 0, pos = 0;

    while ((pos = find (delim, pos)) != InvalidIndex) {
      tokens.push (substr (prev, pos - prev));
      pos += delim.size ();
      prev = pos;
    }
    tokens.push (substr (prev));

    return tokens;
  }

  template <typename U = StringRef> constexpr Array<U> split (size_t max_length) const {
    Array<U> tokens;

    // zero chunk size would never advance the cursor -> guard
    if (max_length == 0) {
      return tokens;
    }

    for (size_t i = 0; i < size (); i += max_length) {
      tokens.emplace (substr (i, max_length));
    }
    return tokens;
  }

public:
  constexpr const char *begin () const {
    return chars_;
  }

  constexpr const char *end () const {
    return chars_ + size_;
  }
};

// simple std::string analogue with small string optimization (sso)
class String final {
public:
  enum : size_t {
    InvalidIndex = StringRef::InvalidIndex
  };

private:
  // sso layout: the tag byte overlaps with the msbyte of heap_.capacity

  static constexpr size_t SSOBuf = sizeof (size_t) * 3;

#if defined(YSTL_ARCH_CPU_BIG_ENDIAN)
  static constexpr size_t SSOTag = sizeof (char *) + sizeof (size_t);
#else
  static constexpr size_t SSOTag = SSOBuf - 1;
#endif
  static constexpr size_t SSOCap = SSOTag - 1;

  struct StringHeap {
    char *data {};
    size_t length {};
    size_t capacity {};
  };

  union {
    StringHeap heap_;
    char sso_[SSOBuf] {};
  };

private:
  YSTL_FORCE_INLINE bool is_sso () const noexcept {
    return static_cast<uint8_t> (sso_[SSOTag]) & 0x80;
  }

  YSTL_FORCE_INLINE size_t sso_len () const noexcept {
    return static_cast<uint8_t> (sso_[SSOTag]) & 0x7F;
  }

  YSTL_FORCE_INLINE void init_sso (size_t len = 0) noexcept {
    YSTL_SATISFY_GCC_ANALYZER (len); // prevent false-positive -wstringop-overflow from sso analysis
    sso_[len] = kNullChar;
    sso_[SSOTag] = static_cast<char> (0x80 | len);
  }

  YSTL_FORCE_INLINE char *mutable_data () noexcept {
    auto ret = is_sso () ? sso_ : heap_.data;
    YSTL_SATISFY_GCC_ANALYZER (ret); // prevent false-positive -wstringop-overflow from sso analysis
    return ret;
  }

  void release_storage () noexcept {
    if (!is_sso ()) [[unlikely]] {
      mem::release (heap_.data);
    }
  }

  [[nodiscard]] static size_t checked_add (size_t a, size_t b) noexcept {
    size_t out = 0;

    if (add_overflow (a, b, out)) [[unlikely]] {
      plat.abort ("String size computation overflow");
    }
    return out;
  }

  [[nodiscard]] static size_t checked_sub (size_t a, size_t b) noexcept {
    if (b > a) [[unlikely]] {
      plat.abort ("String size computation overflow");
    }
    return a - b;
  }

  [[nodiscard]] static size_t checked_mul (size_t a, size_t b) noexcept {
    size_t out = 0;

    if (mul_overflow (a, b, out)) [[unlikely]] {
      plat.abort ("String size computation overflow");
    }
    return out;
  }

  static size_t grow_capacity (size_t current, size_t needed) noexcept {
    auto capacity = current ? current : ystl::max<size_t> (16u, needed);

    while (capacity < needed) {
      if (capacity > (numeric_limits<size_t>::max () / 2)) [[unlikely]] {
        return needed;
      }
      capacity *= 2;
    }
    return capacity;
  }

  // ensure buffer has room for `needed` total bytes
  char *ensure_capacity (size_t needed, size_t preserve) noexcept {
    if (is_sso ()) [[likely]] {
      if (needed <= SSOTag) [[likely]] {
        return sso_;
      }

      // promote sso -> heap: save inline data before overwriting union
      char tmp[SSOTag];

      if (preserve > 0) {
        memcpy (tmp, sso_, preserve);
      }
      const auto cap = grow_capacity (0, needed);
      auto buf = mem::allocate<char> (cap);

      if (preserve > 0) {
        memcpy (buf, tmp, preserve);
      }
      heap_.data = buf;
      heap_.length = preserve;
      heap_.capacity = cap;

      return buf;
    }

    if (needed <= heap_.capacity) {
      return heap_.data;
    }
    const auto cap = grow_capacity (heap_.capacity, needed);

    // realloc is safe for char and keeps the preserved prefix intact
    auto buf = mem::reallocate (heap_.data, cap);

    heap_.data = buf;
    heap_.capacity = cap;

    return buf;
  }

  // update logical length and null-terminate in current mode
  YSTL_FORCE_INLINE void update_length (size_t len) noexcept {
    if (is_sso ()) [[likely]] {
      init_sso (len);
    }
    else {
      heap_.length = len;
      heap_.data[len] = kNullChar;
    }
  }

  // range check without pointer-comparison ub, mirrors array
  static bool range_contains (const char *cur, const char *end, const char *p) noexcept {
    const auto addr = reinterpret_cast<uintptr_t> (static_cast<const void *> (p));
    const auto base = reinterpret_cast<uintptr_t> (static_cast<const void *> (cur));
    const auto stop = reinterpret_cast<uintptr_t> (static_cast<const void *> (end));

    return addr >= base && addr < stop;
  }

  // true if p points into our current storage; mirrors the range checks in assign/append
  bool points_inside (const char *p) const noexcept {
    if (!p) {
      return false;
    }
    const auto cur = is_sso () ? sso_ : heap_.data;

    if (!cur) {
      return false;
    }
    return range_contains (cur, cur + (is_sso () ? SSOBuf : heap_.capacity), p);
  }

  // per argument alias test, other types can never alias our storage
  template <typename T> bool is_format_alias (T &&arg) const noexcept {
    using Decayed = ystl::remove_cvref_t<T>;

    if constexpr (ystl::is_same_v<Decayed, String>) {
      const String &s = arg;

      if (&s == this) {
        return true;
      }
      return points_inside (s.chars ());
    }
    else if constexpr (ystl::is_same_v<Decayed, StringRef>) {
      const StringRef &r = arg;
      return points_inside (r.chars ());
    }
    else if constexpr (ystl::is_same_v<Decayed, char *> || ystl::is_same_v<Decayed, const char *>) {
      return points_inside (arg);
    }
    else if constexpr (ystl::is_array_v<Decayed> && ystl::is_same_v<ystl::remove_cv_t<typename ystl::clear_extent<Decayed>::type>, char>) {
      return points_inside (arg);
    }
    else {
      return false;
    }
  }

public:
  String () noexcept {
    init_sso ();
  }

  ~String () {
    release_storage ();
  }

  // fresh init needs no alias check, the source cannot overlap storage
  YSTL_FORCE_INLINE void init_from (const char *str, size_t len) noexcept {
    // matches assign: null source or zero length means empty string
    if (!str || !len) [[unlikely]] {
      return;
    }

    if (len <= SSOCap) [[likely]] {
      memcpy (sso_, str, len);
      init_sso (len);
    }
    else {
      const auto cap = grow_capacity (0, len + 1);
      auto buf = mem::allocate<char> (cap);

      memcpy (buf, str, len);
      buf[len] = kNullChar;

      heap_.data = buf;
      heap_.length = len;
      heap_.capacity = cap;
    }
  }

  String (const char *str) {
    init_sso ();
    init_from (str, str ? strlen (str) : 0);
  }

  String (const char *str, size_t length) {
    init_sso ();
    init_from (str, length);
  }

  String (const String &rhs) {
    if (rhs.is_sso ()) [[likely]] {
      memcpy (sso_, rhs.sso_, SSOBuf);
    }
    else {
      const auto len = rhs.heap_.length;
      const auto cap = ystl::max<size_t> (16u, len + 1);

      heap_.data = mem::allocate<char> (cap);
      memcpy (heap_.data, rhs.heap_.data, len + 1);

      heap_.length = len;
      heap_.capacity = cap;
    }
  }

  String (StringRef str) {
    init_sso ();
    assign (str.chars (), str.size ());
  }

  explicit String (const char ch) noexcept {
    sso_[0] = ch;
    init_sso (1);
  }

  String (String &&rhs) noexcept {
    if (rhs.is_sso ()) {
      memcpy (sso_, rhs.sso_, SSOBuf);
      rhs.init_sso (0);
    }
    else {
      heap_.data = rhs.heap_.data;
      heap_.length = rhs.heap_.length;
      heap_.capacity = rhs.heap_.capacity;

      rhs.init_sso (0);
    }
  }

public:
  void reserve (const size_t amount) noexcept {
    const auto len = size ();
    auto buf = ensure_capacity (checked_add (checked_add (len, amount), 1), len);
    buf[len] = kNullChar;
  }

  String &assign (const char *str) {
    return assign (str, str ? strlen (str) : 0);
  }

  String &assign (const char *str, size_t len) {
    if (!len || !str || len >= InvalidIndex) [[unlikely]] {
      if (is_sso ()) [[likely]] {
        init_sso (0);
      }
      else {
        heap_.length = 0;
        heap_.data[0] = kNullChar;
      }
      return *this;
    }

    // self-reference detection: source points inside our buffer
    const auto cur = is_sso () ? sso_ : heap_.data;
    const auto cur_end = cur + (is_sso () ? SSOBuf : heap_.capacity);

    if (range_contains (cur, cur_end, str)) [[unlikely]] {
      auto tmp = mem::allocate<char> (len + 1);
      memcpy (tmp, str, len);
      tmp[len] = kNullChar;

      if (len <= SSOCap) [[likely]] {
        // sso overlaps heap, so release the heap buffer before overwriting
        release_storage ();
        memcpy (sso_, tmp, len);
        init_sso (len);
        mem::release (tmp);
      }
      else if (is_sso ()) [[likely]] {
        // inline buffer can't hold the result, adopt the snapshot as heap storage
        heap_.data = tmp;
        heap_.length = len;
        heap_.capacity = len + 1;
        return *this;
      }
      else {
        // keep the heap buffer alive, only swap the contents in place
        memcpy (heap_.data, tmp, len);
        heap_.data[len] = kNullChar;
        heap_.length = len;
        mem::release (tmp);
      }
      return *this;
    }

    // use sso only when no heap buffer needs preserving, else reuse heap
    if (len <= SSOCap && is_sso ()) [[likely]] {
      memcpy (sso_, str, len);
      init_sso (len);
    }
    else {
      // large string (or reused heap buffer): use heap, reuse buffer when possible
      auto buf = ensure_capacity (len + 1, 0);
      YSTL_SATISFY_GCC_ANALYZER (str); // prevent false-positive -wstringop-overread
      memcpy (buf, str, len);

      buf[len] = kNullChar;
      heap_.length = len;

      YSTL_SATISFY_GCC_ANALYZER (heap_.length); // prevent false-positive -wstringop-overflow from sso analysis
    }
    return *this;
  }

  String &assign (const String &str, size_t len = 0) {
    // len zero means whole string, other lengths are clamped to fit
    const auto srcLen = str.size ();

    if (&str == this) {
      if (len == 0 || len >= srcLen) {
        return *this;
      }
      // self truncation falls through to the self reference path below
      return assign (str.chars (), len);
    }
    return assign (str.chars (), (len == 0 || len > srcLen) ? srcLen : len);
  }

  // overload avoids a heap allocating temporary from stringref
  String &assign (StringRef str) {
    return assign (str.chars (), str.size ());
  }

  String &assign (const char ch) noexcept {
    if (is_sso ()) [[likely]] {
      sso_[0] = ch;
      init_sso (1);
    }
    else {
      heap_.data[0] = ch;
      heap_.data[1] = kNullChar;
      heap_.length = 1;
    }
    return *this;
  }

  String &append (const char *str) {
    return append (str, str ? strlen (str) : 0);
  }

  String &append (const char *str, size_t len) {
    if (!len) [[unlikely]] {
      return *this;
    }
    const auto cur_len = size ();

    // self-reference detection
    const auto cur = is_sso () ? sso_ : heap_.data;
    const auto cur_end = cur + (is_sso () ? SSOBuf : heap_.capacity);

    if (range_contains (cur, cur_end, str)) [[unlikely]] {
      auto tmp = mem::allocate<char> (len);
      memcpy (tmp, str, len);

      auto buf = ensure_capacity (checked_add (checked_add (cur_len, len), 1), cur_len);
      memcpy (buf + cur_len, tmp, len);
      mem::release (tmp);
    }
    else {
      auto buf = ensure_capacity (checked_add (checked_add (cur_len, len), 1), cur_len);
      memcpy (buf + cur_len, str, len);
    }
    update_length (cur_len + len);
    return *this;
  }

  String &append (const String &str, size_t len = 0) {
    // clamp lengths to str length, with zero meaning the whole string
    const auto srcLen = str.size ();
    return append (str.chars (), (len == 0 || len > srcLen) ? srcLen : len);
  }

  // overload avoids a heap allocating temporary for append from stringref
  String &append (StringRef str) {
    return append (str.chars (), str.size ());
  }

  String &append (const char ch) noexcept {
    const auto len = size ();

    // fast path: appending single char to sso string that still fits
    if (is_sso () && len + 1 <= SSOCap) [[likely]] {
      sso_[len] = ch;
      init_sso (len + 1);
      return *this;
    }
    auto buf = ensure_capacity (len + 2, len);
    buf[len] = ch;

    update_length (len + 1);
    return *this;
  }

  template <typename... Args> String &assignf (const char *fmt, Args &&...args) {
    const auto result = fmtwrap ().exec (nullptr, 0, fmt, args...);

    if (result < 0) [[unlikely]] {
      clear ();
      return *this;
    }
    const auto size = static_cast<size_t> (result);

    // guard self references, as formatting may reallocate our buffer
    const bool aliased = points_inside (fmt) || (is_format_alias (args) || ...);

    if (!aliased) [[likely]] {
      if (size <= SSOCap) [[likely]] {
        release_storage ();
        fmtwrap ().exec (sso_, size + 1, fmt, ystl::forward<Args> (args)...);
        init_sso (size);
      }
      else {
        auto buf = ensure_capacity (size + 1, 0);
        fmtwrap ().exec (buf, size + 1, fmt, ystl::forward<Args> (args)...);
        heap_.length = size;
      }
      return *this;
    }
    // alias-safe path: format into a fresh buffer while the old storage is still alive, then install it
    auto tmp = mem::allocate<char> (size + 1);
    fmtwrap ().exec (tmp, size + 1, fmt, ystl::forward<Args> (args)...);

    release_storage ();

    if (size <= SSOCap) {
      memcpy (sso_, tmp, size);
      init_sso (size);
      mem::release (tmp);
    }
    else {
      heap_.data = tmp;
      heap_.length = size;
      heap_.capacity = size + 1;
    }
    return *this;
  }

  template <typename... Args> String &appendf (const char *fmt, Args &&...args) {
    const auto result = fmtwrap ().exec (nullptr, 0, fmt, args...);

    if (result < 0) [[unlikely]] {
      return *this;
    }
    const auto formatted_size = static_cast<size_t> (result);
    const auto cur_len = size ();
    const auto total = checked_add (cur_len, formatted_size);
    const auto needed = checked_add (total, 1);

    // guard self references, the dst prefix shares the source terminator
    const bool aliased = points_inside (fmt) || (is_format_alias (args) || ...);

    if (!aliased) [[likely]] {
      auto buf = ensure_capacity (needed, cur_len);
      fmtwrap ().exec (buf + cur_len, formatted_size + 1, fmt, ystl::forward<Args> (args)...);

      update_length (total);
      return *this;
    }
    // format the tail aside first, then append it once buffer is stable
    auto tail = mem::allocate<char> (formatted_size + 1);
    fmtwrap ().exec (tail, formatted_size + 1, fmt, ystl::forward<Args> (args)...);

    auto buf = ensure_capacity (needed, cur_len);
    memcpy (buf + cur_len, tail, formatted_size + 1);
    mem::release (tail);

    update_length (cur_len + formatted_size);
    return *this;
  }

public:
  YSTL_FORCE_INLINE const char &at (size_t index) const {
    return chars ()[index];
  }

  YSTL_FORCE_INLINE char &at (size_t index) {
    return mutable_data ()[index];
  }

  YSTL_FORCE_INLINE const char *chars () const noexcept {
    return is_sso () ? sso_ : heap_.data;
  }

  YSTL_FORCE_INLINE size_t size () const noexcept {
    return is_sso () ? sso_len () : heap_.length;
  }

  YSTL_FORCE_INLINE size_t capacity () const noexcept {
    return is_sso () ? SSOCap : heap_.capacity;
  }

  YSTL_FORCE_INLINE bool empty () const noexcept {
    return size () == 0;
  }

  void clear () noexcept {
    if (is_sso ()) [[likely]] {
      init_sso (0);
    }
    else {
      heap_.length = 0;
      heap_.data[0] = kNullChar;
    }
  }

  StringRef str () const {
    return { chars (), size () };
  }

public:
  bool insert (size_t index, StringRef str) {
    if (str.empty ()) {
      return true;
    }
    const auto str_len = str.size ();

    if (index >= size ()) {
      append (str.chars (), str_len);
    }
    else {
      const auto cur_len = size ();
      const auto total = checked_add (cur_len, str_len);

      // self-reference detection: source may point inside our buffer
      const auto cur = is_sso () ? sso_ : heap_.data;
      const auto cur_end = cur + (is_sso () ? SSOBuf : heap_.capacity);
      const auto src = str.chars ();

      if (range_contains (cur, cur_end, src)) [[unlikely]] {
        auto tmp = mem::allocate<char> (str_len);
        memcpy (tmp, src, str_len);

        auto buf = ensure_capacity (checked_add (total, 1), cur_len);
        memmove (buf + index + str_len, buf + index, cur_len - index);
        memcpy (buf + index, tmp, str_len);
        mem::release (tmp);
      }
      else {
        auto buf = ensure_capacity (checked_add (total, 1), cur_len);
        memmove (buf + index + str_len, buf + index, cur_len - index);
        memcpy (buf + index, str.chars (), str_len);
      }
      update_length (total);
    }
    return true;
  }

  bool erase (size_t index, size_t count = 1) {
    const auto len = size ();

    // order matters: index + count may wrap around (e.g
    if (index > len || count > len - index) {
      return false;
    }
    auto buf = mutable_data ();
    const size_t new_len = len - count;
    const size_t move_size = new_len - index;

    if (move_size > 0) {
      memmove (buf + index, buf + index + count, move_size);
    }
    update_length (new_len);
    return true;
  }

  size_t replace (StringRef needle, StringRef to) {
    if (needle.empty ()) {
      return 0;
    }

    // snapshot the search parameters to avoid self-reference issues
    const auto needle_len = needle.size ();
    const auto to_len = to.size ();
    const auto cur_len = size ();

    // first pass: count occurrences
    size_t replaced = 0;
    size_t pos = 0;

    while (pos < cur_len) {
      pos = detail::find_str_impl (chars (), cur_len, needle.chars (), needle_len, pos);

      if (pos == InvalidIndex) {
        break;
      }
      ++replaced;
      pos += needle_len;
    }

    if (!replaced) {
      return 0;
    }

    // build result in a new buffer
    const auto removed = checked_mul (replaced, needle_len);
    const auto added = checked_mul (replaced, to_len);
    const auto new_len = checked_add (checked_sub (cur_len, removed), added);
    auto result = mem::allocate<char> (checked_add (new_len, 1));

    // snapshot needle/replacement since they may point into our buffer
    auto needle_buf = mem::allocate<char> (needle_len);
    memcpy (needle_buf, needle.chars (), needle_len);

    char *to_buf = nullptr;

    if (to_len > 0) {
      to_buf = mem::allocate<char> (to_len);
      memcpy (to_buf, to.chars (), to_len);
    }
    const auto src = chars ();
    size_t srcPos = 0, dst_pos = 0;

    while (srcPos < cur_len) {
      auto found = detail::find_str_impl (src, cur_len, needle_buf, needle_len, srcPos);

      if (found == InvalidIndex) {
        memcpy (result + dst_pos, src + srcPos, cur_len - srcPos);
        dst_pos += cur_len - srcPos;
        break;
      }

      // copy segment before match
      if (found > srcPos) {
        memcpy (result + dst_pos, src + srcPos, found - srcPos);
        dst_pos += found - srcPos;
      }

      // copy replacement
      if (to_len > 0) {
        memcpy (result + dst_pos, to_buf, to_len);
        dst_pos += to_len;
      }
      srcPos = found + needle_len;
    }
    result[dst_pos] = kNullChar;

    mem::release (needle_buf);
    mem::release (to_buf);

    // assign result to self
    release_storage ();

    if (dst_pos <= SSOCap) {
      memcpy (sso_, result, dst_pos);
      init_sso (dst_pos);
      mem::release (result);
    }
    else {
      heap_.data = result;
      heap_.length = dst_pos;
      heap_.capacity = new_len + 1;
    }
    return replaced;
  }

  String &lowercase () {
    auto buf = mutable_data ();
    const auto len = size ();

    for (size_t i = 0; i < len; ++i) {
      buf[i] = static_cast<char> (::tolower (static_cast<uint8_t> (buf[i])));
    }
    return *this;
  }

  String &uppercase () {
    auto buf = mutable_data ();
    const auto len = size ();

    for (size_t i = 0; i < len; ++i) {
      buf[i] = static_cast<char> (::toupper (static_cast<uint8_t> (buf[i])));
    }
    return *this;
  }

  String &ltrim (StringRef characters = "\r\n\t ") {
    const auto len = size ();
    size_t begin = len;

    for (size_t i = 0; i < begin; ++i) {
      if (characters.find (at (i)) == InvalidIndex) {
        begin = i;
        break;
      }
    }

    if (begin > 0) {
      erase (0, begin); // drops the leading characters in place, without reallocating
    }
    return *this;
  }

  String &rtrim (StringRef characters = "\r\n\t ") {
    const auto len = size ();
    size_t end = 0;

    for (size_t i = len; i > 0; --i) {
      if (characters.find (at (i - 1)) == InvalidIndex) {
        end = i;
        break;
      }
    }

    if (end < len) {
      erase (end, len - end); // drops the trailing characters in place, without reallocating
    }
    return *this;
  }

  String &trim (StringRef characters = "\r\n\t ") {
    return ltrim (characters).rtrim (characters);
  }

public:
  uint32_t hash () const {
    return detail::fnv1a32_n (chars (), size ());
  }

  bool contains (StringRef rhs) const {
    return find (rhs) != InvalidIndex;
  }

  [[nodiscard]] bool equals_no_case (StringRef rhs) const {
    return StringRef (chars (), size ()).equals_no_case (rhs);
  }

  bool starts_with (StringRef prefix) const {
    return detail::starts_with_impl (chars (), size (), prefix.chars (), prefix.size ());
  }

  bool ends_with (StringRef suffix) const {
    return detail::ends_with_impl (chars (), size (), suffix.chars (), suffix.size ());
  }

  size_t find (char pattern, size_t start = 0) const {
    return detail::find_char_impl (chars (), size (), pattern, start);
  }

  // terminated storage allows the vectorized crt strstr here, slices with embedded
  // nuls must use the stringref overload below
  size_t find (const char *pattern, size_t start = 0) const {
    const auto len = size ();

    if (start > len) {
      return InvalidIndex;
    }
    if (!pattern || pattern[0] == kNullChar) {
      return start; // matches find_str_impl for empty patterns
    }
    const auto *found = strstr (chars () + start, pattern);

    return found ? static_cast<size_t> (found - chars ()) : InvalidIndex;
  }

  size_t find (StringRef pattern, size_t start = 0) const {
    return detail::find_str_impl (chars (), size (), pattern.chars (), pattern.size (), start);
  }

  size_t rfind (char pattern) const {
    return detail::rfind_char_impl (chars (), size (), pattern);
  }

  size_t rfind (StringRef pattern) const {
    return detail::rfind_str_impl (chars (), size (), pattern.chars (), pattern.size ());
  }

  size_t find_first_of (StringRef pattern, size_t start = 0) const {
    return detail::find_first_of_impl (chars (), size (), pattern.chars (), pattern.size (), start);
  }

  size_t find_last_of (StringRef pattern) const {
    return detail::find_last_of_impl (chars (), size (), pattern.chars (), pattern.size ());
  }

  size_t find_first_not_of (StringRef pattern, size_t start = 0) const {
    return detail::find_first_not_of_impl (chars (), size (), pattern.chars (), pattern.size (), start);
  }

  size_t find_last_not_of (StringRef pattern) const {
    return detail::find_last_not_of_impl (chars (), size (), pattern.chars (), pattern.size ());
  }

  size_t count_char (char ch) const {
    return detail::count_char_impl (chars (), size (), ch);
  }

  size_t count_str (StringRef pattern) const {
    return detail::count_str_impl (chars (), size (), pattern.chars (), pattern.size ());
  }

  String substr (size_t start, size_t count = InvalidIndex) const {
    const auto ref = StringRef (chars (), size ()).substr (start, count);
    return String (ref.chars (), ref.size ());
  }

  Array<String> split (StringRef delim) const {
    return StringRef (chars (), size ()).split<String> (delim);
  }

  Array<String> split (size_t max_length) const {
    return StringRef (chars (), size ()).split<String> (max_length);
  }

public:
  template <typename U> U as () const {
    if constexpr (ystl::is_same_v<U, float>) {
      return StringRef (chars (), size ()).as<float> ();
    }
    else if constexpr (ystl::is_same_v<U, int>) {
      return StringRef (chars (), size ()).as<int> ();
    }
    else {
      static_assert (ystl::is_same_v<U, float> || ystl::is_same_v<U, int>, "as<U>() only supports float and int");
    }
  }

public:
  char *begin () {
    return mutable_data ();
  }

  const char *begin () const {
    return chars ();
  }

  char *end () {
    return mutable_data () + size ();
  }

  const char *end () const {
    return chars () + size ();
  }

public:
  String &operator= (String &&rhs) noexcept {
    if (this != &rhs) [[likely]] {
      release_storage ();

      if (rhs.is_sso ()) {
        memcpy (sso_, rhs.sso_, SSOBuf);
      }
      else {
        heap_.data = rhs.heap_.data;
        heap_.length = rhs.heap_.length;
        heap_.capacity = rhs.heap_.capacity;
      }
      rhs.init_sso (0);
    }
    return *this;
  }

  String &operator= (const String &rhs) {
    return assign (rhs);
  }

  // overload to avoid the implicit temporary string conversion from stringref
  String &operator= (StringRef rhs) {
    return assign (rhs.chars (), rhs.size ());
  }

  String &operator= (const char *rhs) {
    return assign (rhs);
  }

  String &operator= (char rhs) {
    return assign (rhs);
  }

  String &operator+= (const String &rhs) {
    return append (rhs);
  }

  // overload to avoid the implicit temporary string conversion from stringref
  String &operator+= (StringRef rhs) {
    return append (rhs.chars (), rhs.size ());
  }

  String &operator+= (const char *rhs) {
    return append (rhs);
  }

  String &operator+= (char rhs) {
    return append (rhs);
  }

  const char &operator[] (size_t index) const {
    return chars ()[index];
  }

  char &operator[] (size_t index) {
    return mutable_data ()[index];
  }

  friend String operator+ (const String &lhs, char rhs) {
    return String (lhs).append (rhs);
  }

  friend String operator+ (char lhs, const String &rhs) {
    return String (lhs).append (rhs);
  }

  friend String operator+ (const String &lhs, const char *rhs) {
    return String (lhs).append (rhs);
  }

  friend String operator+ (const char *lhs, const String &rhs) {
    return String (lhs).append (rhs);
  }

  friend String operator+ (const String &lhs, const String &rhs) {
    return String (lhs).append (rhs);
  }

  friend bool operator== (const String &lhs, const String &rhs) {
    const auto len = lhs.size ();
    return len == rhs.size () && memcmp (lhs.chars (), rhs.chars (), len) == 0;
  }

  friend bool operator== (const char *lhs, const String &rhs) {
    return rhs == lhs;
  }

  friend bool operator== (const String &lhs, const char *rhs) {
    // contract: rhs must be null-terminated, see StringRef::operator==
    if (!rhs) [[unlikely]] {
      return lhs.empty ();
    }
    const auto len = lhs.size ();
    return strlen (rhs) == len && memcmp (lhs.chars (), rhs, len) == 0;
  }

  friend bool operator!= (const String &lhs, const String &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator!= (const char *lhs, const String &rhs) {
    return !(rhs == lhs);
  }

  friend bool operator!= (const String &lhs, const char *rhs) {
    return !(lhs == rhs);
  }

  friend bool operator== (const String &lhs, const StringRef &rhs) {
    return StringRef (lhs) == rhs;
  }

  friend bool operator== (const StringRef &lhs, const String &rhs) {
    return rhs == lhs;
  }

  friend bool operator!= (const String &lhs, const StringRef &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator!= (const StringRef &lhs, const String &rhs) {
    return !(lhs == rhs);
  }

public:
  // internal helper for join implementation
  template <typename ArrayType> static String join_impl (const ArrayType &sequence, StringRef delim, const size_t start = 0) {
    if (start >= sequence.size ()) {
      return "";
    }

    if (sequence.size () - start == 1) {
      return sequence.at (start);
    }

    // pre-calculate total length to minimize reallocations
    size_t total = 0;

    for (size_t i = start; i < sequence.size (); ++i) {
      if (i != start) {
        total += delim.size ();
      }
      total += sequence[i].size ();
    }
    String result {};
    result.reserve (total);

    for (size_t index = start; index < sequence.size (); ++index) {
      if (index != start) {
        result.append (delim.chars (), delim.size ());
      }
      result += sequence[index];
    }
    return result;
  }

  // join for array<string> (works with initializer lists)
  static String join (const Array<String> &sequence, StringRef delim, const size_t start = 0) {
    return join_impl (sequence, delim, start);
  }

  // join for smallarray<string> (sbo-optimized array, n != 0)
  template <size_t N>
  static String join (const Array<String, ReservePolicy::Proportional, N> &sequence, StringRef delim, const size_t start = 0) {
    return join_impl (sequence, delim, start);
  }
};

// sso relies on the exact 3-word layout, padding would break the tag overlap
static_assert (sizeof (String) == sizeof (size_t) * 3, "String SSO layout is broken by padding");

// constructor from string to string_ref
inline StringRef::StringRef (const String &str) : chars_ (str.chars ()), size_ (str.size ()) {}

// wrapping string for snprintf
inline const char *SNPrintfWrap::cast (const String &value) {
  return value.chars ();
}

// wrapping stringref for snprintf
inline const char *SNPrintfWrap::cast (const StringRef &value) {
  // note: peeking past the view would overread non-terminated slices
  return terminate_scratch (value.chars (), value.size ());
}

inline SNPrintfWrap::~SNPrintfWrap () {
  for (auto &slot : grown_scratch_) {
    slot = mem::release (slot);
  }
}

// copies the view bytes into a null-terminated scratch slot and returns it
const char *SNPrintfWrap::terminate_scratch (const char *chars, const size_t length) noexcept {
  if (++rotate_ >= ScratchSlots) {
    rotate_ = 0;
  }
  const auto slot = rotate_;

  if (length <= ScratchSize) {
    memcpy (static_scratch_[slot].data (), chars, length);
    static_scratch_[slot][length] = kNullChar;

    return static_scratch_[slot].data ();
  }
  if (grown_capacity_[slot] < length + 1) {
    grown_scratch_[slot] = mem::reallocate (grown_scratch_[slot], length + 1);
    grown_capacity_[slot] = length + 1;
  }
  auto dst = grown_scratch_[slot];

  memcpy (dst, chars, length);
  dst[length] = kNullChar;

  return dst;
}

// simple rotation-string pool for holding temporary data passed to different modules and for formatting
// single-threaded by contract, rotation state and buffers are shared mutable state
class Strings final : public Singleton<Strings> {
public:
  enum : size_t {
    StaticBufferSize = static_cast<size_t> (2048),
    RotationCount = static_cast<size_t> (8)
  };

private:
  char data_[RotationCount][StaticBufferSize + 1] {};
  size_t rotate_ = 0;

public:
  Strings () = default;
  ~Strings () = default;

public:
  char *chars () noexcept {
    if (++rotate_ >= RotationCount) {
      rotate_ = 0;
    }
    auto result = data_[ystl::clamp<size_t> (rotate_, 0, RotationCount)];
    result[0] = kNullChar;

    return result;
  }

  template <typename U, typename... Args> U *format (const U *fmt, Args &&...args) noexcept {
    static_assert (ystl::is_same_v<U, char>, "Strings::format works on char only");

    auto buffer = chars ();
    const auto result = fmtwrap ().exec (buffer, StaticBufferSize, fmt, args...);

    // fail loud on truncation, a cut path or message is worse than a crash here
    if (result < 0 || static_cast<size_t> (result) >= StaticBufferSize) [[unlikely]] {
      plat.abort ("Strings::format result does not fit the static buffer");
    }
    return buffer;
  }

  template <typename... Args> String join_path (Args &&...args) noexcept {
    Array<String> data ({ ystl::forward<Args> (args)... });
    return String::join (data, kPathSeparator);
  }

  template <typename U> U *format (const U *fmt) noexcept {
    static_assert (ystl::is_same_v<U, char>, "Strings::format works on char only");

    auto buffer = chars ();
    copy (buffer, fmt, StaticBufferSize);

    return buffer;
  }

  static bool is_empty (const char *input) noexcept {
    if (input == nullptr) {
      return true;
    }
    return *input == kNullChar;
  }

  bool matches (const char *str1, const char *str2) noexcept {
    if (str1 == str2) {
      return true;
    }

    if (!str1 || !str2) [[unlikely]] {
      return false;
    }
#if defined(YSTL_WINDOWS)
    return _stricmp (str1, str2) == 0;
#else
    return ::strcasecmp (str1, str2) == 0;
#endif
  }

  template <typename U> U *copy (U *dst, const U *src, size_t dst_capacity) noexcept {
    if (dst_capacity == 0) {
      return dst;
    }

    if (!src) {
      dst[0] = kNullChar;
      return dst;
    }
    size_t srcLen = 0;

    while (srcLen < dst_capacity - 1 && src[srcLen] != kNullChar) {
      ++srcLen;
    }

    for (size_t i = 0; i < srcLen; ++i) {
      dst[i] = src[i];
    }
    dst[srcLen] = kNullChar;

    return dst;
  }

  template <typename U> U *concat (U *dst, const U *src, size_t dst_capacity) noexcept {
    if (dst_capacity == 0) {
      return dst;
    }

    if (!src) {
      return dst;
    }
    size_t dst_len = 0;

    while (dst_len < dst_capacity - 1 && dst[dst_len] != kNullChar) {
      ++dst_len;
    }
    dst[dst_capacity - 1] = kNullChar;

    if (dst_len >= dst_capacity - 1) {
      return dst;
    }
    size_t space_left = dst_capacity - dst_len - 1;
    size_t i = 0;

    while (i < space_left && src[i] != kNullChar) {
      dst[dst_len + i] = src[i];
      ++i;
    }
    dst[dst_len + i] = kNullChar;

    return dst;
  }
};

// expose global string pool
YSTL_EXPOSE_GLOBAL_SINGLETON (Strings, strings);

}
