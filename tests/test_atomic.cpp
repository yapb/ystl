// test_atomic.cpp - tests for ystl/atomic.h (atomic, memoryorder)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// constructors and

TEST_CASE ("Atomic default constructs to zero and explicit ctor stores value [atomic]") {
  Atomic<uint32_t> a;
  REQUIRE (a.load () == 0);

  Atomic<uint32_t> b (42);
  REQUIRE (b.load () == 42);
  REQUIRE (static_cast<uint32_t> (b) == 42);
}

TEST_CASE ("Atomic assignment operator stores value [atomic]") {
  Atomic<uint32_t> a;
  uint32_t returned = (a = 7);

  REQUIRE (returned == 7);
  REQUIRE (a.load () == 7);
}

// load / store / exchange

TEST_CASE ("Atomic load and store honor values and memory orders [atomic]") {
  Atomic<int64_t> a (10);

  a.store (77);
  REQUIRE (a.load () == 77);

  a.store (1, MemoryOrder::relaxed);
  REQUIRE (a.load (MemoryOrder::relaxed) == 1);
  REQUIRE (a.load (MemoryOrder::seq_cst) == 1);
}

TEST_CASE ("Atomic exchange returns previous value [atomic]") {
  Atomic<uint64_t> a (5);

  uint64_t old = a.exchange (9);
  REQUIRE (old == 5);
  REQUIRE (a.load () == 9);
}

// compareexchange

TEST_CASE ("Atomic compareExchange succeeds on match [atomic]") {
  Atomic<uint32_t> a (10);

  uint32_t expected = 10;
  REQUIRE (a.compare_exchange (expected, 20));
  REQUIRE (a.load () == 20);
}

TEST_CASE ("Atomic compareExchange updates expected on failure [atomic]") {
  Atomic<uint32_t> a (10);

  uint32_t expected = 99;
  REQUIRE_FALSE (a.compare_exchange (expected, 20));
  REQUIRE (expected == 10);
  REQUIRE (a.load () == 10);

  // retry with the actual value must now succeed
  REQUIRE (a.compare_exchange (expected, 20));
  REQUIRE (a.load () == 20);
}

TEST_CASE ("Atomic compareExchange loop implements atomic max [atomic]") {
  Atomic<uint32_t> maximum (10);

  uint32_t proposed = 15;
  uint32_t current = maximum.load ();

  while (proposed > current && !maximum.compare_exchange (current, proposed)) {
  }

  REQUIRE (maximum.load () == 15);
}

// fetch operations

TEST_CASE ("Atomic fetchAdd and fetchSub return previous value [atomic]") {
  Atomic<int32_t> a (10);

  REQUIRE (a.fetch_add (5) == 10);
  REQUIRE (a.load () == 15);

  REQUIRE (a.fetch_sub (7) == 15);
  REQUIRE (a.load () == 8);

  // signed wrap through zero
  REQUIRE (a.fetch_sub (10) == 8);
  REQUIRE (a.load () == -2);
}

TEST_CASE ("Atomic bitwise fetch operations [atomic]") {
  Atomic<uint32_t> a (0xF0);

  REQUIRE (a.fetch_or (0x0F) == 0xF0);
  REQUIRE (a.load () == 0xFF);

  REQUIRE (a.fetch_and (0x3C) == 0xFF);
  REQUIRE (a.load () == 0x3C);

  REQUIRE (a.fetch_xor (0xFF) == 0x3C);
  REQUIRE (a.load () == 0xC3);
}

// operators

TEST_CASE ("Atomic increment and decrement operators [atomic]") {
  Atomic<uint32_t> a (1);

  REQUIRE (++a == 2);
  REQUIRE (a++ == 2);
  REQUIRE (a.load () == 3);

  REQUIRE (--a == 2);
  REQUIRE (a-- == 2);
  REQUIRE (a.load () == 1);
}

TEST_CASE ("Atomic arithmetic and bitwise compound operators [atomic]") {
  Atomic<uint64_t> a (100);

  a += 50;
  REQUIRE (a.load () == 150);

  a -= 100;
  REQUIRE (a.load () == 50);

  a |= 0x100;
  REQUIRE (a.load () == (50 | 0x100));

  a &= 0xFF;
  REQUIRE (a.load () == 50);

  a ^= 0xFF;
  REQUIRE (a.load () == (50 ^ 0xFF));
}

TEST_CASE ("Atomic works with all supported widths [atomic]") {
  Atomic<uint8_t> u8 (200);
  ++u8;
  REQUIRE (u8.load () == 201);

  Atomic<uint16_t> u16 (65530);
  ++u16;
  REQUIRE (u16.load () == 65531);

  Atomic<uint32_t> u32 (0xFFFFFFFF);
  ++u32;
  REQUIRE (u32.load () == 0); // wraps

  Atomic<int64_t> i64 (-1);
  ++i64;
  REQUIRE (i64.load () == 0);

  Atomic<bool> flag (false);
  flag.store (true);
  REQUIRE (flag.load ());
  REQUIRE (flag.exchange (false));
  REQUIRE_FALSE (flag.load ());
}

// thread contention

TEST_CASE ("Atomic counter is contended safely between threads [atomic]") {
  Atomic<uint64_t> counter;
  Atomic<int> gate (0);

  const int N = 4;
  const int ITERS = 25000;

  Array<Thread> threads;
  for (int i = 0; i < N; ++i) {
    threads.emplace ([&counter, &gate] () {
      while (gate.load (MemoryOrder::acquire) == 0) {
      }

      for (int j = 0; j < ITERS; ++j) {
        ++counter;
      }
    });
  }

  gate.store (1, MemoryOrder::release);

  for (auto &t : threads) {
    t.join ();
  }

  REQUIRE (counter.load () == static_cast<uint64_t> (N) * ITERS);
}

TEST_CASE ("Atomic fetchSub is contended safely between threads [atomic]") {
  Atomic<int64_t> balance (4 * 10000);

  const int N = 4;

  Array<Thread> threads;
  for (int i = 0; i < N; ++i) {
    threads.emplace ([&balance] () {
      for (int j = 0; j < 10000; ++j) {
        balance.fetch_sub (1);
      }
    });
  }

  for (auto &t : threads) {
    t.join ();
  }

  REQUIRE (balance.load () == 0);
}
