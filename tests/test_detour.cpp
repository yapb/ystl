// test_detour.cpp tests for ystl detour.h without installing hooks
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// a simple function signature we can use as the template parameter
using VoidFn = void ();
using IntFn = int (int);

// default construction
TEST_CASE ("Detour default-constructed is not valid [detour]") {
  Detour<VoidFn> d;
  REQUIRE (!d.valid ());
}

TEST_CASE ("Detour default-constructed is not detoured [detour]") {
  Detour<VoidFn> d;
  REQUIRE (!d.detoured ());
}

// detour / restore on
TEST_CASE ("Detour detour() returns false when not initialized [detour]") {
  Detour<VoidFn> d;
  REQUIRE (!d.detour ());
}

TEST_CASE ("Detour restore() returns false when not initialized [detour]") {
  Detour<VoidFn> d;
  REQUIRE (!d.restore ());
}

// install with nullptr
TEST_CASE ("Detour remains invalid after install with null function [detour]") {
  Detour<VoidFn> d;
  d.install (nullptr, false);
  // original_ is still null, so valid() == false
  REQUIRE (!d.valid ());
}

// on non-x86 / shim
#if defined(YSTL_ARCH_NON_X86) || (defined(YSTL_MACOS) && defined(YSTL_ARCH_X64))
TEST_CASE ("Detour shim initialize does not crash [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", nullptr);
  REQUIRE (!d.valid ());
  REQUIRE (!d.detoured ());
}
#endif

// on x86 / x64:
#if !defined(YSTL_ARCH_NON_X86) && !(defined(YSTL_MACOS) && defined(YSTL_ARCH_X64))
extern int sample_function (int x);

TEST_CASE ("Detour initialize with real function does not crash [detour]") {
  Detour<IntFn> d;
  // on windows empty module uses passed address directly
  d.initialize ("", "", sample_function);
  // valid needs both parts set and install not called yet
  REQUIRE (!d.valid ());
}

extern int replacement_function (int x);

TEST_CASE ("Detour install sets valid() to true [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.valid ());
  // detoured() is still false because we passed enable=false to install()
  REQUIRE (!d.detoured ());
}

TEST_CASE ("Detour destructor restores safely [detour]") {
  {
    Detour<IntFn> d;
    d.initialize ("", "", sample_function);
    d.install (reinterpret_cast<void *> (replacement_function), false);
    REQUIRE (d.valid ());
    // destructor calls restore() - should not crash even though hook is not patched
  }
  REQUIRE (true);
}

TEST_CASE ("Detour detour() returns true when valid [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.valid ());
  REQUIRE (!d.detoured ());
  REQUIRE (d.detour ());
  REQUIRE (d.detoured ());
}

TEST_CASE ("Detour restore() returns true after detour [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.detour ());
  REQUIRE (d.detoured ());
  REQUIRE (d.restore ());
  REQUIRE (!d.detoured ());
}

TEST_CASE ("Detour multiple detour/restore cycles work [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);

  for (int i = 0; i < 3; ++i) {
    REQUIRE (!d.detoured ());
    REQUIRE (d.detour ());
    REQUIRE (d.detoured ());
    REQUIRE (d.restore ());
    REQUIRE (!d.detoured ());
  }
}

TEST_CASE ("Detour install with enable=true immediately detours [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), true);
  REQUIRE (d.valid ());
  REQUIRE (d.detoured ());
}

TEST_CASE ("Detour double detour call is safe [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.detour ());
  REQUIRE (d.detoured ());
  REQUIRE (d.detour ());
  REQUIRE (d.detoured ());
}

TEST_CASE ("Detour double restore call is safe [detour]") {
  Detour<IntFn> d;
  d.initialize ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.detour ());
  REQUIRE (d.restore ());
  REQUIRE (!d.detoured ());
  REQUIRE (d.restore ());
  REQUIRE (!d.detoured ());
}

TEST_CASE ("Detour constructor with parameters initializes correctly [detour]") {
  Detour<IntFn> d ("", "", sample_function);
  d.install (reinterpret_cast<void *> (replacement_function), false);
  REQUIRE (d.valid ());
}
#endif
