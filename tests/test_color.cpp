// test_color.cpp - tests for ystl/color.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction
TEST_CASE ("Color default construction is zero [color]") {
  Color c;
  REQUIRE (c.red == 0);
  REQUIRE (c.green == 0);
  REQUIRE (c.blue == 0);
}

TEST_CASE ("Color parameterised construction stores RGB [color]") {
  Color c (255, 128, 64);
  REQUIRE (c.red == 255);
  REQUIRE (c.green == 128);
  REQUIRE (c.blue == 64);
}

// reset
TEST_CASE ("Color::reset zeroes all channels [color]") {
  Color c (100, 150, 200);
  c.reset ();
  REQUIRE (c.red == 0);
  REQUIRE (c.green == 0);
  REQUIRE (c.blue == 0);
}

// sum
TEST_CASE ("Color::sum returns red+green+blue [color]") {
  Color c (10, 20, 30);
  REQUIRE (c.sum () == 60);

  Color black;
  REQUIRE (black.sum () == 0);

  Color white (255, 255, 255);
  REQUIRE (white.sum () == 765);
}

// avg
TEST_CASE ("Color::avg returns integer average of channels [color]") {
  Color c (0, 0, 0);
  REQUIRE (c.avg () == 0);

  Color c2 (10, 20, 30);
  REQUIRE (c2.avg () == 20); // sum=60, /3 = 20

  Color c3 (255, 255, 255);
  REQUIRE (c3.avg () == 255); // 765/3 = 255
}

// operator == / operator
TEST_CASE ("Color operator== compares all channels [color]") {
  Color a (100, 150, 200);
  Color b (100, 150, 200);
  Color c (100, 150, 201);

  REQUIRE (a == b);
  REQUIRE_FALSE (a == c);
}

TEST_CASE ("Color operator!= detects any difference [color]") {
  Color a (100, 150, 200);
  Color b (100, 150, 200);
  Color c (101, 150, 200);

  REQUIRE_FALSE (a != b);
  REQUIRE (a != c);
}

// operator + / operator -
TEST_CASE ("Color operator+ adds channels [color]") {
  Color a (10, 20, 30);
  Color b (5, 10, 15);
  Color result = a + b;

  REQUIRE (result.red == 15);
  REQUIRE (result.green == 30);
  REQUIRE (result.blue == 45);
}

TEST_CASE ("Color operator- subtracts channels [color]") {
  Color a (100, 150, 200);
  Color b (10, 20, 30);
  Color result = a - b;

  REQUIRE (result.red == 90);
  REQUIRE (result.green == 130);
  REQUIRE (result.blue == 170);
}

// operator * / operator /
TEST_CASE ("Color operator* multiplies by scalar [color]") {
  Color c (10, 20, 30);
  Color result = c * 3;

  REQUIRE (result.red == 30);
  REQUIRE (result.green == 60);
  REQUIRE (result.blue == 90);
}

TEST_CASE ("Color operator/ divides by scalar [color]") {
  Color c (30, 60, 90);
  Color result = c / 3;

  REQUIRE (result.red == 10);
  REQUIRE (result.green == 20);
  REQUIRE (result.blue == 30);
}

// operator += / operator
TEST_CASE ("Color operator+= adds in place [color]") {
  Color a (10, 20, 30);
  Color b (5, 10, 15);
  a += b;

  REQUIRE (a.red == 15);
  REQUIRE (a.green == 30);
  REQUIRE (a.blue == 45);
}

TEST_CASE ("Color operator-= subtracts in place [color]") {
  Color a (100, 150, 200);
  Color b (10, 20, 30);
  a -= b;

  REQUIRE (a.red == 90);
  REQUIRE (a.green == 130);
  REQUIRE (a.blue == 170);
}

// operator *= / operator
TEST_CASE ("Color operator*= multiplies in place [color]") {
  Color c (10, 20, 30);
  c *= 2;

  REQUIRE (c.red == 20);
  REQUIRE (c.green == 40);
  REQUIRE (c.blue == 60);
}

TEST_CASE ("Color operator/= divides in place [color]") {
  Color c (20, 40, 60);
  c /= 2;

  REQUIRE (c.red == 10);
  REQUIRE (c.green == 20);
  REQUIRE (c.blue == 30);
}

// clamped / clamp
TEST_CASE ("Color clamped returns new color with values in 0-255 [color]") {
  Color c (300, -50, 128);
  Color result = c.clamped ();

  REQUIRE (result.red == 255);
  REQUIRE (result.green == 0);
  REQUIRE (result.blue == 128);

  // original unchanged
  REQUIRE (c.red == 300);
  REQUIRE (c.green == -50);
}

TEST_CASE ("Color clamp modifies in place [color]") {
  Color c (300, -50, 128);
  c.clamp ();

  REQUIRE (c.red == 255);
  REQUIRE (c.green == 0);
  REQUIRE (c.blue == 128);
}

TEST_CASE ("Color clamped on already valid values returns same [color]") {
  Color c (0, 128, 255);
  Color result = c.clamped ();

  REQUIRE (result.red == 0);
  REQUIRE (result.green == 128);
  REQUIRE (result.blue == 255);
}

// defaulted three-way
TEST_CASE ("Color defaulted operator<=> orders lexicographically [color]") {
  Color a (10, 20, 30);
  Color b (10, 20, 30);
  Color c (10, 21, 30);

  REQUIRE (a == b);
  REQUIRE (a < c);
  REQUIRE (c > a);
  REQUIRE (a <= b);
  REQUIRE (a >= b);
  REQUIRE (a != c);
}
