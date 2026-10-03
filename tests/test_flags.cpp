// test_flags.cpp – tests for ystl/flags.h arithmetic operations
#include <ystl/ystl.h>
#include <ystl/test.h>

// sequential enum with arithmetic operators only (no bitwise)
enum class TestFlags : int {
  None = 0,
  One = 1,
  Two = 2,
  Three = 3,
  Four = 4,
  Five = 5,
  Ten = 10
};
YSTL_ENABLE_ENUM_ARITHMETIC (TestFlags);

// plain bitmask enum (bitwise operators only, no arithmetic)
enum class BitFlags : unsigned int {
  None = 0,
  Flag1 = 1 << 0,
  Flag2 = 1 << 1,
  Flag4 = 1 << 2,
  Flag8 = 1 << 3,
  All = Flag1 | Flag2 | Flag4 | Flag8
};
YSTL_ENABLE_ENUM_FLAGS (BitFlags);

// arithmetic operators: +
TEST_CASE ("operator+ adds two flags [flags]") {
  REQUIRE (static_cast<int> (TestFlags::Two + TestFlags::Three) == 5);
  REQUIRE (static_cast<int> (TestFlags::One + TestFlags::Four) == 5);
  REQUIRE (static_cast<int> (TestFlags::Five + TestFlags::Five) == 10);
}

TEST_CASE ("operator+ with mixed types [flags]") {
  REQUIRE (static_cast<int> (5 + TestFlags::Five) == 10);
  REQUIRE (static_cast<int> (TestFlags::Five + 5) == 10);
  REQUIRE (static_cast<int> (0 + TestFlags::None) == 0);
}

// arithmetic operators: -
TEST_CASE ("operator- subtracts two flags [flags]") {
  REQUIRE (static_cast<int> (TestFlags::Five - TestFlags::Two) == 3);
  REQUIRE (static_cast<int> (TestFlags::Ten - TestFlags::Five) == 5);
  REQUIRE (static_cast<int> (TestFlags::Three - TestFlags::One) == 2);
}

TEST_CASE ("operator- with mixed types [flags]") {
  REQUIRE (static_cast<int> (10 - TestFlags::Five) == 5);
  REQUIRE (static_cast<int> (TestFlags::Ten - 5) == 5);
  REQUIRE (static_cast<int> (TestFlags::Five - 0) == 5);
}

// arithmetic operators: *
TEST_CASE ("operator* multiplies two flags [flags]") {
  REQUIRE (static_cast<int> (TestFlags::Two * TestFlags::Three) == 6);
  REQUIRE (static_cast<int> (TestFlags::Five * TestFlags::Two) == 10);
  REQUIRE (static_cast<int> (TestFlags::Four * TestFlags::Four) == 16);
}

TEST_CASE ("operator* with mixed types [flags]") {
  REQUIRE (static_cast<int> (2 * TestFlags::Five) == 10);
  REQUIRE (static_cast<int> (TestFlags::Five * 2) == 10);
  REQUIRE (static_cast<int> (TestFlags::None * 100) == 0);
  REQUIRE (static_cast<int> (100 * TestFlags::None) == 0);
}

// arithmetic operators: /
TEST_CASE ("operator/ divides two flags [flags]") {
  REQUIRE (static_cast<int> (TestFlags::Ten / TestFlags::Two) == 5);
  REQUIRE (static_cast<int> (TestFlags::Ten / TestFlags::Five) == 2);
  REQUIRE (static_cast<int> (TestFlags::Four / TestFlags::Two) == 2);
}

TEST_CASE ("operator/ with mixed types [flags]") {
  REQUIRE (static_cast<int> (10 / TestFlags::Two) == 5);
  REQUIRE (static_cast<int> (TestFlags::Ten / 2) == 5);
  REQUIRE (static_cast<int> (TestFlags::Five / 5) == 1);
}

// arithmetic operators: %
TEST_CASE ("operator% modulo two flags [flags]") {
  REQUIRE (static_cast<int> (TestFlags::Ten % TestFlags::Three) == 1);
  REQUIRE (static_cast<int> (TestFlags::Five % TestFlags::Two) == 1);
  REQUIRE (static_cast<int> (TestFlags::Four % TestFlags::Two) == 0);
}

TEST_CASE ("operator% with mixed types [flags]") {
  REQUIRE (static_cast<int> (10 % TestFlags::Three) == 1);
  REQUIRE (static_cast<int> (TestFlags::Ten % 3) == 1);
  REQUIRE (static_cast<int> (TestFlags::Five % 2) == 1);
}

// compound assignment
TEST_CASE ("operator+= adds and assigns [flags]") {
  TestFlags a = TestFlags::Two;
  a += TestFlags::Three;
  REQUIRE (static_cast<int> (a) == 5);

  TestFlags b = TestFlags::Five;
  b += TestFlags::Five;
  REQUIRE (static_cast<int> (b) == 10);
}

// compound assignment
TEST_CASE ("operator-= subtracts and assigns [flags]") {
  TestFlags a = TestFlags::Five;
  a -= TestFlags::Two;
  REQUIRE (static_cast<int> (a) == 3);

  TestFlags b = TestFlags::Ten;
  b -= TestFlags::Five;
  REQUIRE (static_cast<int> (b) == 5);
}

// compound assignment
TEST_CASE ("operator*= multiplies and assigns [flags]") {
  TestFlags a = TestFlags::Two;
  a *= TestFlags::Three;
  REQUIRE (static_cast<int> (a) == 6);

  TestFlags b = TestFlags::Five;
  b *= TestFlags::Two;
  REQUIRE (static_cast<int> (b) == 10);
}

// compound assignment
TEST_CASE ("operator/= divides and assigns [flags]") {
  TestFlags a = TestFlags::Ten;
  a /= TestFlags::Two;
  REQUIRE (static_cast<int> (a) == 5);

  TestFlags b = TestFlags::Ten;
  b /= TestFlags::Five;
  REQUIRE (static_cast<int> (b) == 2);
}

// compound assignment
TEST_CASE ("operator%= modulo and assigns [flags]") {
  TestFlags a = TestFlags::Ten;
  a %= TestFlags::Three;
  REQUIRE (static_cast<int> (a) == 1);

  TestFlags b = TestFlags::Five;
  b %= TestFlags::Two;
  REQUIRE (static_cast<int> (b) == 1);
}

// increment operators: ++
TEST_CASE ("prefix operator++ increments and returns reference [flags]") {
  TestFlags a = TestFlags::Two;
  TestFlags &result = ++a;
  REQUIRE (static_cast<int> (result) == 3);
  REQUIRE (static_cast<int> (a) == 3);
  REQUIRE (&result == &a);
}

TEST_CASE ("postfix operator++ increments and returns old value [flags]") {
  TestFlags a = TestFlags::Two;
  TestFlags result = a++;
  REQUIRE (static_cast<int> (result) == 2);
  REQUIRE (static_cast<int> (a) == 3);
}

// decrement operators: --
TEST_CASE ("prefix operator-- decrements and returns reference [flags]") {
  TestFlags a = TestFlags::Five;
  TestFlags &result = --a;
  REQUIRE (static_cast<int> (result) == 4);
  REQUIRE (static_cast<int> (a) == 4);
  REQUIRE (&result == &a);
}

TEST_CASE ("postfix operator-- decrements and returns old value [flags]") {
  TestFlags a = TestFlags::Five;
  TestFlags result = a--;
  REQUIRE (static_cast<int> (result) == 5);
  REQUIRE (static_cast<int> (a) == 4);
}

// edge cases
TEST_CASE ("arithmetic with None (zero) [flags]") {
  REQUIRE (static_cast<int> (TestFlags::None + TestFlags::Five) == 5);
  REQUIRE (static_cast<int> (TestFlags::Five + TestFlags::None) == 5);
  REQUIRE (static_cast<int> (TestFlags::Five - TestFlags::None) == 5);
  REQUIRE (static_cast<int> (TestFlags::None * TestFlags::Five) == 0);
}

TEST_CASE ("chained arithmetic operations [flags]") {
  TestFlags a = TestFlags::Two;
  a += TestFlags::Three;
  a *= TestFlags::Two;
  REQUIRE (static_cast<int> (a) == 10);

  a -= TestFlags::Five;
  REQUIRE (static_cast<int> (a) == 5);

  a /= TestFlags::Five;
  REQUIRE (static_cast<int> (a) == 1);
}

// hash tests for flags
TEST_CASE ("Hash struct works with flags types [flags][hash]") {
  ystl::Hash<TestFlags> hasher;

  TestFlags a = TestFlags::Five;
  TestFlags b = TestFlags::Five;
  TestFlags c = TestFlags::Ten;

  uint32_t hash_a = hasher (a);
  uint32_t hash_b = hasher (b);
  uint32_t hash_c = hasher (c);

  REQUIRE (hash_a == hash_b);
  REQUIRE (hash_a != hash_c);
}

TEST_CASE ("HashMap works with flags as key [flags][hashmap]") {
  ystl::HashMap<TestFlags, int> map;

  map[TestFlags::One] = 1;
  map[TestFlags::Two] = 2;
  map[TestFlags::Five] = 5;

  REQUIRE (map.size () == 3);
  REQUIRE (*map.find (TestFlags::One) == 1);
  REQUIRE (*map.find (TestFlags::Two) == 2);
  REQUIRE (*map.find (TestFlags::Five) == 5);
  REQUIRE (map.find (TestFlags::Three) == nullptr);
}

TEST_CASE ("is_flags_hashable_v is true for declared flags [flags][traits]") {
  REQUIRE (ystl::is_flags_hashable_v<TestFlags> == true);
}

// flag manipulation
TEST_CASE ("hasFlag checks if single flag is set [flags]") {
  BitFlags flags = BitFlags::Flag1;
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == false);
  REQUIRE (has_flag (flags, BitFlags::None) == false);
}

TEST_CASE ("hasFlag checks combined flags [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag4) == false);
  REQUIRE (has_flag (flags, BitFlags::All) == true);
}

TEST_CASE ("hasFlag with mixed types [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;
  REQUIRE (has_flag (flags, 1u) == true);
  REQUIRE (has_flag (flags, 2u) == true);
  REQUIRE (has_flag (flags, 4u) == false);
  REQUIRE (has_flag (3u, BitFlags::Flag1) == true);
  REQUIRE (has_flag (3u, BitFlags::Flag4) == false);
}

TEST_CASE ("setFlag sets a single flag [flags]") {
  BitFlags flags = BitFlags::None;
  flags = set_flag (flags, BitFlags::Flag1);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);

  flags = set_flag (flags, BitFlags::Flag2);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
}

TEST_CASE ("setFlag with mixed types [flags]") {
  BitFlags flags = BitFlags::None;
  flags = set_flag (flags, 1u);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);

  flags = set_flag (2u, BitFlags::Flag1);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
}

TEST_CASE ("setFlag preserves already set flags [flags]") {
  BitFlags flags = BitFlags::Flag1;
  flags = set_flag (flags, BitFlags::Flag1);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);

  flags = set_flag (flags, BitFlags::Flag2);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
}

TEST_CASE ("clearFlag clears a single flag [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;
  flags = clear_flag (flags, BitFlags::Flag1);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == false);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
}

TEST_CASE ("clearFlag with mixed types [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;
  flags = clear_flag (flags, 1u);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == false);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);

  flags = clear_flag (3u, BitFlags::Flag1);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == false);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
}

TEST_CASE ("clearFlag on already cleared flag [flags]") {
  BitFlags flags = BitFlags::Flag1;
  flags = clear_flag (flags, BitFlags::Flag2);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == false);
}

TEST_CASE ("clearFlag clears all flags [flags]") {
  BitFlags flags = BitFlags::All;
  flags = clear_flag (flags, BitFlags::All);
  REQUIRE (flags == BitFlags::None);
}

TEST_CASE ("combined setFlag and clearFlag operations [flags]") {
  BitFlags flags = BitFlags::None;

  flags = set_flag (flags, BitFlags::Flag1);
  flags = set_flag (flags, BitFlags::Flag2);
  flags = set_flag (flags, BitFlags::Flag4);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag4) == true);

  flags = clear_flag (flags, BitFlags::Flag2);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == false);
  REQUIRE (has_flag (flags, BitFlags::Flag4) == true);

  flags = set_flag (flags, BitFlags::Flag2);
  flags = clear_flag (flags, BitFlags::Flag1 | BitFlags::Flag4);
  REQUIRE (has_flag (flags, BitFlags::Flag1) == false);
  REQUIRE (has_flag (flags, BitFlags::Flag2) == true);
  REQUIRE (has_flag (flags, BitFlags::Flag4) == false);
}

TEST_CASE ("hasFlag returns false for None [flags]") {
  REQUIRE (has_flag (BitFlags::None, BitFlags::Flag1) == false);
  REQUIRE (has_flag (BitFlags::None, BitFlags::None) == false);
}

// trait relationships
enum class HashOnlyEnum : int32_t {
  One = 1,
  Two = 2
};
YSTL_ENABLE_ENUM_HASH (HashOnlyEnum);

static_assert (ystl::is_flags_v<BitFlags> == true, "declared flags must be flags");
static_assert (ystl::is_flags_arithmetic_v<BitFlags> == false, "bitwise flags must not be arithmetic");
static_assert (ystl::is_flags_v<TestFlags> == false, "arithmetic-only enum must not be flags");
static_assert (ystl::is_flags_arithmetic_v<TestFlags> == true, "declared arithmetic enum must be arithmetic");
static_assert (ystl::is_flags_v<HashOnlyEnum> == false, "hash-only enum must not be flags");
static_assert (ystl::is_flags_arithmetic_v<HashOnlyEnum> == false, "hash-only enum must not be arithmetic");
static_assert (ystl::is_flags_hashable_v<HashOnlyEnum> == true, "hash-only enum must be hashable");
static_assert (ystl::is_flags_hashable_v<BitFlags> == true, "flags must be hashable by default");
static_assert (ystl::is_flags_hashable_v<TestFlags> == true, "arithmetic enums must be hashable by default");

TEST_CASE ("allFlags checks that all bits are set [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;

  REQUIRE (all_flags (flags, BitFlags::Flag1) == true);
  REQUIRE (all_flags (flags, BitFlags::Flag1 | BitFlags::Flag2) == true);
  REQUIRE (all_flags (flags, BitFlags::Flag1 | BitFlags::Flag2 | BitFlags::Flag4) == false);
  REQUIRE (all_flags (BitFlags::All, BitFlags::All) == true);
  REQUIRE (all_flags (BitFlags::None, BitFlags::None) == true);
  REQUIRE (all_flags (BitFlags::None, BitFlags::Flag1) == false);
}

TEST_CASE ("hasFlag with non-underlying integral type [flags]") {
  BitFlags flags = BitFlags::Flag1 | BitFlags::Flag2;

  // int is not the underlying type (unsigned int), generic overload must be picked
  REQUIRE (has_flag (3, BitFlags::Flag1) == true);
  REQUIRE (has_flag (3, BitFlags::Flag4) == false);
  REQUIRE (has_flag (flags, 3) == true);
}

TEST_CASE ("hash-only enum works as hashmap key [flags][hashmap]") {
  ystl::HashMap<HashOnlyEnum, int> map;

  map[HashOnlyEnum::One] = 1;
  map[HashOnlyEnum::Two] = 2;

  REQUIRE (map.size () == 2);
  REQUIRE (*map.find (HashOnlyEnum::One) == 1);
  REQUIRE (*map.find (HashOnlyEnum::Two) == 2);
  REQUIRE (map.find (static_cast<HashOnlyEnum> (3)) == nullptr);
}

TEST_CASE ("flags & flag works in boolean context [flags]") {
  BitFlags flags = BitFlags::Flag1;

  // contextual bool conversion relies on operator!
  REQUIRE (static_cast<bool> (flags & BitFlags::Flag1));
  REQUIRE (!static_cast<bool> (flags & BitFlags::Flag4));
  REQUIRE ((has_flag (flags, BitFlags::Flag1) && !has_flag (flags, BitFlags::Flag4)));
}
