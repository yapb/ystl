// test_singleton.cpp tests for ystl singleton.h with ystl first
#include <ystl/ystl.h>
#include <ystl/test.h>

// ystl::singleton
namespace {
struct MyTestSingleton : public ystl::Singleton<MyTestSingleton> {
  int value { 42 };
  explicit MyTestSingleton () = default;
};
}

TEST_CASE ("Singleton::instance returns the same object every time [singleton]") {
  auto &a = MyTestSingleton::instance ();
  auto &b = MyTestSingleton::instance ();
  REQUIRE (&a == &b);
  REQUIRE (a.value == 42);

  a.value = 99;
  REQUIRE (b.value == 99); // same object
  a.value = 42; // restore
}
