// test_uniqueptr.cpp - tests for ystl/uniqueptr.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// helper: allocate + construct a single object via mem::
template <typename T, typename... Args> T *alloc_one (Args &&...args) {
  auto ptr = mem::allocate<T> ();
  return mem::construct (ptr, ystl::forward<Args> (args)...);
}

// helper: allocate + construct an array via mem::
template <typename T> T *alloc_array (size_t n) {
  return mem::allocate<T> (n);
}

// single-object uniqueptr
TEST_CASE ("UniquePtr default construction holds nullptr [uniqueptr]") {
  UniquePtr<int> p;
  REQUIRE_FALSE (bool (p));
  REQUIRE (p.get () == nullptr);
}

TEST_CASE ("UniquePtr construction from raw pointer holds value [uniqueptr]") {
  UniquePtr<int> p (alloc_one<int> (42));
  REQUIRE (bool (p));
  REQUIRE (p.get () != nullptr);
  REQUIRE (*p == 42);
}

TEST_CASE ("UniquePtr move constructor transfers ownership [uniqueptr]") {
  UniquePtr<int> a (alloc_one<int> (7));
  UniquePtr<int> b (ystl::move (a));

  REQUIRE (bool (b));
  REQUIRE (*b == 7);
  REQUIRE_FALSE (bool (a));
  REQUIRE (a.get () == nullptr);
}

TEST_CASE ("UniquePtr move assignment transfers ownership [uniqueptr]") {
  UniquePtr<int> a (alloc_one<int> (99));
  UniquePtr<int> b;
  b = ystl::move (a);

  REQUIRE (bool (b));
  REQUIRE (*b == 99);
  REQUIRE_FALSE (bool (a));
}

TEST_CASE ("UniquePtr nullptr assignment releases resource [uniqueptr]") {
  UniquePtr<int> p (alloc_one<int> (5));
  p = nullptr;
  REQUIRE_FALSE (bool (p));
  REQUIRE (p.get () == nullptr);
}

TEST_CASE ("UniquePtr release returns raw pointer and relinquishes ownership [uniqueptr]") {
  UniquePtr<int> p (alloc_one<int> (10));
  int *raw = p.release ();

  REQUIRE (raw != nullptr);
  REQUIRE (*raw == 10);
  REQUIRE_FALSE (bool (p));

  mem::destruct_and_release (raw);
}

TEST_CASE ("UniquePtr reset replaces the managed resource [uniqueptr]") {
  UniquePtr<int> p (alloc_one<int> (1));
  p.reset (alloc_one<int> (2));
  REQUIRE (*p == 2);

  p.reset ();
  REQUIRE_FALSE (bool (p));
}

TEST_CASE ("UniquePtr arrow operator accesses struct members [uniqueptr]") {
  struct Foo {
    int x { 55 };
  };
  UniquePtr<Foo> p (alloc_one<Foo> ());
  REQUIRE (p->x == 55);
  p->x = 77;
  REQUIRE ((*p).x == 77);
}

// array specialisation
TEST_CASE ("UniquePtr<T[]> default construction holds nullptr [uniqueptr]") {
  UniquePtr<int[]> p;
  REQUIRE_FALSE (bool (p));
}

TEST_CASE ("UniquePtr<T[]> supports element access [uniqueptr]") {
  auto raw = alloc_array<int> (3);
  raw[0] = 10;
  raw[1] = 20;
  raw[2] = 30;

  UniquePtr<int[]> p (raw, 3);
  REQUIRE (bool (p));
  REQUIRE (p[0] == 10);
  REQUIRE (p[1] == 20);
  REQUIRE (p[2] == 30);
}

TEST_CASE ("UniquePtr<T[]> move constructor transfers ownership [uniqueptr]") {
  auto raw = alloc_array<int> (2);
  raw[0] = 4;
  raw[1] = 5;

  UniquePtr<int[]> a (raw, 2);
  UniquePtr<int[]> b (ystl::move (a));

  REQUIRE (bool (b));
  REQUIRE (b[0] == 4);
  REQUIRE_FALSE (bool (a));
}

TEST_CASE ("UniquePtr<T[]> reset clears the array [uniqueptr]") {
  UniquePtr<int[]> p (alloc_array<int> (4), 4);
  p.reset ();
  REQUIRE_FALSE (bool (p));
}

// makeunique factory
TEST_CASE ("makeUnique creates a UniquePtr for a single object [uniqueptr]") {
  auto p = make_unique<int> (123);
  REQUIRE (bool (p));
  REQUIRE (*p == 123);
}

TEST_CASE ("makeUnique creates a UniquePtr for an array [uniqueptr]") {
  auto p = make_unique<int[]> (5u);
  REQUIRE (bool (p));
  // array is value-initialised to 0
  for (size_t i = 0; i < 5; ++i) {
    REQUIRE (p[i] == 0);
  }
}

TEST_CASE ("makeUnique with struct calls constructor [uniqueptr]") {
  struct Bar {
    int a, b;
    Bar (int a, int b) : a (a), b (b) {}
  };
  auto p = make_unique<Bar> (3, 7);
  REQUIRE (p->a == 3);
  REQUIRE (p->b == 7);
}

// custom deleters
TEST_CASE ("UniquePtr with function pointer deleter calls it on destruction [uniqueptr]") {
  static int freed_count;
  freed_count = 0;

  auto free_fn = [] (int *p) {
    ++freed_count;
    mem::destruct_and_release (p);
  };

  {
    UniquePtr<int, decltype (free_fn)> p (alloc_one<int> (1), free_fn);
    REQUIRE (*p == 1);
  }
  REQUIRE (freed_count == 1);
}

TEST_CASE ("UniquePtr with stateful lambda deleter [uniqueptr]") {
  int local_count = 0;

  auto counting_deleter = [&local_count] (int *p) {
    ++local_count;
    mem::destruct_and_release (p);
  };
  using Deleter = decltype (counting_deleter);

  {
    UniquePtr<int, Deleter> p (alloc_one<int> (42), counting_deleter);
    REQUIRE (*p == 42);
    REQUIRE (local_count == 0);
  }
  REQUIRE (local_count == 1);
}

TEST_CASE ("UniquePtr custom deleter called on reset [uniqueptr]") {
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif
  [[maybe_unused]] static int delete_count;
  delete_count = 0;

  struct TrackDelete {
    void operator() (int *p) const {
      ++delete_count;
      mem::destruct_and_release (p);
    }
  };

  UniquePtr<int, TrackDelete> p (alloc_one<int> (1));
  REQUIRE (delete_count == 0);

  p.reset (alloc_one<int> (2));
  REQUIRE (delete_count == 1);

  p.reset ();
  REQUIRE (delete_count == 2);
#if defined(__clang__)
  #pragma clang diagnostic pop
#endif
}

TEST_CASE ("UniquePtr custom deleter moved on move construction [uniqueptr]") {
  static int delete_count;
  delete_count = 0;

  struct CountingDeleter {
    int id;
    void operator() (int *p) const {
      ++delete_count;
      mem::destruct_and_release (p);
    }
  };

  {
    UniquePtr<int, CountingDeleter> a (alloc_one<int> (1), CountingDeleter { 42 });
    UniquePtr<int, CountingDeleter> b (ystl::move (a));

    REQUIRE (b.get_deleter ().id == 42);
    REQUIRE_FALSE (bool (a));
  }
  REQUIRE (delete_count == 1);
}

TEST_CASE ("UniquePtr custom deleter moved on move assignment [uniqueptr]") {
  static int delete_count;
  delete_count = 0;

  struct CountingDeleter {
    int id;
    void operator() (int *p) const {
      ++delete_count;
      mem::destruct_and_release (p);
    }
  };

  UniquePtr<int, CountingDeleter> a (alloc_one<int> (1), CountingDeleter { 7 });
  UniquePtr<int, CountingDeleter> b (alloc_one<int> (2), CountingDeleter { 0 });

  b = ystl::move (a);
  REQUIRE (b.get_deleter ().id == 7);
  REQUIRE (delete_count == 1); // old b's resource freed
}

TEST_CASE ("UniquePtr get_deleter returns the stored deleter [uniqueptr]") {
  struct MyDeleter {
    int tag = 0;
    void operator() (int *p) const {
      mem::destruct_and_release (p);
    }
  };

  UniquePtr<int, MyDeleter> p (alloc_one<int> (1), MyDeleter { 99 });
  REQUIRE (p.get_deleter ().tag == 99);

  p.get_deleter ().tag = 55;
  REQUIRE (p.get_deleter ().tag == 55);
}

TEST_CASE ("UniquePtr converting move with custom deleter [uniqueptr]") {
  struct Base {
    virtual ~Base () = default;
    int x = 1;
  };
  struct Derived : Base {
    int y = 2;
  };

  static int delete_count;
  delete_count = 0;

  struct Tracker {
    void operator() (Base *p) const {
      ++delete_count;
      mem::destruct_and_release (p);
    }
  };

  {
    UniquePtr<Derived, Tracker> d (alloc_one<Derived> (), Tracker {});
    UniquePtr<Base, Tracker> b (ystl::move (d));

    REQUIRE (bool (b));
    REQUIRE (b->x == 1);
    REQUIRE_FALSE (bool (d));
  }
  REQUIRE (delete_count == 1);
}

TEST_CASE ("UniquePtr with DefaultDelete explicit template works like default [uniqueptr]") {
  UniquePtr<int, DefaultDelete<int>> p (alloc_one<int> (42));
  REQUIRE (*p == 42);
  p.reset ();
  REQUIRE_FALSE (bool (p));
}

// array custom deleters
TEST_CASE ("UniquePtr<T[]> with custom deleter [uniqueptr]") {
  static int freed_count;
  freed_count = 0;

  auto free_arr = [] (int *p) {
    ++freed_count;
    mem::destruct_and_release_array (p, 3);
  };

  {
    auto raw = alloc_array<int> (3);
    raw[0] = 1;
    raw[1] = 2;
    raw[2] = 3;

    UniquePtr<int[], decltype (free_arr)> p (raw, free_arr);
    REQUIRE (p[0] == 1);
    REQUIRE (p[1] == 2);
    REQUIRE (p[2] == 3);
  }
  REQUIRE (freed_count == 1);
}

TEST_CASE ("UniquePtr<T[]> custom deleter moved on move [uniqueptr]") {
  static int delete_count;
  delete_count = 0;

  struct ArrDeleter {
    int id;
    void operator() (int *p) const {
      ++delete_count;
      mem::destruct_and_release_array (p, 2);
    }
  };

  auto raw = alloc_array<int> (2);
  raw[0] = 10;
  raw[1] = 20;

  UniquePtr<int[], ArrDeleter> a (raw, ArrDeleter { 5 });
  {
    UniquePtr<int[], ArrDeleter> b (ystl::move (a));

    REQUIRE (b.get_deleter ().id == 5);
    REQUIRE (b[0] == 10);
    REQUIRE_FALSE (bool (a));
  }
  REQUIRE (delete_count == 1);
}

TEST_CASE ("UniquePtr converting move assignment derived to base [uniqueptr]") {
  struct Base {
    virtual ~Base () = default;
    int x = 1;
  };
  struct Derived : Base {
    int y = 2;
  };

  UniquePtr<Derived> d (alloc_one<Derived> ());
  d->y = 9;

  UniquePtr<Base> b;
  b = ystl::move (d);

  REQUIRE (bool (b));
  REQUIRE (b->x == 1);
  REQUIRE_FALSE (bool (d));
}

TEST_CASE ("UniquePtr<T[]> converting move assignment from convertible deleter [uniqueptr]") {
  // regression: the converting operator= used reset(ptr), which does not exist for arrays
  struct ArrayDeleter : DefaultDelete<int[]> {
    using DefaultDelete<int[]>::DefaultDelete;
  };

  auto raw = alloc_array<int> (3);
  raw[0] = 1;
  raw[1] = 2;
  raw[2] = 3;

  UniquePtr<int[], ArrayDeleter> src (raw, 3);
  UniquePtr<int[]> dst (alloc_array<int> (1), 1);

  dst = ystl::move (src);

  REQUIRE (bool (dst));
  REQUIRE (dst[0] == 1);
  REQUIRE (dst[1] == 2);
  REQUIRE (dst[2] == 3);
  REQUIRE (dst.get_deleter ().size == 3); // deleter state carried over, not rebuilt
  REQUIRE_FALSE (bool (src));
}
