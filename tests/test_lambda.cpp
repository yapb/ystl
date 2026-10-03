// test_lambda.cpp - tests for ystl/lambda.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// construction / bool
TEST_CASE ("Lambda default construction holds nothing [lambda]") {
  Lambda<int ()> fn;
  REQUIRE_FALSE (bool (fn));
}

TEST_CASE ("Lambda nullptr construction holds nothing [lambda]") {
  Lambda<int ()> fn (nullptr);
  REQUIRE_FALSE (bool (fn));
}

TEST_CASE ("Lambda from callable is valid [lambda]") {
  Lambda<int ()> fn ([] {
    return 42;
  });
  REQUIRE (bool (fn));
}

// invocation
TEST_CASE ("Lambda invokes a stateless lambda [lambda]") {
  Lambda<int ()> fn ([] {
    return 99;
  });
  REQUIRE (fn () == 99);
}

TEST_CASE ("Lambda invokes a lambda capturing a variable [lambda]") {
  int x = 7;
  Lambda<int ()> fn ([x] {
    return x * 2;
  });
  REQUIRE (fn () == 14);
}

TEST_CASE ("Lambda with parameters passes arguments correctly [lambda]") {
  Lambda<int (int, int)> add ([] (int a, int b) {
    return a + b;
  });
  REQUIRE (add (3, 4) == 7);
  REQUIRE (add (0, 0) == 0);
}

TEST_CASE ("Lambda with void return type executes side effects [lambda]") {
  int counter = 0;
  Lambda<void ()> inc ([&counter] {
    ++counter;
  });
  inc ();
  inc ();
  inc ();
  REQUIRE (counter == 3);
}

TEST_CASE ("Lambda captures mutable state via reference [lambda]") {
  int val = 0;
  Lambda<void (int)> set ([&val] (int v) {
    val = v;
  });
  set (100);
  REQUIRE (val == 100);
}

// copy construction and
TEST_CASE ("Lambda copy construction produces independent callable [lambda]") {
  Lambda<int ()> original ([] {
    return 42;
  });
  Lambda<int ()> copy (original);

  REQUIRE (bool (copy));
  REQUIRE (copy () == 42);
  REQUIRE (original () == 42); // original still works
}

TEST_CASE ("Lambda copy assignment [lambda]") {
  Lambda<int ()> a ([] {
    return 10;
  });
  Lambda<int ()> b;
  b = a;
  REQUIRE (bool (b));
  REQUIRE (b () == 10);
}

// move construction and
TEST_CASE ("Lambda move construction transfers callable [lambda]") {
  Lambda<int ()> a ([] {
    return 55;
  });
  Lambda<int ()> b (ystl::move (a));

  REQUIRE (bool (b));
  REQUIRE (b () == 55);
  REQUIRE_FALSE (bool (a));
}

TEST_CASE ("Lambda move assignment transfers callable [lambda]") {
  Lambda<int ()> a ([] {
    return 77;
  });
  Lambda<int ()> b;
  b = ystl::move (a);

  REQUIRE (bool (b));
  REQUIRE (b () == 77);
  REQUIRE_FALSE (bool (a));
}

// nullptr assignment
TEST_CASE ("Lambda nullptr assignment releases callable [lambda]") {
  Lambda<int ()> fn ([] {
    return 1;
  });
  fn = nullptr;
  REQUIRE_FALSE (bool (fn));
}

// callable replacement
TEST_CASE ("Lambda replacement with different callable [lambda]") {
  Lambda<int ()> fn ([] {
    return 1;
  });
  REQUIRE (fn () == 1);

  fn = [] {
    return 2;
  };
  REQUIRE (fn () == 2);
}

// small-buffer
TEST_CASE ("Lambda with large capture (heap allocation) works correctly [lambda]") {
  // force heap allocation by capturing a large object
  struct BigCapture {
    int data[64] {};
    int value { 123 };
  } big;
  big.value = 456;

  Lambda<int ()> fn ([big] {
    return big.value;
  });
  REQUIRE (fn () == 456);
}

// function pointer
static int static_func (int a, int b) {
  return a * b;
}

TEST_CASE ("Lambda from function pointer [lambda]") {
  Lambda<int (int, int)> fn (static_func);
  REQUIRE (fn (3, 7) == 21);
}

// self-assignment
TEST_CASE ("Lambda copy self-assignment leaves callable valid [lambda]") {
  Lambda<int ()> fn ([] {
    return 7;
  });
  Lambda<int ()> &alias = fn;
  alias = fn; // self-copy via reference alias
  REQUIRE (bool (fn));
  REQUIRE (fn () == 7);
}

TEST_CASE ("Lambda move self-assignment does not crash [lambda]") {
  Lambda<int ()> fn ([] {
    return 8;
  });
  Lambda<int ()> &alias = fn;
  alias = ystl::move (fn); // self-move via reference alias
  // post-self-move state is valid but unspecified; just must not crash
  REQUIRE_NOTHROW ((void)bool (fn));
}

// operator= forwards
TEST_CASE ("Lambda assignment from rvalue functor moves it [lambda]") {
  bool moved = false;

  struct MoveTracker {
    bool *moved_;
    int value;
    MoveTracker (bool *m, int v) : moved_ (m), value (v) {}
    MoveTracker (const MoveTracker &o) : moved_ (o.moved_), value (o.value) {}
    MoveTracker (MoveTracker &&o) noexcept : moved_ (o.moved_), value (o.value) {
      *moved_ = true;
    }
    int operator() () const {
      return value;
    }
  };

  Lambda<int ()> fn;
  fn = MoveTracker { &moved, 42 }; // rvalue - must move, not copy
  REQUIRE (fn () == 42);
  REQUIRE (moved); // fails if apply(rhs) used instead of apply(forward<u>(rhs))
}

// large capture copy
TEST_CASE ("Lambda copy construction of heap-allocated lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 789;

  Lambda<int ()> original ([big] {
    return big.value;
  });
  Lambda<int ()> copy (original);

  REQUIRE (bool (copy));
  REQUIRE (copy () == 789);
  REQUIRE (original () == 789);
}

TEST_CASE ("Lambda copy assignment of heap-allocated lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 321;

  Lambda<int ()> original ([big] {
    return big.value;
  });
  Lambda<int ()> copy;
  copy = original;

  REQUIRE (bool (copy));
  REQUIRE (copy () == 321);
  REQUIRE (original () == 321);
}

// large capture move
TEST_CASE ("Lambda move construction of heap-allocated lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 555;

  Lambda<int ()> original ([big] {
    return big.value;
  });
  Lambda<int ()> moved (ystl::move (original));

  REQUIRE (bool (moved));
  REQUIRE (moved () == 555);
  REQUIRE_FALSE (bool (original));
}

TEST_CASE ("Lambda move assignment of heap-allocated lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 666;

  Lambda<int ()> original ([big] {
    return big.value;
  });
  Lambda<int ()> moved;
  moved = ystl::move (original);

  REQUIRE (bool (moved));
  REQUIRE (moved () == 666);
  REQUIRE_FALSE (bool (original));
}

// copy/move assignment
TEST_CASE ("Lambda copy assignment from empty lambda [lambda]") {
  Lambda<int ()> fn ([] {
    return 42;
  });
  Lambda<int ()> empty;

  fn = empty;
  REQUIRE_FALSE (bool (fn));
}

TEST_CASE ("Lambda move assignment from empty lambda [lambda]") {
  Lambda<int ()> fn ([] {
    return 42;
  });
  Lambda<int ()> empty;

  fn = ystl::move (empty);
  REQUIRE_FALSE (bool (fn));
}

// copy/move from sbo to
TEST_CASE ("Lambda copy assignment replaces heap with SBO lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 100;

  Lambda<int ()> heap_lambda ([big] {
    return big.value;
  });
  Lambda<int ()> sbo_lambda ([] {
    return 200;
  });

  heap_lambda = sbo_lambda;
  REQUIRE (heap_lambda () == 200);
}

TEST_CASE ("Lambda move assignment replaces heap with SBO lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 100;

  Lambda<int ()> heap_lambda ([big] {
    return big.value;
  });
  Lambda<int ()> sbo_lambda ([] {
    return 300;
  });

  heap_lambda = ystl::move (sbo_lambda);
  REQUIRE (heap_lambda () == 300);
  REQUIRE_FALSE (bool (sbo_lambda));
}

// copy/move from heap to
TEST_CASE ("Lambda copy assignment replaces SBO with heap lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 400;

  Lambda<int ()> sbo_lambda ([] {
    return 50;
  });
  Lambda<int ()> heap_lambda ([big] {
    return big.value;
  });

  sbo_lambda = heap_lambda;
  REQUIRE (sbo_lambda () == 400);
}

TEST_CASE ("Lambda move assignment replaces SBO with heap lambda [lambda]") {
  struct BigCapture {
    int data[64] {};
    int value { 0 };
  } big;
  big.value = 500;

  Lambda<int ()> sbo_lambda ([] {
    return 50;
  });
  Lambda<int ()> heap_lambda ([big] {
    return big.value;
  });

  sbo_lambda = ystl::move (heap_lambda);
  REQUIRE (sbo_lambda () == 500);
  REQUIRE_FALSE (bool (heap_lambda));
}
