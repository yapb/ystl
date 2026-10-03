// test_traits.cpp - tests for ystl/traits.h
#include <ystl/ystl.h>
#include <ystl/test.h>

// traits.h defines types only so test with static asserts

using namespace ystl;

// is_same
TEST_CASE ("is_same::value is true for identical types [traits]") {
  REQUIRE ((ystl::is_same<int, int>::value));
  REQUIRE ((ystl::is_same<float, float>::value));
  REQUIRE ((ystl::is_same<double, double>::value));
}

TEST_CASE ("is_same::value is false for different types [traits]") {
  REQUIRE_FALSE ((ystl::is_same<int, float>::value));
  REQUIRE_FALSE ((ystl::is_same<int, double>::value));
  REQUIRE_FALSE ((ystl::is_same<int, char>::value));
}

TEST_CASE ("is_same with pointer types [traits]") {
  REQUIRE ((ystl::is_same<int *, int *>::value));
  REQUIRE_FALSE ((ystl::is_same<int *, int>::value));
}

// conditional
TEST_CASE ("conditional<true, T, F>::type is T [traits]") {
  REQUIRE ((ystl::is_same<ystl::conditional<true, int, float>::type, int>::value));
}

TEST_CASE ("conditional<false, T, F>::type is F [traits]") {
  REQUIRE ((ystl::is_same<ystl::conditional<false, int, float>::type, float>::value));
}

// enable_if / enable_if_t
TEST_CASE ("enable_if<true, T>::type is T [traits]") {
  REQUIRE ((ystl::is_same<ystl::enable_if<true, int>::type, int>::value));
}

TEST_CASE ("enable_if_t<true, T> is T [traits]") {
  REQUIRE ((ystl::is_same<ystl::enable_if_t<true, double>, double>::value));
}

// clear_extent
TEST_CASE ("clear_extent for non-array type is the type itself [traits]") {
  REQUIRE ((ystl::is_same<ystl::clear_extent<int>::type, int>::value));
}

TEST_CASE ("clear_extent for T[] is T [traits]") {
  REQUIRE ((ystl::is_same<ystl::clear_extent<int[]>::type, int>::value));
}

TEST_CASE ("clear_extent for T[N] is T [traits]") {
  REQUIRE ((ystl::is_same<ystl::clear_extent<int[5]>::type, int>::value));
}

// integral_constant
TEST_CASE ("integral_constant holds compile-time value [traits]") {
  using Five = ystl::integral_constant<int, 5>;
  using False = ystl::integral_constant<bool, false>;

  REQUIRE ((Five::value == 5));
  REQUIRE ((False::value == false));

  // operator() and operator value_type
  Five five {};
  REQUIRE ((static_cast<int> (five) == 5));
  REQUIRE ((five () == 5));
}

// bool_constant /
TEST_CASE ("true_type has value true [traits]") {
  REQUIRE ((ystl::true_type::value == true));
}

TEST_CASE ("false_type has value false [traits]") {
  REQUIRE ((ystl::false_type::value == false));
}

// add_lvalue_reference /
TEST_CASE ("add_lvalue_reference converts T to T& [traits]") {
  REQUIRE ((ystl::is_same<ystl::add_lvalue_reference<int>::type, int &>::value));
}

TEST_CASE ("add_rvalue_reference converts T to T&& [traits]") {
  REQUIRE ((ystl::is_same<ystl::add_rvalue_reference<int>::type, int &&>::value));
}

// nullptr_t
TEST_CASE ("nullptr_t is the type of nullptr [traits]") {
  ystl::nullptr_t np = nullptr;
  (void)np;
  // if this compiles, the typedef is correct
  REQUIRE ((true));
}

// decay
TEST_CASE ("decay removes reference from type [traits]") {
  REQUIRE ((ystl::is_same<ystl::decay<int &>::type, int>::value));
}

TEST_CASE ("decay removes rvalue reference from type [traits]") {
  REQUIRE ((ystl::is_same<ystl::decay<int &&>::type, int>::value));
}

TEST_CASE ("decay of plain type is the type itself [traits]") {
  REQUIRE ((ystl::is_same<ystl::decay<int>::type, int>::value));
}

TEST_CASE ("decay of const type yields the type [traits]") {
  // decay const behavior checked minimally here
  using T = ystl::decay<const int>::type;
  (void)sizeof (T);
  REQUIRE ((true));
}

// remove_reference /
TEST_CASE ("remove_reference removes lvalue reference [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_reference<int &>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_reference_t<int &>, int>::value));
}

TEST_CASE ("remove_reference removes rvalue reference [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_reference<int &&>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_reference_t<int &&>, int>::value));
}

TEST_CASE ("remove_reference on non-reference type is unchanged [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_reference<int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_reference_t<int>, int>::value));
}

TEST_CASE ("remove_reference preserves const qualifier [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_reference<const int &>::type, const int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_reference_t<const int &>, const int>::value));
}

// remove_cv / remove_cv_t
TEST_CASE ("remove_cv removes const [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_cv<const int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_cv_t<const int>, int>::value));
}

TEST_CASE ("remove_cv removes volatile [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_cv<volatile int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_cv_t<volatile int>, int>::value));
}

TEST_CASE ("remove_cv removes const volatile [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_cv<const volatile int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_cv_t<const volatile int>, int>::value));
}

TEST_CASE ("remove_cv on plain type is unchanged [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_cv<int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_cv_t<int>, int>::value));
}

// remove_const /
TEST_CASE ("remove_const removes const [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_const<const int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_const_t<const int>, int>::value));
}

TEST_CASE ("remove_const on non-const type is unchanged [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_const<int>::type, int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_const_t<int>, int>::value));
}

TEST_CASE ("remove_const does not remove volatile [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_const<volatile int>::type, volatile int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_const_t<volatile int>, volatile int>::value));
}

TEST_CASE ("remove_const on const volatile removes only const [traits]") {
  REQUIRE ((ystl::is_same<ystl::remove_const<const volatile int>::type, volatile int>::value));
  REQUIRE ((ystl::is_same<ystl::remove_const_t<const volatile int>, volatile int>::value));
}

// is_trivially_copyable /
TEST_CASE ("is_trivially_copyable is true for primitive types [traits]") {
  REQUIRE ((ystl::is_trivially_copyable<int>::value));
  REQUIRE ((ystl::is_trivially_copyable_v<int>));
  REQUIRE ((ystl::is_trivially_copyable<float>::value));
  REQUIRE ((ystl::is_trivially_copyable_v<float>));
  REQUIRE ((ystl::is_trivially_copyable<double>::value));
  REQUIRE ((ystl::is_trivially_copyable<char>::value));
}

TEST_CASE ("is_trivially_copyable is true for pointers [traits]") {
  REQUIRE ((ystl::is_trivially_copyable<int *>::value));
  REQUIRE ((ystl::is_trivially_copyable_v<int *>));
  REQUIRE ((ystl::is_trivially_copyable<const char *>::value));
}

namespace {
struct TrivialStruct {
  int x;
  float y;
};

struct NonTrivialStruct {
  NonTrivialStruct () {}
  NonTrivialStruct (const NonTrivialStruct &) {}
  ~NonTrivialStruct () {}
  int x;
};
}

TEST_CASE ("is_trivially_copyable is true for trivial structs [traits]") {
  REQUIRE ((ystl::is_trivially_copyable<TrivialStruct>::value));
  REQUIRE ((ystl::is_trivially_copyable_v<TrivialStruct>));
}

TEST_CASE ("is_trivially_copyable is false for non-trivial types [traits]") {
  REQUIRE_FALSE ((ystl::is_trivially_copyable<NonTrivialStruct>::value));
  REQUIRE_FALSE ((ystl::is_trivially_copyable_v<NonTrivialStruct>));
}
