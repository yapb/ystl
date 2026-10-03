// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>
#include <ystl/platform.h>

namespace ystl {

// intrusive doubly-linked list
//
// nodes live inside the objects themselves, so links stay valid as long as the object
// lives: no invalidation on growth (unlike array-backed containers), unlink is o(1).
// each node stores its owning list, so misuse (double-add, foreign unlink) aborts
// instead of corrupting memory. the list never owns: clear() drops the links,
// destroying the elements is the caller's job, unlink before destroying a linked object.

template <typename T> class InlineList;

template <typename T> class InlineListNode : public NonCopyable, NonMovable {
  friend class InlineList<T>;

private:
  InlineList<T> *owner_ {};
  InlineListNode *next_ {};
  InlineListNode *prev_ {};

public:
  InlineListNode () = default;
  ~InlineListNode () = default;

public:
  [[nodiscard]] bool linked () const noexcept {
    return owner_ != nullptr;
  }
};

template <typename T> class InlineList final : public NonCopyable, NonMovable {
private:
  static InlineListNode<T> *node_of (T *object) noexcept {
    return static_cast<InlineListNode<T> *> (object);
  }

  static T *object_of (InlineListNode<T> *node) noexcept {
    return static_cast<T *> (node);
  }

  static const T *object_of (const InlineListNode<T> *node) noexcept {
    return static_cast<const T *> (node);
  }

private:
  InlineListNode<T> *head_ {};
  InlineListNode<T> *tail_ {};
  size_t size_ {};

private:
  void attach (InlineListNode<T> *node) noexcept {
    node->owner_ = this;
  }

  void detach (InlineListNode<T> *node) noexcept {
    node->owner_ = nullptr;
    node->prev_ = nullptr;
    node->next_ = nullptr;
  }

  void unlink_node (InlineListNode<T> *node) noexcept {
    if (node->prev_ != nullptr) {
      node->prev_->next_ = node->next_;
    }
    else {
      head_ = node->next_;
    }

    if (node->next_ != nullptr) {
      node->next_->prev_ = node->prev_;
    }
    else {
      tail_ = node->prev_;
    }
    detach (node);
    --size_;
  }

public:
  InlineList () = default;
  ~InlineList () = default;

public:
  // links the object at the back, aborts when already linked into any list
  void push_back (T &object) noexcept {
    auto node = node_of (&object);

    if (node->owner_ != nullptr) [[unlikely]] {
      plat.abort ("InlineList::push_back() node is already linked");
    }
    node->prev_ = tail_;
    node->next_ = nullptr;

    if (tail_ != nullptr) {
      tail_->next_ = node;
    }
    else {
      head_ = node;
    }
    tail_ = node;
    attach (node);
    ++size_;
  }

  // links the object at the front, aborts when already linked into any list
  void push_front (T &object) noexcept {
    auto node = node_of (&object);

    if (node->owner_ != nullptr) [[unlikely]] {
      plat.abort ("InlineList::push_front() node is already linked");
    }
    node->prev_ = nullptr;
    node->next_ = head_;

    if (head_ != nullptr) {
      head_->prev_ = node;
    }
    else {
      tail_ = node;
    }
    head_ = node;
    attach (node);
    ++size_;
  }

  // drops the object from this list without destroying it, aborts on foreign nodes
  void unlink (T &object) noexcept {
    auto node = node_of (&object);

    if (node->owner_ != this) [[unlikely]] {
      plat.abort ("InlineList::unlink() node does not belong to this list");
    }
    unlink_node (node);
  }

  // drops every link without destroying the objects
  void clear () noexcept {
    auto node = head_;

    while (node != nullptr) {
      auto next = node->next_;
      detach (node);
      node = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
  }

public:
  [[nodiscard]] T *front () noexcept {
    return head_ != nullptr ? object_of (head_) : nullptr;
  }

  [[nodiscard]] const T *front () const noexcept {
    return head_ != nullptr ? object_of (head_) : nullptr;
  }

  [[nodiscard]] T *back () noexcept {
    return tail_ != nullptr ? object_of (tail_) : nullptr;
  }

  [[nodiscard]] const T *back () const noexcept {
    return tail_ != nullptr ? object_of (tail_) : nullptr;
  }

  template <typename U = size_t> [[nodiscard]] U size () const noexcept {
    return static_cast<U> (size_);
  }

  [[nodiscard]] bool empty () const noexcept {
    return size_ == 0;
  }

public:
  // iterators hold the list for --end() support; node links stay valid across
  // list mutations, except unlinking the pointed node itself
  template <bool IsConst> class InlineListIterator {
  private:
    using ListType = typename ystl::conditional_t<IsConst, const InlineList, InlineList>;
    using NodeType = typename ystl::conditional_t<IsConst, const InlineListNode<T>, InlineListNode<T>>;
    using ValueType = typename ystl::conditional_t<IsConst, const T, T>;

    ListType *list_ {};
    NodeType *node_ {};

  public:
    using Reference = ValueType &;

    InlineListIterator () = default;
    InlineListIterator (ListType *list, NodeType *node) : list_ (list), node_ (node) {}

  public:
    Reference operator* () const noexcept {
      return *InlineList::object_of (node_);
    }

    InlineListIterator &operator++ () noexcept {
      node_ = node_->next_;
      return *this;
    }

    InlineListIterator operator++ (int) noexcept {
      InlineListIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    InlineListIterator &operator-- () noexcept {
      node_ = node_ != nullptr ? node_->prev_ : list_->tail_;
      return *this;
    }

    InlineListIterator operator-- (int) noexcept {
      InlineListIterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator== (const InlineListIterator &other) const noexcept {
      return node_ == other.node_;
    }

    bool operator!= (const InlineListIterator &other) const noexcept {
      return !(*this == other);
    }

    // for erase(): the underlying node being stepped from
    NodeType *node () const noexcept {
      return node_;
    }
  };

  using iterator = InlineListIterator<false>;
  using const_iterator = InlineListIterator<true>;

  iterator begin () noexcept {
    return iterator (this, head_);
  }

  iterator end () noexcept {
    return iterator (this, nullptr);
  }

  const_iterator begin () const noexcept {
    return cbegin ();
  }

  const_iterator end () const noexcept {
    return cend ();
  }

  const_iterator cbegin () const noexcept {
    return const_iterator (this, head_);
  }

  const_iterator cend () const noexcept {
    return const_iterator (this, nullptr);
  }

  // unlinks the pointed element and returns an iterator to the next one
  iterator erase (iterator position) noexcept {
    auto node = position.node ();

    if (node->owner_ != this) [[unlikely]] {
      plat.abort ("InlineList::erase() node does not belong to this list");
    }
    position = iterator (this, node->next_);
    unlink_node (node);

    return position;
  }
};

}
