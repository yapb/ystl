// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/array.h>
#include <ystl/deque.h>
#include <ystl/lambda.h>
#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/uniqueptr.h>

#if !defined(YSTL_WINDOWS)
  #include <pthread.h>
  #include <sched.h>
  #include <errno.h>
#else
  #include <process.h>
#endif

namespace ystl {

// simple scoped lock wrapper
template <typename T> class ScopedLock final : public NonCopyable {
private:
  T &lockable_;

public:
  ScopedLock (T &lock) : lockable_ (lock) {
    lockable_.lock ();
  }

  ~ScopedLock () {
    lockable_.unlock ();
  }
};

// scoped unlock wrapper, releases on construction, reacquires on destruction
template <typename T> class ScopedUnlock final : public NonCopyable {
private:
  T &lockable_;

public:
  ScopedUnlock (T &lock) : lockable_ (lock) {
    lockable_.unlock ();
  }

  ~ScopedUnlock () {
    lockable_.lock ();
  }
};

// simple wrapper for critical sections
#if defined(YSTL_WINDOWS) && defined(YSTL_HAS_WINXP_SUPPORT)
class Mutex final : public NonCopyable {
private:
  CRITICAL_SECTION cs_;

public:
  Mutex () {
    InitializeCriticalSectionAndSpinCount (&cs_, 1);
  }
  ~Mutex () {
    DeleteCriticalSection (&cs_);
  }

  void lock () {
    EnterCriticalSection (&cs_);
  }

  void unlock () {
    LeaveCriticalSection (&cs_);
  }

  bool try_lock () {
    return !!TryEnterCriticalSection (&cs_);
  }

  decltype (auto) raw () {
    return &cs_;
  }
};

#elif defined(YSTL_WINDOWS)
class Mutex final : public NonCopyable {
private:
  SRWLOCK srw_ = SRWLOCK_INIT;

public:
  Mutex () = default;
  ~Mutex () = default;

  void lock () {
    AcquireSRWLockExclusive (&srw_);
  }

  void unlock () {
    ReleaseSRWLockExclusive (&srw_);
  }

  bool try_lock () {
    return !!TryAcquireSRWLockExclusive (&srw_);
  }

  decltype (auto) raw () {
    return &srw_;
  }
};

#else

class Mutex final : public NonCopyable {
private:
  pthread_mutex_t mutex_;

public:
  Mutex () {
    pthread_mutex_init (&mutex_, nullptr);
  }
  ~Mutex () {
    pthread_mutex_destroy (&mutex_);
  }

  void lock () {
    pthread_mutex_lock (&mutex_);
  }

  void unlock () {
    pthread_mutex_unlock (&mutex_);
  }

  bool try_lock () {
    return pthread_mutex_trylock (&mutex_) == 0;
  }

  decltype (auto) raw () {
    return &mutex_;
  }
};
#endif

// conditional variable (signal)
#if defined(YSTL_WINDOWS) && defined(YSTL_HAS_WINXP_SUPPORT)
class Signal final : public NonCopyable {
private:
  Mutex cs_;
  HANDLE event_ {};
  size_t waiters_ {};

public:
  Signal () {
    event_ = CreateEvent (nullptr, FALSE, FALSE, nullptr);
  }
  ~Signal () {
    CloseHandle (event_);
  }

  void lock () {
    cs_.lock ();
  }
  void unlock () {
    cs_.unlock ();
  }

  void notify () {
    SetEvent (event_);
  }

  void broadcast () {
    for (size_t i = 0; i < waiters_; ++i) {
      SetEvent (event_);
    }
  }

  template <typename T> bool wait (T timeout) {
    ++waiters_;
    unlock ();

    auto result = WaitForSingleObject (event_, timeout);
    lock ();

    --waiters_;

    return result == WAIT_OBJECT_0;
  }

  bool wait () {
    return wait (INFINITE);
  }
};

#elif defined(YSTL_WINDOWS)

class Signal final : public NonCopyable {
private:
  Mutex cs_;
  CONDITION_VARIABLE cv_;

public:
  Signal () {
    InitializeConditionVariable (&cv_);
  }
  ~Signal () = default;

  void lock () {
    cs_.lock ();
  }

  void unlock () {
    cs_.unlock ();
  }

  void notify () {
    WakeConditionVariable (&cv_);
  }

  void broadcast () {
    WakeAllConditionVariable (&cv_);
  }

  template <typename T> bool wait (T timeout) {
    return SleepConditionVariableSRW (&cv_, cs_.raw (), timeout, 0) != FALSE;
  }

  bool wait () {
    return wait (INFINITE);
  }
};

#else

class Signal final : public NonCopyable {
private:
  Mutex cs_;
  pthread_cond_t cv_;

public:
  Signal () {
    pthread_cond_init (&cv_, nullptr);
  }
  ~Signal () {
    pthread_cond_destroy (&cv_);
  }

  void lock () {
    cs_.lock ();
  }

  void unlock () {
    cs_.unlock ();
  }

  void notify () {
    pthread_cond_signal (&cv_);
  }

  void broadcast () {
    pthread_cond_broadcast (&cv_);
  }

  template <typename T> bool wait (T timeout) {
    struct timespec ts;

  #if defined(YSTL_POSIX) && !defined(YSTL_MACOS)
    if (clock_gettime (CLOCK_REALTIME, &ts) == -1) {
      return false;
    }
  #else
    struct timeval tv;
    gettimeofday (&tv, nullptr);

    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
  #endif

    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait (&cv_, cs_.raw (), &ts) == 0;
  }

  bool wait () {
    return pthread_cond_wait (&cv_, cs_.raw ()) == 0;
  }
};
#endif

using MutexScopedLock = ScopedLock<Mutex>;
using SignalScopedLock = ScopedLock<Signal>;

// helpers to name threads (best effort, for debuggers / profilers)
namespace detail {
constexpr size_t kMaxThreadName = 64;

#if defined(YSTL_WINDOWS)
YSTL_FORCE_INLINE bool set_windows_thread_name (HANDLE handle, const char *name) noexcept {
  if (!name || !*name || !handle) {
    return false;
  }
  // plain ascii expansion since windows headers lack nls support
  wchar_t wname[kMaxThreadName] {};
  for (size_t i = 0; i + 1 < kMaxThreadName && name[i] != '\0'; ++i) {
    wname[i] = static_cast<wchar_t> (static_cast<unsigned char> (name[i]));
  }

  // resolve setthreaddescription dynamically for older system support
  using SetThreadDescriptionFn = HRESULT (WINAPI *) (HANDLE, PCWSTR);
  static auto set_thread_description =
    reinterpret_cast<SetThreadDescriptionFn> (GetProcAddress (GetModuleHandleA ("kernel32.dll"), "SetThreadDescription"));

  if (!set_thread_description) {
    return false;
  }
  return SUCCEEDED (set_thread_description (handle, wname));
}
#endif

YSTL_FORCE_INLINE void set_current_thread_name (const char *name) noexcept {
  if (!name || !*name) {
    return;
  }
#if defined(YSTL_WINDOWS)
  if (set_windows_thread_name (GetCurrentThread (), name)) {
    return;
  }
  #if defined(_MSC_VER)
    // xp-era fallback: visible only to an attached debugger
    #pragma pack(push, 8)
  struct ThreadNameInfo {
    DWORD dwType = 0x1000;
    LPCSTR szName {};
    DWORD dwThreadID = static_cast<DWORD> (-1);
    DWORD dwFlags {};
  };
    #pragma pack(pop)

  ThreadNameInfo info {};
  info.szName = name;

  __try {
    RaiseException (0x406D1388, 0, sizeof (info) / sizeof (ULONG_PTR), reinterpret_cast<ULONG_PTR *> (&info));
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  #endif
#elif defined(YSTL_MACOS)
  pthread_setname_np (name);
#elif defined(YSTL_LINUX) || defined(YSTL_ANDROID)
  char buf[16] {};
  #if defined(__GNUC__) || defined(__clang__)
  _Pragma ("GCC diagnostic push") _Pragma ("GCC diagnostic ignored \"-Wformat-truncation\"")
  #endif
    snprintf (buf, sizeof (buf), "%s", name);
  #if defined(__GNUC__) || defined(__clang__)
  _Pragma ("GCC diagnostic pop")
  #endif
    pthread_setname_np (pthread_self (), buf);
#elif defined(YSTL_FREEBSD)
  pthread_set_name_np (pthread_self (), name);
#endif
}
} // namespace detail

// basic thread class
class Thread final : public NonCopyable {
public:
  using Func = Lambda<void ()>;

private:
  struct StartParam {
    Func func {};
    FixedArray<char, detail::kMaxThreadName> name {};
    bool has_name { false };
  };

private:
#if defined(YSTL_WINDOWS)
  HANDLE thread_ {};
#else
  bool initialized_ {};
  pthread_t thread_ {};
#endif
  UniquePtr<StartParam> invokable_;

private:
#if defined(YSTL_WINDOWS)
  static unsigned int __stdcall worker (void *pinvokable) {
    assert (pinvokable);
    UniquePtr<StartParam> ctx { reinterpret_cast<StartParam *> (pinvokable) };

    if (ctx->has_name) {
      detail::set_current_thread_name (ctx->name.data ());
    }
    ctx->func ();

    return 0;
  }
#else
  static void *worker (void *pinvokable) {
    assert (pinvokable);

    UniquePtr<StartParam> ctx { reinterpret_cast<StartParam *> (pinvokable) };

    if (ctx->has_name) {
      detail::set_current_thread_name (ctx->name.data ());
    }
    ctx->func ();

    return nullptr;
  }
#endif

private:
  static void copy_thread_name (StartParam *param, const char *name) {
    if (!param || !name || !*name) {
      return;
    }
    param->has_name = true;

    size_t i = 0;
    for (; i + 1 < param->name.capacity () && name[i] != '\0'; ++i) {
      param->name[i] = name[i];
    }
    param->name[i] = '\0';
  }

public:
  Thread () = default;

  explicit Thread (Func &&callback, const char *name = nullptr) {
    start (ystl::move (callback), name);
  }

  Thread (Thread &&rhs) noexcept {
    thread_ = rhs.thread_;
    invokable_ = ystl::move (rhs.invokable_);

#if defined(YSTL_WINDOWS)
    rhs.thread_ = nullptr;
#else
    initialized_ = rhs.initialized_;
    rhs.thread_ = 0;
    rhs.initialized_ = false;
#endif
  }

  Thread &operator= (Thread &&rhs) noexcept {
    if (this != &rhs) {
      join ();

      thread_ = rhs.thread_;
      invokable_ = ystl::move (rhs.invokable_);

#if defined(YSTL_WINDOWS)
      rhs.thread_ = nullptr;
#else
      initialized_ = rhs.initialized_;
      rhs.thread_ = 0;
      rhs.initialized_ = false;
#endif
    }
    return *this;
  }

  ~Thread () noexcept {
    join ();
  }

  void start (Func &&callback, const char *name = nullptr) {
    join ();

    invokable_ = make_unique<StartParam> ();
    invokable_->func = ystl::move (callback);
    copy_thread_name (invokable_.get (), name);

    StartParam *ptr = invokable_.get ();

#if defined(YSTL_WINDOWS)
    thread_ = reinterpret_cast<HANDLE> (_beginthreadex (nullptr, 0, worker, ptr, 0, nullptr));
#else
    initialized_ = (pthread_create (&thread_, nullptr, worker, ptr) == 0);
#endif

    if (ok ()) {
      static_cast<void> (invokable_.release ()); // ownership passes to the thread
    }
    else {
      invokable_.reset ();
    }
  }

  // names an already running thread
  bool set_name ([[maybe_unused]] const char *name) {
    if (!name || !*name || !ok ()) {
      return false;
    }
#if defined(YSTL_WINDOWS)
    return detail::set_windows_thread_name (thread_, name);
#elif defined(YSTL_LINUX) || defined(YSTL_ANDROID)
    char buf[16] {};
    strncpy (buf, name, sizeof (buf) - 1);
    return pthread_setname_np (thread_, buf) == 0;
#elif defined(YSTL_MACOS)
    if (!pthread_equal (thread_, pthread_self ())) {
      return false;
    }
    return pthread_setname_np (name) == 0;
#elif defined(YSTL_FREEBSD)
    pthread_set_name_np (thread_, name);
    return true;
#else
    return false;
#endif
  }

public:
  bool ok () const {
#if defined(YSTL_WINDOWS)
    return !!thread_;
#else
    return initialized_;
#endif
  }

  void join () {
    if (!ok ()) {
      return;
    }
#if defined(YSTL_WINDOWS)
    WaitForSingleObjectEx (thread_, INFINITE, FALSE);
    CloseHandle (thread_);
    thread_ = nullptr;
#else
    pthread_join (thread_, nullptr);
    initialized_ = false;
#endif
  }

  void detach () {
    if (!ok ()) {
      return;
    }
#if defined(YSTL_WINDOWS)
    CloseHandle (thread_);
    thread_ = nullptr;
#else
    pthread_detach (thread_);
    initialized_ = false;
#endif
  }

  decltype (auto) handle () const {
    return thread_;
  }

  bool joinable () const {
    return ok ();
  }
};

// extra simple thread pool
class ThreadPool final : public NonCopyable {
private:
  using Func = Thread::Func;

private:
  bool running_ { false };
  mutable Signal signal_ {};

  Deque<Func> jobs_ {};
  Array<Thread> threads_;

public:
  explicit ThreadPool (size_t workers = 0, const char *name = nullptr) noexcept {
    if (workers > 0) {
      startup (workers, name);
    }
  }

  ~ThreadPool () {
    shutdown ();
  }

public:
  size_t jobs () noexcept {
    SignalScopedLock lock (signal_);
    return jobs_.size ();
  }

  size_t thread_count () noexcept {
    return threads_.size ();
  }

public:
  void enqueue (Func &&task) {
    SignalScopedLock lock (signal_);

    jobs_.emplace_last (ystl::move (task));
    signal_.notify ();
  }

  void shutdown () {
    {
      SignalScopedLock lock (signal_);
      running_ = false;
      signal_.broadcast ();
    }

    for (auto &thread : threads_) {
      thread.join ();
    }
    threads_.clear ();
  }

  void startup (size_t workers, const char *name = nullptr) {
    {
      SignalScopedLock lock (signal_);
      running_ = true;
    }

    for (size_t i = 0; i < workers; ++i) {
      char buf[detail::kMaxThreadName] {};

      const char *thread_name = nullptr;
      if (name && *name) {
        snprintf (buf, sizeof (buf), "%s-%zu", name, i);
        thread_name = buf;
      }
      threads_.emplace (
        [this] () {
          for (;;) {
            Func job {};
            {
              SignalScopedLock lock (signal_);

              while (running_ && jobs_.empty ()) {
                signal_.wait ();
              }

              if (!running_ && jobs_.empty ()) {
                return;
              }
              job = ystl::move (jobs_.pop_front ());
            }
            job ();
          }
        },
        thread_name);
    }
  }
};

// helpers for the calling thread
namespace ThisThread {
YSTL_FORCE_INLINE void yield () noexcept {
#if defined(YSTL_WINDOWS)
  SwitchToThread ();
#else
  sched_yield ();
#endif
}

YSTL_FORCE_INLINE void sleep (unsigned ms) noexcept {
#if defined(YSTL_WINDOWS)
  Sleep (ms);
#else
  struct timespec ts {};

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;

  while (nanosleep (&ts, &ts) == -1 && errno == EINTR) {
  }
#endif
}

// names the calling thread (best effort, for debuggers / profilers)
YSTL_FORCE_INLINE void set_name (const char *name) noexcept {
  detail::set_current_thread_name (name);
}
}

}
