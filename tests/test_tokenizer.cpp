// test_tokenizer.cpp – tests for ystl/tokenizer.h forward text cursor

#include <ystl/test.h>

#include <ystl/tokenizer.h>

using namespace ystl;

TEST_CASE ("cursor basics [tokenizer]") {
  Tokenizer scan { "hello\nworld" };

  REQUIRE (scan.line () == 1);
  REQUIRE (!scan.eof ());
  REQUIRE (scan.peek () == 'h');
  REQUIRE (scan.peek (4) == 'o');
  // volatile hides the constant from gcc's -warray-bounds when peek () gets inlined
  volatile size_t beyond = 100u;
  REQUIRE (scan.peek (beyond) == kNullChar);

  scan.advance ();
  REQUIRE (scan.pos () == 1);
  REQUIRE (scan.accept ('e'));
  REQUIRE (!scan.accept ('x'));

  // line tracking on newline
  scan.skip_while ([] (char ch) {
    return ch != '\n';
  });
  scan.skip_while (Tokenizer::is_space);
  REQUIRE (scan.line () == 2);
  REQUIRE (scan.rest () == "world");
}

TEST_CASE ("eof handling [tokenizer]") {
  Tokenizer scan { "ab" };

  scan.advance ();
  scan.advance ();
  REQUIRE (scan.eof ());

  // advance past the end is a no-op
  scan.advance ();
  REQUIRE (scan.pos () == 2);
  REQUIRE (scan.accept ('x') == false);
}

TEST_CASE ("readWhile / readUntil spans [tokenizer]") {
  Tokenizer scan { "key = value ; comment\n" };

  REQUIRE (scan.read_while ([] (char ch) {
    return !Tokenizer::is_inline_space (ch);
  }) == "key");
  scan.skip_spaces ();
  REQUIRE (scan.accept ('='));

  const auto value = scan.read_until (';');
  REQUIRE (value == " value ");
  REQUIRE (scan.peek () == ';');

  // terminator is left unconsumed
  REQUIRE (scan.accept (';'));
  REQUIRE (scan.rest () == " comment\n");
}

TEST_CASE ("seek restores position [tokenizer]") {
  Tokenizer scan { "one;two" };

  scan.skip_while ([] (char ch) {
    return ch != ';';
  });
  const auto saved = scan.pos ();
  REQUIRE (scan.rest () == ";two");

  scan.accept (';');
  scan.skip_line ();
  REQUIRE (scan.eof ());

  scan.seek (saved);
  REQUIRE (scan.rest () == ";two");
}

TEST_CASE ("trim spans [tokenizer]") {
  REQUIRE (Tokenizer::trim ("  hello  ", " ") == "hello");
  REQUIRE (Tokenizer::trim ("\t\"quoted\"\r\n", " \t\r\n\"") == "quoted");
  REQUIRE (Tokenizer::trim ("\"\"\"", "\"").empty ());
  REQUIRE (Tokenizer::trim ("abc", " ").size () == 3);
  REQUIRE (Tokenizer::trim ("", " ").empty ());
}
