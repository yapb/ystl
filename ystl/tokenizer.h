// SPDX-License-Identifier: Unlicense

#pragma once

// lightweight forward cursor with line tracking and zero-copy spans

#include <ystl/string.h>

namespace ystl {

class Tokenizer final {
private:
  const char *text_ = "";
  size_t size_ = 0;
  size_t pos_ = 0;
  size_t line_ = 1;

public:
  Tokenizer () = default;

  Tokenizer (StringRef text) : text_ (text.chars ()), size_ (text.size ()) {}

public:
  // text being scanned
  StringRef text () const {
    return StringRef (text_, size_);
  }

  // current position in the buffer
  size_t pos () const {
    return pos_;
  }

  // current line number, starts at 1
  size_t line () const {
    return line_;
  }

  bool eof () const {
    return pos_ >= size_;
  }

  // character at offset from the current position, null char when out of bounds
  char peek (size_t offset = 0) const {
    const auto at = pos_ + offset;

    if (at < size_) {
      return text_[at];
    }
    return kNullChar;
  }

  // consumes a single character, keeping track of the line number
  void advance () {
    if (pos_ < size_) {
      if (text_[pos_] == '\n') {
        ++line_;
      }
      ++pos_;
    }
  }

  // consumes the current character when it matches
  bool accept (char ch) {
    if (peek () == ch) {
      advance ();
      return true;
    }
    return false;
  }

  // consumes characters matching the predicate
  template <typename Pred> void skip_while (Pred pred) {
    while (!eof () && pred (peek ())) {
      advance ();
    }
  }

  // consumes characters matching the predicate, returns them as a span
  template <typename Pred> StringRef read_while (Pred pred) {
    const auto start = pos_;

    skip_while (pred);
    return StringRef (text_ + start, pos_ - start);
  }

  // reads until the terminator, leaving the terminator unconsumed
  StringRef read_until (char ch) {
    const auto start = pos_;

    while (!eof () && peek () != ch) {
      advance ();
    }
    return StringRef (text_ + start, pos_ - start);
  }

  // rest of the input as a span, without consuming anything
  StringRef rest () const {
    return StringRef (text_ + pos_, size_ - pos_);
  }

  // jumps to an absolute position; line tracking is intentionally not restored
  void seek (size_t pos) {
    pos_ = pos < size_ ? pos : size_;
  }

  // skips inline whitespace (spaces and tabs, never crosses the line end)
  void skip_spaces () {
    skip_while (is_inline_space);
  }

  // skips everything up to the line end, the newline itself is not consumed
  void skip_line () {
    read_until ('\n');
  }

public:
  static constexpr bool is_space (char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
  }

  static constexpr bool is_inline_space (char ch) {
    return ch == ' ' || ch == '\t';
  }

  // trims characters from both ends of the span
  static StringRef trim (StringRef span, StringRef chars) {
    size_t begin = 0;
    size_t end = span.size ();

    while (begin < end && chars.find (span[begin]) != StringRef::InvalidIndex) {
      ++begin;
    }
    while (end > begin && chars.find (span[end - 1]) != StringRef::InvalidIndex) {
      --end;
    }
    return span.substr (begin, end - begin);
  }
};

}
