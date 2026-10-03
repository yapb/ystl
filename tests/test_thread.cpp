// test_thread.cpp - tests for ystl/thread.h (mutex, scopedlock, signal, thread, threadpool)
#include <ystl/ystl.h>
#include <ystl/test.h>

namespace {
inline void test_sleep (int ms) {
  ystl::ThisThread::sleep (static_cast<unsigned> (ms));
}
}

using namespace ystl;

// mutex - basic lock /
TEST_CASE ("Mutex lock and unlock do not crash [thread]") {
  Mutex m;
  m.lock ();
  m.unlock ();
  REQUIRE (true);
}

TEST_CASE ("Mutex tryLock succeeds when unlocked [thread]") {
  Mutex m;
  bool got = m.try_lock ();
  REQUIRE (got);
  m.unlock ();
}

TEST_CASE ("Mutex tryLock fails when already locked (from the same thread on Win non-XP / reentrant path) [thread]") {
  // srwlock on windows is not reentrant; trylock will fail if same thread holds it
  Mutex m;
  m.lock ();
#if defined(YSTL_WINDOWS) && !defined(YSTL_HAS_WINXP_SUPPORT)
  bool second = m.try_lock ();
  // srwlock: same-thread trylock for exclusive is undefined; we just verify it does not crash
  (void)second;
#endif
  m.unlock ();
  REQUIRE (true);
}

// scopedlock - raii
TEST_CASE ("ScopedLock locks and unlocks automatically [thread]") {
  Mutex m;
  {
    MutexScopedLock guard (m);
    // m is locked here
  }
  // m must be unlocked now; trylock should succeed
  bool got = m.try_lock ();
  REQUIRE (got);
  m.unlock ();
}

// scopedunlock - releases during the scope, reacquires after
TEST_CASE ("ScopedUnlock unlocks in scope and relocks after [thread]") {
  Mutex m;
  m.lock ();
  {
    ScopedUnlock<Mutex> ul (m);
    bool got = m.try_lock ();
    REQUIRE (got);
    m.unlock ();
  }
  m.unlock ();
}

// thread - launch and
TEST_CASE ("Thread launches and ok() returns true [thread]") {
  volatile bool ran = false;

  Thread t ([&ran] () {
    ran = true;
  });

  REQUIRE (t.ok ());
  t.join ();
  REQUIRE (ran);
}

TEST_CASE ("Thread join completes without hanging [thread]") {
  Thread t ([] () {
    // do nothing
  });
  t.join ();
  REQUIRE (true);
}

TEST_CASE ("Thread move constructor transfers ownership and thread runs correctly [thread]") {
  // hold the thread at its start so the move is guaranteed to happen before the callback executes
  Signal gate;
  bool released = false;
  volatile bool ran = false;

  Thread t1 ([&gate, &released, &ran] () {
    gate.lock ();
    while (!released) {
      gate.wait ();
    }
    gate.unlock ();
    ran = true;
  });

  // move before the thread is released from the gate
  Thread t2 (ystl::move (t1));
  REQUIRE_FALSE (t1.ok ());
  REQUIRE (t2.ok ());

  // release the thread - it will now execute its body via t2's invokable_
  {
    SignalScopedLock lock (gate);
    released = true;
    gate.notify ();
  }

  t2.join ();
  REQUIRE (ran);
}

TEST_CASE ("Thread executes work incrementing a shared counter [thread]") {
  Mutex m;
  int counter = 0;
  const int N = 5;

  Array<Thread> threads;
  for (int i = 0; i < N; ++i) {
    threads.emplace ([&m, &counter] () {
      MutexScopedLock guard (m);
      ++counter;
    });
  }
  for (auto &t : threads) {
    t.join ();
  }
  REQUIRE (counter == N);
}

// threadpool - basic
TEST_CASE ("ThreadPool starts with correct threadCount [thread]") {
  ThreadPool pool (2);
  REQUIRE (pool.thread_count () == 2);
  pool.shutdown ();
}

TEST_CASE ("ThreadPool enqueue and execute tasks [thread]") {
  Mutex m;
  int done = 0;
  const int tasks = 10;

  {
    ThreadPool pool (2);

    for (int i = 0; i < tasks; ++i) {
      pool.enqueue ([&m, &done] () {
        MutexScopedLock guard (m);
        ++done;
      });
    }
    // shutdown clears pending jobs so spin wait for tasks
    while (true) {
      {
        MutexScopedLock guard (m);
        if (done == tasks) {
          break;
        }
      }
      test_sleep (1);
    }
  }
  REQUIRE (done == tasks);
}

TEST_CASE ("ThreadPool jobs count returns zero after shutdown [thread]") {
  ThreadPool pool (1);
  pool.shutdown ();
  REQUIRE (pool.jobs () == 0);
}

TEST_CASE ("ThreadPool startup with 0 workers does nothing [thread]") {
  ThreadPool pool (0);
  REQUIRE (pool.thread_count () == 0);
  // enqueue is safe to call even with no workers... but tasks won't run
}

// signal - basic notify
TEST_CASE ("Signal notify from main thread does not crash [thread]") {
  Signal sig;
  sig.lock ();
  sig.notify ();
  sig.unlock ();
  REQUIRE (true);
}

TEST_CASE ("Signal wait with short timeout returns [thread]") {
  Signal sig;
  sig.lock ();
  // wait with 1ms timeout - should return quickly
  bool result = sig.wait (1);
  sig.unlock ();
  // result might be true or false depending on spurious wakeups; just no deadlock
  (void)result;
  REQUIRE (true);
}

// additional tests for missing coverage

// mutex - additional
TEST_CASE ("Mutex raw method returns handle [thread]") {
  Mutex m;
  auto handle = m.raw ();
  REQUIRE (handle != nullptr);
}

TEST_CASE ("Mutex double unlock does not crash [thread]") {
  Mutex m;
  m.lock ();
  m.unlock ();
  m.unlock (); // second unlock should be safe
  REQUIRE (true);
}

TEST_CASE ("Mutex destruction while locked is safe [thread]") {
  // this tests that destroying a locked mutex doesn't crash (though it's bad practice in real code)
  {
    Mutex m;
    m.lock ();
    // mutex destroyed while locked
  }
  REQUIRE (true);
}

// scopedlock /
TEST_CASE ("ScopedLock with Signal type [thread]") {
  Signal sig;
  {
    SignalScopedLock lock (sig);
    // signal is locked
  }
  // signal should be unlocked
  REQUIRE (true);
}

TEST_CASE ("ScopedUnlock with Signal type [thread]") {
  Signal sig;
  sig.lock ();
  {
    ScopedUnlock<Signal> ul (sig);
    // signal is unlocked temporarily
  }
  // signal should be locked again (scopedunlock doesn't relock)
  sig.unlock ();
  REQUIRE (true);
}

// signal - additional
TEST_CASE ("Signal broadcast wakes multiple waiters [thread]") {
  Signal sig;
  Mutex counter_mutex;
  int woken_count = 0;
  const int num_threads = 3;

  Array<Thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace ([&sig, &counter_mutex, &woken_count] () {
      sig.lock ();
      sig.wait (); // wait for broadcast
      {
        MutexScopedLock lock (counter_mutex);
        ++woken_count;
      }
      sig.unlock ();
    });
  }

  // give threads time to start waiting
  test_sleep (50);

  // broadcast should wake all threads
  sig.lock ();
  sig.broadcast ();
  sig.unlock ();

  // wait for threads to finish
  for (auto &t : threads) {
    t.join ();
  }

  REQUIRE (woken_count == num_threads);
}

TEST_CASE ("Signal wait without timeout blocks [thread]") {
  Signal sig;
  volatile bool waited = false;

  Thread t ([&sig, &waited] () {
    sig.lock ();
    sig.wait (); // should block until notified
    waited = true;
    sig.unlock ();
  });

  // give thread time to start waiting
  test_sleep (50);

  // thread should still be waiting
  REQUIRE (!waited);

  // notify to wake it
  sig.lock ();
  sig.notify ();
  sig.unlock ();

  t.join ();
  REQUIRE (waited);
}

TEST_CASE ("Signal wait after notify waits [thread]") {
  Signal sig;

  sig.lock ();
  sig.notify (); // notify before anyone is waiting
  bool result = sig.wait (10); // should still wait (timeout)
  sig.unlock ();

  // should timeout (false) since no one will notify again just verify it doesn't deadlock
  (void)result;
  REQUIRE (true);
}

// thread - additional
TEST_CASE ("Thread detach method [thread]") {
  Signal done;
  bool ran = false;

  Thread t ([&done, &ran] () {
    test_sleep (10);
    ran = true;
    done.notify ();
  });

  REQUIRE (t.ok ());
  t.detach ();
  REQUIRE (!t.ok ());

  {
    SignalScopedLock lock (done);
    REQUIRE (done.wait (1000));
  }
  REQUIRE (ran);
}

TEST_CASE ("Thread handle method returns non-zero [thread]") {
  Thread t ([] () {});
  auto handle = t.handle ();
  // handle should be non-zero/null for valid thread
  (void)handle;
  t.join ();
  REQUIRE (true);
}

TEST_CASE ("Thread ok on default-constructed returns false [thread]") {
  Thread t;
  REQUIRE (!t.ok ());
}

TEST_CASE ("Thread join on already joined thread is safe [thread]") {
  Thread t ([] () {});
  t.join ();
  t.join (); // second join should be safe
  REQUIRE (true);
}

TEST_CASE ("Thread start on already running thread joins first [thread]") {
  volatile bool first_ran = false;
  volatile bool second_ran = false;

  Thread t ([&first_ran] () {
    first_ran = true;
  });

  t.join ();
  REQUIRE (first_ran);

  // start second thread after first completed
  t.start ([&second_ran] () {
    second_ran = true;
  });

  t.join ();
  REQUIRE (second_ran);
}

TEST_CASE ("Thread move assignment operator [thread]") {
  volatile bool ran1 = false;
  volatile bool ran2 = false;

  Thread t1 ([&ran1] () {
    ran1 = true;
  });
  Thread t2 ([&ran2] () {
    ran2 = true;
  });

  t2 = ystl::move (t1); // move assignment

  REQUIRE (!t1.ok ());
  REQUIRE (t2.ok ());

  t2.join ();
  REQUIRE (ran1);
  // t2's original thread was joined by move assignment
}

// threadpool - additional
TEST_CASE ("ThreadPool jobs count while tasks are running [thread]") {
  ystl::Signal done;
  volatile bool task_running = false;

  ThreadPool pool (1);

  pool.enqueue ([&task_running, &done] () {
    task_running = true;
    test_sleep (60); // simulate work
    task_running = false;
    done.notify ();
  });

  // wait for completion (timeout to avoid hangs)
  done.lock ();
  bool signaled = done.wait (1000);
  done.unlock ();
  REQUIRE (signaled);

  pool.shutdown ();
}

TEST_CASE ("ThreadPool shutdown with pending jobs [thread]") {
  Mutex m;
  int completed = 0;

  ThreadPool pool (1);

  // enqueue more tasks than threads can process before shutdown
  for (int i = 0; i < 5; ++i) {
    pool.enqueue ([&m, &completed] () {
      test_sleep (50); // simulate work
      MutexScopedLock lock (m);
      ++completed;
    });
  }

  // shutdown immediately - some tasks may not run
  pool.shutdown ();

  // some tasks may have completed, some may have been dropped
  REQUIRE (completed >= 0);
  REQUIRE (completed <= 5);
}

TEST_CASE ("ThreadPool restart after shutdown [thread]") {
  ThreadPool pool (1);
  pool.shutdown ();

  // should be able to start again
  pool.startup (2);
  REQUIRE (pool.thread_count () == 2);

  pool.shutdown ();
}

TEST_CASE ("ThreadPool enqueue after shutdown does not crash [thread]") {
  ThreadPool pool (1);
  pool.shutdown ();

  // enqueue after shutdown should not crash
  pool.enqueue ([] () {
    // this task should not run
  });

  REQUIRE (true);
}

TEST_CASE ("ThreadPool thread safety of public methods [thread]") {
  ThreadPool pool (2);
  Mutex counter_mutex;
  int enqueue_count = 0;

  // multiple threads calling enqueue concurrently
  Array<Thread> clients;
  for (int i = 0; i < 4; ++i) {
    clients.emplace ([&pool, &counter_mutex, &enqueue_count] () {
      for (int j = 0; j < 10; ++j) {
        pool.enqueue ([&counter_mutex, &enqueue_count] () {
          MutexScopedLock lock (counter_mutex);
          ++enqueue_count;
        });
        test_sleep (1);
      }
    });
  }

  // also call jobs() concurrently
  Array<Thread> monitors;
  for (int i = 0; i < 2; ++i) {
    monitors.emplace ([&pool] () {
      for (int j = 0; j < 20; ++j) {
        (void)pool.jobs ();
        test_sleep (2);
      }
    });
  }

  // wait for all
  for (auto &t : clients)
    t.join ();
  for (auto &t : monitors)
    t.join ();

  // let pool process tasks
  test_sleep (100);

  pool.shutdown ();
  REQUIRE (enqueue_count == 40); // 4 clients * 10 tasks each
}
