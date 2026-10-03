// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/algorithm.h>
#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/utility.h>

namespace ystl {

template <typename T, typename Compare = ystl::Less<T>> class BinaryHeap final : public NonCopyable {
private:
  static constexpr size_t kArity = 4;
  static constexpr size_t kInitialCapacity = 16;

  T *data_ {};
  size_t size_ {};
  size_t capacity_ {};
  Compare comp_ {};

private:
  void grow () {
    const size_t capacity = capacity_ > 0 ? capacity_ * 2 : kInitialCapacity;
    auto data = mem::allocate<T> (capacity);

    if (size_ > 0) {
      mem::transfer (data, data_, size_);
    }
    mem::release (data_);

    data_ = data;
    capacity_ = capacity;
  }

  void destroy () {
    mem::destruct_and_release_array (data_, size_);
  }

  void reset () {
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
  }

  void sift_up (size_t index) {
    auto value = ystl::move (data_[index]);

    while (index > 0) {
      const size_t parent = (index - 1) / kArity;

      if (!comp_ (value, data_[parent])) {
        break;
      }
      data_[index] = ystl::move (data_[parent]);
      index = parent;
    }
    data_[index] = ystl::move (value);
  }

  void sift_down (size_t hole, T value) {
    for (;;) {
      const size_t first = hole * kArity + 1;

      if (first >= size_) {
        break;
      }
      const size_t last = first + kArity < size_ ? first + kArity : size_;

      // unrolled branchless child selection for 4-ary
      size_t best = first;

      if (first + 1 < last && comp_ (data_[first + 1], data_[best])) {
        best = first + 1;
      }
      if (first + 2 < last && comp_ (data_[first + 2], data_[best])) {
        best = first + 2;
      }
      if (first + 3 < last && comp_ (data_[first + 3], data_[best])) {
        best = first + 3;
      }

      if (comp_ (data_[best], value)) {
        data_[hole] = ystl::move (data_[best]);
        hole = best;
      }
      else {
        break;
      }
    }
    data_[hole] = ystl::move (value);
  }

  void remove_top () {
    --size_;

    if (size_ > 0) {
      auto last = ystl::move (data_[size_]);
      mem::destruct (&data_[size_]);
      sift_down (0, ystl::move (last));
    }
    else {
      mem::destruct (&data_[0]);
    }
  }

public:
  explicit BinaryHeap () = default;
  explicit BinaryHeap (const Compare &compare) : comp_ (compare) {}

  BinaryHeap (BinaryHeap &&rhs) noexcept : data_ (rhs.data_), size_ (rhs.size_), capacity_ (rhs.capacity_), comp_ (rhs.comp_) {
    rhs.reset ();
  }

  ~BinaryHeap () {
    destroy ();
  }

  BinaryHeap &operator= (BinaryHeap &&rhs) noexcept {
    if (this != &rhs) {
      destroy ();

      data_ = rhs.data_;
      size_ = rhs.size_;
      capacity_ = rhs.capacity_;
      comp_ = rhs.comp_;

      rhs.reset ();
    }
    return *this;
  }

public:
  // allocation failures abort inside mem::allocate, so push cannot fail
  void push (const T &item) {
    if (size_ == capacity_) [[unlikely]] {
      grow ();
    }
    mem::construct (&data_[size_], item);
    ++size_;
    sift_up (size_ - 1);
  }

  void push (T &&item) {
    if (size_ == capacity_) [[unlikely]] {
      grow ();
    }
    mem::construct (&data_[size_], ystl::move (item));
    ++size_;
    sift_up (size_ - 1);
  }

  template <typename... Args> void emplace (Args &&...args) {
    if (size_ == capacity_) [[unlikely]] {
      grow ();
    }
    mem::construct (&data_[size_], ystl::forward<Args> (args)...);
    ++size_;
    sift_up (size_ - 1);
  }

  [[nodiscard]] const T &top () const noexcept {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("BinaryHeap::top() called on empty heap");
    }
    return data_[0];
  }

  [[nodiscard]] T pop () noexcept {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("BinaryHeap::pop() called on empty heap");
    }
    auto result = ystl::move (data_[0]);
    remove_top ();

    return result;
  }

  void pop (T &out) noexcept {
    if (size_ == 0) [[unlikely]] {
      plat.abort ("BinaryHeap::pop() called on empty heap");
    }
    T result = ystl::move (data_[0]);
    remove_top ();
    out = ystl::move (result);
  }

  void discard () noexcept {
    remove_top ();
  }

public:
  [[nodiscard]] size_t size () const noexcept {
    return size_;
  }

  [[nodiscard]] bool empty () const noexcept {
    return size_ == 0;
  }

  void clear () noexcept {
    mem::destruct_array (data_, size_);
    size_ = 0;
  }

  void reserve (size_t amount) {
    if (amount <= capacity_) {
      return;
    }
    const size_t capacity = ystl::bit_ceil (amount);
    auto data = mem::allocate<T> (capacity);

    if (size_ > 0) {
      mem::transfer (data, data_, size_);
    }
    mem::release (data_);

    data_ = data;
    capacity_ = capacity;
  }

  void swap (BinaryHeap &other) noexcept {
    ystl::swap (data_, other.data_);
    ystl::swap (size_, other.size_);
    ystl::swap (capacity_, other.capacity_);
    ystl::swap (comp_, other.comp_);
  }

public:
  // note iteration walks the heap array in heap order, not sorted order
  template <bool IsConst> class BinaryHeapIterator {
  private:
    using HeapType = typename ystl::conditional_t<IsConst, const BinaryHeap, BinaryHeap>;
    using ValueType = typename ystl::conditional_t<IsConst, const T, T>;

    HeapType *heap_ {};
    size_t index_ {};

  public:
    using Reference = ValueType &;
    using Pointer = void;

    BinaryHeapIterator () = default;

    BinaryHeapIterator (HeapType *heap, size_t index) : heap_ (heap), index_ (index) {}

    Reference operator* () const noexcept {
      return heap_->data_[index_];
    }

    BinaryHeapIterator &operator++ () noexcept {
      ++index_;
      return *this;
    }

    BinaryHeapIterator operator++ (int) noexcept {
      BinaryHeapIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    BinaryHeapIterator &operator-- () noexcept {
      --index_;
      return *this;
    }

    BinaryHeapIterator operator-- (int) noexcept {
      BinaryHeapIterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator== (const BinaryHeapIterator &other) const noexcept {
      return heap_ == other.heap_ && index_ == other.index_;
    }

    bool operator!= (const BinaryHeapIterator &other) const noexcept {
      return !(*this == other);
    }
  };

  using iterator = BinaryHeapIterator<false>;
  using const_iterator = BinaryHeapIterator<true>;

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
