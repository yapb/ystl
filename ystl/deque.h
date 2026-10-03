// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdint.h>

#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>

namespace ystl {

template <typename T> class Deque : public NonCopyable {
private:
  T *contents_ {};

  size_t capacity_ {};
  size_t head_ {};
  size_t size_ {};

private:
  size_t wrap (size_t index) const {
    return index < capacity_ ? index : index - capacity_;
  }

  size_t tail () const {
    return wrap (head_ + size_);
  }

  void grow_to (const size_t capacity) {
    auto contents = mem::allocate<T> (capacity);

    if (size_ > 0) {
      if (head_ + size_ <= capacity_) {
        mem::transfer (contents, &contents_[head_], size_);
      }
      else {
        const auto first_part = capacity_ - head_;

        mem::transfer (contents, &contents_[head_], first_part);
        mem::transfer (&contents[first_part], contents_, size_ - first_part);
      }
    }
    mem::release (contents_);

    contents_ = contents;
    capacity_ = capacity;
    head_ = 0;
  }

  void grow () {
    grow_to (capacity_ > 0 ? capacity_ * 2 : 8);
  }

public:
  // ensures the capacity for at least length elements, so pushes won't grow
  void reserve (const size_t amount) {
    if (capacity_ >= amount) {
      return;
    }
    size_t target = capacity_ > 0 ? capacity_ : 8;

    while (target < amount) {
      if (target > numeric_limits<size_t>::max () / 2) [[unlikely]] {
        plat.abort ("Deque::reserve() capacity overflows");
      }
      target *= 2;
    }
    grow_to (target);
  }

private:
  void destruct_elements () {
    if (size_ == 0) [[unlikely]] {
      return;
    }

    if (head_ + size_ <= capacity_) {
      mem::destruct_array (&contents_[head_], size_);
    }
    else {
      const auto first_part = capacity_ - head_;

      mem::destruct_array (&contents_[head_], first_part);
      mem::destruct_array (contents_, size_ - first_part);
    }
  }

  void destroy () {
    destruct_elements ();
    mem::release (contents_);
  }

  void reset () {
    contents_ = nullptr;
    capacity_ = 0;
    head_ = 0;
    size_ = 0;
  }

public:
  explicit Deque () = default;

  Deque (Deque &&rhs) noexcept : contents_ (rhs.contents_), capacity_ (rhs.capacity_), head_ (rhs.head_), size_ (rhs.size_) {
    rhs.reset ();
  }

  ~Deque () {
    destroy ();
  }

public:
  bool empty () const {
    return size_ == 0;
  }

  size_t size () const {
    return size_;
  }

  // check if deque contains value
  bool contains (const T &value) const {
    for (size_t i = 0; i < size_; ++i) {
      if (contents_[wrap (head_ + i)] == value) {
        return true;
      }
    }
    return false;
  }

  template <typename... Args> void emplace_front (Args &&...args) {
    if (size_ == capacity_) {
      grow ();
    }
    head_ = head_ == 0 ? capacity_ - 1 : head_ - 1;

    mem::construct (&contents_[head_], ystl::forward<Args> (args)...);
    ++size_;
  }

  template <typename... Args> void emplace_last (Args &&...args) {
    if (size_ == capacity_) {
      grow ();
    }
    mem::construct (&contents_[tail ()], ystl::forward<Args> (args)...);
    ++size_;
  }

  void discard_front () {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::discard_front() called on empty deque");
    }
    mem::destruct (&contents_[head_]);

    head_ = wrap (head_ + 1);
    --size_;
  }

  void discard_last () {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::discard_last() called on empty deque");
    }
    --size_;
    mem::destruct (&contents_[tail ()]);
  }

  T pop_front () {
    auto object (ystl::move (front ()));
    discard_front ();

    return object;
  }

  T pop_last () {
    auto object (ystl::move (last ()));
    discard_last ();

    return object;
  }

public:
  const T &front () const {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::front() called on empty deque");
    }
    return contents_[head_];
  }

  T &front () {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::front() called on empty deque");
    }
    return contents_[head_];
  }

  const T &last () const {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::last() called on empty deque");
    }
    return contents_[wrap (head_ + size_ - 1)];
  }

  T &last () {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("Deque::last() called on empty deque");
    }
    return contents_[wrap (head_ + size_ - 1)];
  }

  void clear () {
    destruct_elements ();

    head_ = 0;
    size_ = 0;
  }

public:
  Deque &operator= (Deque &&rhs) noexcept {
    if (this != &rhs) {
      destroy ();

      contents_ = rhs.contents_;
      capacity_ = rhs.capacity_;
      head_ = rhs.head_;
      size_ = rhs.size_;

      rhs.reset ();
    }
    return *this;
  }

public:
  template <bool IsConst> class DequeIterator {
  private:
    using DequeType = typename ystl::conditional_t<IsConst, const Deque, Deque>;
    using ValueType = typename ystl::conditional_t<IsConst, const T, T>;

    DequeType *deque_ {};
    size_t index_ {};

  public:
    using Reference = ValueType &;
    using Pointer = void;

    DequeIterator () = default;

    DequeIterator (DequeType *deque, size_t index) : deque_ (deque), index_ (index) {}

    Reference operator* () const noexcept {
      return deque_->contents_[deque_->wrap (deque_->head_ + index_)];
    }

    DequeIterator &operator++ () noexcept {
      ++index_;
      return *this;
    }

    DequeIterator operator++ (int) noexcept {
      DequeIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    DequeIterator &operator-- () noexcept {
      --index_;
      return *this;
    }

    DequeIterator operator-- (int) noexcept {
      DequeIterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator== (const DequeIterator &other) const noexcept {
      return deque_ == other.deque_ && index_ == other.index_;
    }

    bool operator!= (const DequeIterator &other) const noexcept {
      return !(*this == other);
    }
  };

  using iterator = DequeIterator<false>;
  using const_iterator = DequeIterator<true>;

  iterator begin () noexcept {
    return iterator (this, 0);
  }

  iterator end () noexcept {
    return iterator (this, size_);
  }

  const_iterator begin () const noexcept {
    return cbegin ();
  }

  const_iterator end () const noexcept {
    return cend ();
  }

  const_iterator cbegin () const noexcept {
    return const_iterator (this, 0);
  }

  const_iterator cend () const noexcept {
    return const_iterator (this, size_);
  }
};

}
