// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>

namespace ystl {

struct NulloptType {
  explicit constexpr NulloptType (int) {}
};

struct InPlaceType {
  explicit constexpr InPlaceType (int) {}
};

static constexpr NulloptType Nullopt { 0 };
static constexpr InPlaceType InPlace { 0 };

// optional value container, stores at most one element inline
template <typename T> class Optional {
private:
  alignas (T) char storage_[sizeof (T)];
  bool engaged_ {};

public:
  Optional () = default;

  Optional (NulloptType) : engaged_ (false) {}

  Optional (const T &value) {
    mem::construct (storage_ptr (), value);
    engaged_ = true;
  }

  Optional (T &&value) {
    mem::construct (storage_ptr (), ystl::move (value));
    engaged_ = true;
  }

  template <typename... Args> explicit Optional (InPlaceType, Args &&...args) {
    mem::construct (storage_ptr (), ystl::forward<Args> (args)...);
    engaged_ = true;
  }

  Optional (const Optional &rhs) {
    if (rhs.engaged_) {
      mem::construct (storage_ptr (), *rhs.storage_ptr ());
      engaged_ = true;
    }
  }

  Optional (Optional &&rhs) noexcept {
    if (rhs.engaged_) {
      mem::construct (storage_ptr (), ystl::move (*rhs.storage_ptr ()));
      engaged_ = true;

      rhs.destroy ();
    }
  }

  ~Optional () {
    destroy ();
  }

private:
  T *storage_ptr () noexcept {
    return reinterpret_cast<T *> (storage_);
  }

  const T *storage_ptr () const noexcept {
    return reinterpret_cast<const T *> (storage_);
  }

  void destroy () noexcept {
    if (engaged_) {
      mem::destruct (storage_ptr ());
      engaged_ = false;
    }
  }

  void assign_value (const T &value) {
    if (engaged_) {
      *storage_ptr () = value;
    }
    else {
      mem::construct (storage_ptr (), value);
      engaged_ = true;
    }
  }

  void assign_value (T &&value) {
    if (engaged_) {
      *storage_ptr () = ystl::move (value);
    }
    else {
      mem::construct (storage_ptr (), ystl::move (value));
      engaged_ = true;
    }
  }

public:
  template <typename... Args> T &emplace (Args &&...args) {
    destroy ();
    mem::construct (storage_ptr (), ystl::forward<Args> (args)...);
    engaged_ = true;

    return *storage_ptr ();
  }

  void reset () {
    destroy ();
  }

  void swap (Optional &rhs) noexcept {
    if (this == &rhs) [[unlikely]] {
      return;
    }

    if constexpr (ystl::is_trivially_copyable_v<T>) {
      char temp[sizeof (T)];

      memcpy (temp, storage_, sizeof (T));
      memcpy (storage_, rhs.storage_, sizeof (T));
      memcpy (rhs.storage_, temp, sizeof (T));

      const auto tmp_engaged = engaged_;
      engaged_ = rhs.engaged_;
      rhs.engaged_ = tmp_engaged;
    }
    else {
      if (engaged_ && rhs.engaged_) {
        ystl::swap (*storage_ptr (), *rhs.storage_ptr ());
      }
      else if (engaged_) {
        mem::construct (rhs.storage_ptr (), ystl::move (*storage_ptr ()));
        mem::destruct (storage_ptr ());
        engaged_ = false;
        rhs.engaged_ = true;
      }
      else if (rhs.engaged_) {
        mem::construct (storage_ptr (), ystl::move (*rhs.storage_ptr ()));
        mem::destruct (rhs.storage_ptr ());
        engaged_ = true;
        rhs.engaged_ = false;
      }
    }
  }

public:
  bool has () const {
    return engaged_;
  }

  [[nodiscard]] bool has_value () const {
    return engaged_;
  }

  explicit operator bool () const {
    return engaged_;
  }

  T &value () {
    if (!engaged_) [[unlikely]] {
      plat.abort ("Optional::value() called on empty optional");
    }
    return *storage_ptr ();
  }

  const T &value () const {
    if (!engaged_) [[unlikely]] {
      plat.abort ("Optional::value() called on empty optional");
    }
    return *storage_ptr ();
  }

  template <typename U> T value_or (U &&default_value) const {
    return engaged_ ? *storage_ptr () : static_cast<T> (ystl::forward<U> (default_value));
  }

  const T &operator* () const {
    return *storage_ptr ();
  }

  T &operator* () {
    return *storage_ptr ();
  }

  const T *operator->() const {
    return storage_ptr ();
  }

  T *operator->() {
    return storage_ptr ();
  }

public:
  Optional &operator= (NulloptType) {
    destroy ();
    return *this;
  }

  Optional &operator= (const T &value) {
    assign_value (value);
    return *this;
  }

  Optional &operator= (T &&value) {
    assign_value (ystl::move (value));
    return *this;
  }

  Optional &operator= (const Optional &rhs) {
    if (this != &rhs) [[likely]] {
      if (rhs.engaged_) {
        assign_value (*rhs.storage_ptr ());
      }
      else {
        destroy ();
      }
    }
    return *this;
  }

  Optional &operator= (Optional &&rhs) noexcept {
    if (this != &rhs) [[likely]] {
      if (rhs.engaged_) {
        assign_value (ystl::move (*rhs.storage_ptr ()));
        rhs.destroy ();
      }
      else {
        destroy ();
      }
    }
    return *this;
  }

public:
  bool operator== (const Optional &rhs) const {
    if (engaged_ != rhs.engaged_) {
      return false;
    }
    return !engaged_ || *storage_ptr () == *rhs.storage_ptr ();
  }

  bool operator!= (const Optional &rhs) const {
    return !(*this == rhs);
  }

  bool operator< (const Optional &rhs) const {
    if (!engaged_) {
      return rhs.engaged_;
    }
    if (!rhs.engaged_) {
      return false;
    }
    return *storage_ptr () < *rhs.storage_ptr ();
  }

  bool operator> (const Optional &rhs) const {
    return rhs < *this;
  }

  bool operator<= (const Optional &rhs) const {
    return !(rhs < *this);
  }

  bool operator>= (const Optional &rhs) const {
    return !(*this < rhs);
  }

public:
  bool operator== (NulloptType) const {
    return !engaged_;
  }

  bool operator!= (NulloptType) const {
    return engaged_;
  }

  bool operator< (NulloptType) const {
    return false;
  }

  bool operator> (NulloptType) const {
    return engaged_;
  }

  bool operator<= (NulloptType) const {
    return !engaged_;
  }

  bool operator>= (NulloptType) const {
    return true;
  }

  template <typename U> bool operator== (const U &value) const {
    return engaged_ && *storage_ptr () == value;
  }

  template <typename U> bool operator!= (const U &value) const {
    return !engaged_ || !(*storage_ptr () == value);
  }

  template <typename U> bool operator< (const U &value) const {
    return !engaged_ || *storage_ptr () < value;
  }

  template <typename U> bool operator> (const U &value) const {
    return engaged_ && value < *storage_ptr ();
  }

  template <typename U> bool operator<= (const U &value) const {
    return !(*this > value);
  }

  template <typename U> bool operator>= (const U &value) const {
    return !(*this < value);
  }
};

// free-function operators for reversed operand order (nullopt == opt, value == opt)
template <typename T> bool operator== (NulloptType, const Optional<T> &rhs) {
  return !rhs.has ();
}

template <typename T> bool operator!= (NulloptType, const Optional<T> &rhs) {
  return rhs.has ();
}

template <typename T> bool operator< (NulloptType, const Optional<T> &rhs) {
  return rhs.has ();
}

template <typename T> bool operator> (NulloptType, [[maybe_unused]] const Optional<T> &rhs) {
  return false;
}

template <typename T> bool operator<= (NulloptType, [[maybe_unused]] const Optional<T> &rhs) {
  return true;
}

template <typename T> bool operator>= (NulloptType, const Optional<T> &rhs) {
  return !rhs.has ();
}

template <typename T, typename U> bool operator== (const U &value, const Optional<T> &rhs) {
  return rhs.has () && value == *rhs;
}

template <typename T, typename U> bool operator!= (const U &value, const Optional<T> &rhs) {
  return !rhs.has () || !(value == *rhs);
}

template <typename T, typename U> bool operator< (const U &value, const Optional<T> &rhs) {
  return rhs.has () && value < *rhs;
}

template <typename T, typename U> bool operator> (const U &value, const Optional<T> &rhs) {
  return !rhs.has () || *rhs < value;
}

template <typename T, typename U> bool operator<= (const U &value, const Optional<T> &rhs) {
  return !(value > rhs);
}

template <typename T, typename U> bool operator>= (const U &value, const Optional<T> &rhs) {
  return !(value < rhs);
}

template <typename T, typename... Args> Optional<T> make_optional (Args &&...args) {
  return Optional<T> (InPlace, ystl::forward<Args> (args)...);
}

}
