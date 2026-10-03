// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/hashmap.h>

namespace ystl {

// hash set over the hash map with a dummy value: same triangular probing,
// same growth rules, membership-only api with key iterators
template <typename K, typename H = Hash<K>, typename E = KeyEqual<K>> class HashSet {
private:
  struct Dummy {};

  HashMap<K, Dummy, H, E> map_ {};

public:
  HashSet () = default;
  HashSet (HashSet &&rhs) noexcept = default;
  ~HashSet () = default;

  HashSet &operator= (HashSet &&rhs) noexcept = default;

public:
  bool insert (const K &key) noexcept {
    return map_.insert (key, Dummy {});
  }

  bool insert (K &&key) noexcept {
    return map_.insert (ystl::move (key), Dummy {});
  }

  size_t erase (const K &key) noexcept {
    return map_.erase (key);
  }

  [[nodiscard]] bool exists (const K &key) const noexcept {
    return map_.exists (key);
  }

  // c++20-style membership check, alias for exists()
  [[nodiscard]] bool contains (const K &key) const noexcept {
    return map_.contains (key);
  }

  void clear () noexcept {
    map_.clear ();
  }

  void zap () noexcept {
    map_.zap ();
  }

  void reserve (size_t n) noexcept {
    map_.reserve (n);
  }

  [[nodiscard]] constexpr size_t size () const noexcept {
    return map_.size ();
  }

  [[nodiscard]] constexpr bool empty () const noexcept {
    return map_.empty ();
  }

  [[nodiscard]] size_t capacity () const noexcept {
    return map_.capacity ();
  }

public:
  // key-only view over the map iterators, the reference stays valid like the map's
  template <bool IsConst> class HashSetIterator {
  private:
    using MapType = typename ystl::conditional_t<IsConst, const HashMap<K, Dummy, H, E>, HashMap<K, Dummy, H, E>>;
    using MapIterator = typename ystl::conditional_t<IsConst, typename MapType::const_iterator, typename MapType::iterator>;

    MapIterator current_ {};

  public:
    HashSetIterator () = default;
    explicit HashSetIterator (MapIterator current) : current_ (current) {}

  public:
    const K &operator* () const noexcept {
      return (*current_).first;
    }

    HashSetIterator &operator++ () noexcept {
      ++current_;
      return *this;
    }

    HashSetIterator operator++ (int) noexcept {
      HashSetIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator== (const HashSetIterator &other) const noexcept {
      return current_ == other.current_;
    }

    bool operator!= (const HashSetIterator &other) const noexcept {
      return !(*this == other);
    }
  };

  using iterator = HashSetIterator<false>;
  using const_iterator = HashSetIterator<true>;

  iterator begin () noexcept {
    return iterator (map_.begin ());
  }

  iterator end () noexcept {
    return iterator (map_.end ());
  }

  const_iterator begin () const noexcept {
    return cbegin ();
  }

  const_iterator end () const noexcept {
    return cend ();
  }

  const_iterator cbegin () const noexcept {
    return const_iterator (map_.cbegin ());
  }

  const_iterator cend () const noexcept {
    return const_iterator (map_.cend ());
  }
};

}
