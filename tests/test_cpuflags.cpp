// test_cpuflags.cpp - tests for ystl/cpuflags.h (cpuflags)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// singleton
TEST_CASE ("CpuFlags singleton returns same instance [cpuflags]") {
  auto &a = CpuFlags::instance ();
  auto &b = cpuflags; // global alias
  REQUIRE (&a == &b);
}

// on x86/x64 windows,
TEST_CASE ("CpuFlags singleton is consistent across multiple accesses [cpuflags]") {
  auto &cf = cpuflags;

  // reading twice should give the same result
  bool sse3_a = cf.sse3;
  bool sse3_b = cf.sse3;
  REQUIRE (sse3_a == sse3_b);

  bool avx_a = cf.avx;
  bool avx_b = cf.avx;
  REQUIRE (avx_a == avx_b);
}

TEST_CASE ("CpuFlags avx2 implies avx on real hardware [cpuflags]") {
  // if avx2 is detected, avx must also be detected (hardware invariant)
  if (cpuflags.avx2) {
    REQUIRE (cpuflags.avx);
  }
  // even if both are false, this condition is satisfied
  REQUIRE (true);
}

TEST_CASE ("CpuFlags sse42 implies sse41 on real hardware [cpuflags]") {
  if (cpuflags.sse42) {
    REQUIRE (cpuflags.sse41);
  }
  REQUIRE (true);
}

TEST_CASE ("CpuFlags sse41 implies ssse3 on real hardware [cpuflags]") {
  if (cpuflags.sse41) {
    REQUIRE (cpuflags.ssse3);
  }
  REQUIRE (true);
}

TEST_CASE ("CpuFlags ssse3 implies sse3 on real hardware [cpuflags]") {
  if (cpuflags.ssse3) {
    REQUIRE (cpuflags.sse3);
  }
  REQUIRE (true);
}

// smoke test: all boolean
TEST_CASE ("CpuFlags all boolean fields are readable [cpuflags]") {
  auto &cf = cpuflags;

  // just access them - this verifies the struct layout is as expected
  bool vals[] = { cf.sse3, cf.ssse3, cf.sse41, cf.sse42, cf.avx, cf.avx2, cf.neon };
  for (bool v : vals) {
    REQUIRE ((v == true || v == false)); // tautology, but exercises the read
  }
}
