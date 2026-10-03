// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/memory.h>

namespace ystl {

template <typename> class Lambda;
template <typename R, typename... Args> class Lambda<R (Args...)> final {
private:
  // natural max alignment; over-aligned callables fall back to the heap
  struct MaxAlign {
    void *ptr_ {};
    double dbl_ {};
    long double ldbl_ {};
    void (*fn_) () {};
  };

  static constexpr size_t kSmallBufferSize = sizeof (void *) * 4;

  // sbo buffer carrying the maxalign alignment
  struct alignas (MaxAlign) SboBuffer {
    uint8_t bytes[kSmallBufferSize] {};
  };

  static_assert (alignof (SboBuffer) == alignof (MaxAlign));
  static_assert (sizeof (SboBuffer) == kSmallBufferSize);

  // type-erased callable interface; invoke is non-const so mutable lambdas work
  class Holder {
  public:
    Holder () = default;
    virtual ~Holder () = default;

    Holder (const Holder &) = delete;
    Holder &operator= (const Holder &) = delete;

  public:
    virtual R invoke (Args &&...args) = 0;
    virtual Holder *clone_into (void *buf) const = 0;
    virtual Holder *move_into (void *buf) noexcept = 0;
  };

  template <typename T> class CallableHolder final : public Holder {
  private:
    T callee_;

  public:
    explicit CallableHolder (const T &callee) : callee_ (callee) {}
    explicit CallableHolder (T &&callee) : callee_ (ystl::move (callee)) {}

    CallableHolder (const CallableHolder &rhs) : callee_ (rhs.callee_) {}
    CallableHolder (CallableHolder &&rhs) noexcept : callee_ (ystl::move (rhs.callee_)) {}

  public:
    R invoke (Args &&...args) override {
      return callee_ (ystl::forward<Args> (args)...);
    }

    Holder *clone_into (void *buf) const override {
      if (buf) {
        return mem::construct (static_cast<CallableHolder *> (buf), *this);
      }
      return mem::allocate_and_construct<CallableHolder> (*this);
    }

    Holder *move_into (void *buf) noexcept override {
      if (buf) {
        return mem::construct (static_cast<CallableHolder *> (buf), ystl::move (*this));
      }
      return mem::allocate_and_construct<CallableHolder> (ystl::move (*this));
    }
  };

private:
  Holder *holder_ {};
  bool small_ {};

  // padding rounding holder_ plus small_ up to sbobuffer alignment
  uint8_t reserved_[sizeof (void *) - sizeof (bool)] {};
  SboBuffer storage_ {};

private:
  void *small_buffer () noexcept {
    return static_cast<void *> (storage_.bytes);
  }

  void destroy_holder () noexcept {
    if (!holder_) {
      return;
    }

    if (small_) {
      holder_->~Holder ();
    }
    else {
      mem::destruct_and_release (holder_);
    }
    holder_ = nullptr;
    small_ = false;
  }

  template <typename U> void assign_callable (U &&fn) {
    using Callable = CallableHolder<typename ystl::decay<U>::type>;
    constexpr bool fits_sbo = sizeof (Callable) <= sizeof (storage_) && alignof (Callable) <= alignof (decltype (storage_));

    if constexpr (fits_sbo) {
      holder_ = mem::construct (static_cast<Callable *> (small_buffer ()), ystl::forward<U> (fn));
      small_ = true;
    }
    else {
      holder_ = mem::allocate_and_construct<Callable> (ystl::forward<U> (fn));
      small_ = false;
    }
  }

  void copy_from (const Lambda &rhs) {
    if (!rhs.holder_) {
      return;
    }

    if (rhs.small_) {
      holder_ = rhs.holder_->clone_into (small_buffer ());
      small_ = true;
    }
    else {
      holder_ = rhs.holder_->clone_into (nullptr);
      small_ = false;
    }
  }

  void move_from (Lambda &&rhs) noexcept {
    if (!rhs.holder_) {
      return;
    }

    if (rhs.small_) {
      holder_ = rhs.holder_->move_into (small_buffer ());
      small_ = true;

      // destruct the moved-from object still alive in rhs small buffer
      rhs.holder_->~Holder ();
      rhs.holder_ = nullptr;
      rhs.small_ = false;
    }
    else {
      // heap-allocated, just steal the pointer
      holder_ = rhs.holder_;
      small_ = false;

      rhs.holder_ = nullptr;
    }
  }

public:
  Lambda () noexcept = default;
  Lambda (nullptr_t) noexcept {}

  Lambda (const Lambda &rhs) {
    copy_from (rhs);
  }

  Lambda (Lambda &&rhs) noexcept {
    move_from (ystl::move (rhs));
  }

  template <typename U>
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, Lambda> && ystl::is_invocable_r_v<R, U, Args...>)
  Lambda (U &&obj) {
    assign_callable (ystl::forward<U> (obj));
  }

  ~Lambda () {
    destroy_holder ();
  }

public:
  void swap (Lambda &other) noexcept {
    if (this == &other) {
      return;
    }

    if (!holder_ && !other.holder_) {
      return;
    }

    if (small_ && holder_ && other.small_ && other.holder_) {
      // both small: rotate through a stack temp
      alignas (MaxAlign) uint8_t tmp[sizeof (storage_)];
      Holder *tmph = other.holder_->move_into (tmp);
      other.holder_->~Holder ();

      other.holder_ = holder_->move_into (other.small_buffer ());
      holder_->~Holder ();

      holder_ = tmph->move_into (small_buffer ());
      tmph->~Holder ();
    }
    else if (small_ && holder_ && !other.holder_) {
      other.holder_ = holder_->move_into (other.small_buffer ());
      other.small_ = true;

      holder_->~Holder ();
      holder_ = nullptr;
      small_ = false;
    }
    else if (!holder_ && other.small_ && other.holder_) {
      holder_ = other.holder_->move_into (small_buffer ());
      small_ = true;

      other.holder_->~Holder ();
      other.holder_ = nullptr;
      other.small_ = false;
    }
    else if (small_ && holder_ && !other.small_ && other.holder_) {
      alignas (MaxAlign) uint8_t tmp[sizeof (storage_)];
      Holder *tmph = holder_->move_into (tmp);
      holder_->~Holder ();

      holder_ = other.holder_;
      small_ = false;

      other.holder_ = tmph->move_into (other.small_buffer ());
      other.small_ = true;
      tmph->~Holder ();
    }
    else if (!small_ && holder_ && other.small_ && other.holder_) {
      other.swap (*this);
    }
    else {
      // both heap (either side may be empty): just steal pointers
      ystl::swap (holder_, other.holder_);
      ystl::swap (small_, other.small_);
    }
  }

  friend void swap (Lambda &left, Lambda &right) noexcept {
    left.swap (right);
  }

public:
  explicit operator bool () const noexcept {
    return !!holder_;
  }

  bool operator== (nullptr_t) const noexcept {
    return !holder_;
  }

  bool operator!= (nullptr_t) const noexcept {
    return !!holder_;
  }

  R operator() (Args... args) const {
    if (!holder_) [[unlikely]] {
      plat.abort ("lambda: call of an empty lambda");
    }
    return holder_->invoke (ystl::forward<Args> (args)...);
  }

public:
  Lambda &operator= (nullptr_t) noexcept {
    destroy_holder ();
    return *this;
  }

  // copy-and-swap so a throwing clone leaves the old target intact
  Lambda &operator= (const Lambda &rhs) {
    if (this != &rhs) {
      Lambda tmp (rhs);
      swap (tmp);
    }
    return *this;
  }

  Lambda &operator= (Lambda &&rhs) noexcept {
    if (this != &rhs) {
      destroy_holder ();
      move_from (ystl::move (rhs));
    }
    return *this;
  }

  template <typename U>
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, Lambda> && ystl::is_invocable_r_v<R, U, Args...>)
  Lambda &operator= (U &&rhs) {
    Lambda tmp (ystl::forward<U> (rhs));
    swap (tmp);
    return *this;
  }
};

}
