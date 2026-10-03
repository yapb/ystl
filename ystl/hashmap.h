// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/platform.h>
#include <ystl/utility.h>
#include <ystl/array.h>
#include <ystl/string.h>
#include <ystl/twin.h>
#include <ystl/flags.h>

namespace ystl {

// identity hash uses the key value directly as the hash value
template <typename T> struct IdentityHash {
  uint32_t operator() (T key) const noexcept {
    return static_cast<uint32_t> (key);
  }
};

template <typename T, typename Enable = void> struct Hash {
  // identity hash below static_casts, which only enums and arithmetics allow.
  // note: floating-point keys compare by value, so distinct nans occupy distinct entries
  static_assert (
    ystl::is_enum_v<T> || ystl::is_integral_v<T> || ystl::is_floating_point_v<T>, "Hash<T> needs an explicit specialization for this key type");

  // default: use identity hash for unspecified types
  uint32_t operator() (T key) const noexcept {
    return IdentityHash<T> {}(key);
  }
};

template <> struct Hash<String, void> {
  uint32_t operator() (const String &key) const noexcept {
    return key.hash ();
  }
};

template <> struct Hash<StringRef, void> {
  uint32_t operator() (const StringRef &key) const noexcept {
    return key.hash ();
  }
};

template <> struct Hash<const char *, void> {
  // hashes the pointed-to string, but the map stores the pointer itself - the key must outlive the entry.
  // mutating the pointed-to bytes after insert corrupts lookups, hash no longer matches
  uint32_t operator() (const char *key) const noexcept {
    if (!key) {
      return 0;
    }
    return StringRef::fnv1a32 (key);
  }
};

// full-avalanche mixer for integer keys with poor low-bit distribution
template <typename T> struct AvalancheHash32 {
  uint32_t operator() (T key) const noexcept {
    uint32_t x = static_cast<uint32_t> (key);
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
  }
};

template <> struct Hash<int32_t, void> {
  // deliberately identity: triangular probing spreads sequential keys well enough.
  // switch to avalanchehash32 for keys with poor low-bit distribution
  uint32_t operator() (int32_t key) const noexcept {
    return static_cast<uint32_t> (key);
  }
};

// hash for flags enums hashes the underlying type value
template <typename E>
  requires ystl::is_flags_hashable_v<E>
struct Hash<E> {
  uint32_t operator() (E key) const noexcept {
    return Hash<ystl::underlying_type_t<E>> {}(ystl::to_underlying (key));
  }
};

template <typename T> struct KeyEqual {
  bool operator() (const T &a, const T &b) const noexcept {
    return a == b;
  }
};

template <> struct KeyEqual<const char *> {
  bool operator() (const char *a, const char *b) const noexcept {
    if (a == b) {
      return true;
    }
    if (!a || !b) {
      return false;
    }
    return strcmp (a, b) == 0;
  }
};

namespace detail {
enum class HashEntryStatus : uint8_t {
  Empty,
  Occupied,
  Deleted
};

template <typename K, typename V> struct HashEntry final : NonCopyable {
  K key {};
  V val {};
  uint32_t hash {};

  HashEntryStatus status { HashEntryStatus::Empty };

  HashEntry () = default;
  ~HashEntry () = default;

  HashEntry (HashEntry &&rhs) noexcept : key (ystl::move (rhs.key)), val (ystl::move (rhs.val)), hash (rhs.hash), status (rhs.status) {}

  HashEntry &operator= (HashEntry &&rhs) noexcept {
    if (this != &rhs) {
      key = ystl::move (rhs.key);
      val = ystl::move (rhs.val);
      hash = rhs.hash;
      status = rhs.status;
    }
    return *this;
  }
};
}

template <typename K, typename V, typename H = Hash<K>, typename E = KeyEqual<K>> class HashMap {
private:
  // note: const char* keys are matched by content, but the pointer itself is stored: it must outlive the entry.
  // entries preconstruct k/v, so both types must be default-constructible and assignable
private:
  using Entry = detail::HashEntry<K, V>;
  using Status = detail::HashEntryStatus;

  H hash_ {};
  E equal_ {};

  size_t size_ {};
  size_t deleted_ {}; // number of tombstones; used_ = length_ + deleted_ tracks slots not empty
  Array<Entry> contents_ {};

  static constexpr size_t kInitialSize = 8;
  static constexpr float kLoadFactor = 0.7f;
  static constexpr size_t kMaxSize = 1 << 24;

private:
  struct PositionResult {
    size_t index {};
    bool found {};
  };

  // probes with triangular numbers to visit every slot exactly once
  PositionResult locate (const K &key, uint32_t hash_value, bool for_insert = false) const noexcept {
    const size_t len = contents_.size ();

    if (len == 0) [[unlikely]] {
      return { 0, false };
    }
    const size_t mask = len - 1;

    size_t index = hash_value & mask;
    size_t step = 1;
    size_t hole = len; // first reusable slot on the path: tombstone (insert), else empty

    for (size_t attempts = 0; attempts < len; ++attempts) {
      const auto &entry = contents_[index];

      if (entry.status == Status::Occupied) [[likely]] {
        if (entry.hash == hash_value && equal_ (entry.key, key)) {
          return { index, true };
        }
      }
      else if (entry.status == Status::Empty) [[likely]] {
        if (hole == len) {
          hole = index;
        }
        break;
      }
      else if (for_insert && hole == len) {
        hole = index;
      }

      index = (index + step) & mask;
      ++step;
    }

    return { hole != len ? hole : 0, false };
  }

  bool needs_rehash () const noexcept {
    // rehash at floor(70% of capacity), tombstones included
    return size_ + deleted_ >= (contents_.size () * 7) / 10;
  }

  void rehash_to (size_t new_size) noexcept {
    assert (new_size != 0 && (new_size & (new_size - 1)) == 0); // triangular probing needs power of two
    auto old_contents = ystl::move (contents_);

    if (new_size > kMaxSize) {
      new_size = kMaxSize;
    }

    if (!contents_.resize (new_size)) [[unlikely]] {
      contents_ = ystl::move (old_contents);
      plat.abort ("HashMap::rehash_to() cannot grow past capacity");
    }

    // resize builds every slot fresh as empty, so only the counters reset here
    size_ = 0;
    deleted_ = 0;

    const size_t mask = contents_.size () - 1;

    for (auto &old_entry : old_contents) {
      if (old_entry.status == Status::Occupied) {
        const uint32_t hash_value = old_entry.hash;

        size_t index = hash_value & mask;
        size_t step = 1;

        for (size_t attempts = 0; attempts < contents_.size (); ++attempts) {
          if (contents_[index].status != Status::Occupied) {
            break;
          }
          index = (index + step) & mask;
          ++step;
        }

        contents_[index].key = ystl::move (old_entry.key);
        contents_[index].val = ystl::move (old_entry.val);

        contents_[index].hash = hash_value;
        contents_[index].status = Status::Occupied;

        ++size_;
      }
    }
  }

  // grows the table on overflow, tombstone rehash keeps the current size
  void rehash () noexcept {
    // full and capped with nothing to reclaim: fail loud instead of overwriting slot zero
    if (deleted_ == 0 && size_ >= contents_.size () && contents_.size () >= kMaxSize) [[unlikely]] {
      plat.abort ("HashMap::rehash() table is full");
    }
    const size_t new_size = contents_.empty () ? kInitialSize : next_power_of_two (ystl::max (size_ * 2, contents_.size ()));

    rehash_to (new_size);
  }

  static size_t next_power_of_two (size_t n) noexcept {
    if (n <= 2) {
      return 2;
    }
    return ystl::bit_ceil (n);
  }

public:
  HashMap () {
    if (!contents_.resize (kInitialSize)) [[unlikely]] {
      plat.abort ("HashMap: reserve failed");
    }
  }

  HashMap (HashMap &&rhs) noexcept :
    hash_ (ystl::move (rhs.hash_)), equal_ (ystl::move (rhs.equal_)), size_ (rhs.size_), deleted_ (rhs.deleted_),
    contents_ (ystl::move (rhs.contents_)) {
    rhs.size_ = 0;
    rhs.deleted_ = 0;
  }

  HashMap (std::initializer_list<Twin<K, V>> list) {
    const size_t cap = next_power_of_two (list.size ());

    if (!contents_.resize (cap > kInitialSize ? cap : kInitialSize)) [[unlikely]] {
      plat.abort ("HashMap: reserve failed");
    }

    for (const auto &elem : list) {
      operator[] (elem.first) = ystl::move (elem.second);
    }
  }
  ~HashMap () = default;

  HashMap &operator= (HashMap &&rhs) noexcept {
    if (this != &rhs) {
      contents_ = ystl::move (rhs.contents_);
      size_ = rhs.size_;
      deleted_ = rhs.deleted_;
      hash_ = ystl::move (rhs.hash_);
      equal_ = ystl::move (rhs.equal_);
      rhs.size_ = 0;
      rhs.deleted_ = 0;
    }
    return *this;
  }

public:
  template <bool IsConst> class HashMapIterator {
  private:
    using EntryType = typename ystl::conditional_t<IsConst, const Entry, Entry>;

    EntryType *current_ {};
    EntryType *end_ {};

    void advance_to_next_occupied () noexcept {
      while (current_ != end_ && current_->status != Status::Occupied) {
        ++current_;
      }
    }

  public:
    using ValueType = typename ystl::conditional_t<IsConst, ystl::Twin<const K &, const V &>, ystl::Twin<const K &, V &>>;

    using Reference = ValueType;
    using Pointer = void; // no operator-> by design, use (*it).first

    HashMapIterator (EntryType *current, EntryType *end) : current_ (current), end_ (end) {

      advance_to_next_occupied ();
    }

    Reference operator* () const noexcept {
      return Reference (current_->key, current_->val);
    }

    HashMapIterator &operator++ () noexcept {
      ++current_;
      advance_to_next_occupied ();

      return *this;
    }

    HashMapIterator operator++ (int) noexcept {
      HashMapIterator tmp = *this;
      ++(*this);

      return tmp;
    }

    bool operator== (const HashMapIterator &other) const noexcept {
      return current_ == other.current_;
    }

    bool operator!= (const HashMapIterator &other) const noexcept {
      return !(*this == other);
    }
  };

  using iterator = HashMapIterator<false>;
  using const_iterator = HashMapIterator<true>;

  iterator begin () noexcept {
    return iterator (contents_.data (), contents_.data () + contents_.size ());
  }

  iterator end () noexcept {
    return iterator (contents_.data () + contents_.size (), contents_.data () + contents_.size ());
  }

  const_iterator begin () const noexcept {
    return cbegin ();
  }

  const_iterator end () const noexcept {
    return cend ();
  }

  const_iterator cbegin () const noexcept {
    return const_iterator (contents_.data (), contents_.data () + contents_.size ());
  }

  const_iterator cend () const noexcept {
    return const_iterator (contents_.data () + contents_.size (), contents_.data () + contents_.size ());
  }

public:
  V &operator[] (const K &key) noexcept {
    if (contents_.empty ()) {
      rehash ();
    }

    const uint32_t hash_value = hash_ (key);
    auto result = locate (key, hash_value, true);

    if (!result.found) {
      if (needs_rehash ()) {
        // the key may reference our own storage, snapshot it before rehash invalidates the reference
        K key_snapshot = key;
        rehash ();
        result = locate (key_snapshot, hash_value, true);

        auto &entry = contents_[result.index];

        if (entry.status == Status::Deleted) {
          --deleted_;
          entry.val = V {}; // drop the stale value left by the erased entry
        }
        entry.key = ystl::move (key_snapshot);
        entry.hash = hash_value;
        entry.status = Status::Occupied;

        ++size_;

        return contents_[result.index].val;
      }

      auto &entry = contents_[result.index];

      if (entry.status == Status::Deleted) {
        --deleted_;
        entry.val = V {}; // drop the stale value left by the erased entry
      }
      entry.key = key;
      entry.hash = hash_value;
      entry.status = Status::Occupied;

      ++size_;
    }
    return contents_[result.index].val;
  }

  V &operator[] (K &&key) noexcept {
    if (contents_.empty ()) {
      rehash ();
    }

    const uint32_t hash_value = hash_ (key);
    auto result = locate (key, hash_value, true);

    if (!result.found) {
      if (needs_rehash ()) {
        // same aliasing hazard as above, the rvalue may still point into our storage
        K key_snapshot = ystl::move (key);
        rehash ();
        result = locate (key_snapshot, hash_value, true);

        auto &entry = contents_[result.index];

        if (entry.status == Status::Deleted) {
          --deleted_;
          entry.val = V {}; // drop the stale value left by the erased entry
        }
        entry.key = ystl::move (key_snapshot);
        entry.hash = hash_value;
        entry.status = Status::Occupied;

        ++size_;

        return contents_[result.index].val;
      }

      auto &entry = contents_[result.index];

      if (entry.status == Status::Deleted) {
        --deleted_;
        entry.val = V {}; // drop the stale value left by the erased entry
      }
      entry.key = ystl::move (key);
      entry.hash = hash_value;
      entry.status = Status::Occupied;

      ++size_;
    }
    return contents_[result.index].val;
  }

  bool insert (const K &key, const V &val) noexcept {
    if (contents_.empty ()) {
      rehash ();
    }

    const uint32_t hash_value = hash_ (key);
    auto result = locate (key, hash_value, true);

    if (result.found) {
      return false;
    }

    if (needs_rehash ()) {
      // either argument may reference our own storage, snapshot both before rehash invalidates them
      K key_snapshot = key;
      V val_snapshot = val;
      rehash ();
      result = locate (key_snapshot, hash_value, true);

      if (result.found) {
        return false;
      }

      auto &entry = contents_[result.index];

      if (entry.status == Status::Deleted) {
        --deleted_;
      }
      entry.key = ystl::move (key_snapshot);
      entry.val = ystl::move (val_snapshot);
      entry.hash = hash_value;
      entry.status = Status::Occupied;
      ++size_;

      return true;
    }

    auto &entry = contents_[result.index];

    if (entry.status == Status::Deleted) {
      --deleted_;
    }
    entry.key = key;
    entry.val = val;
    entry.hash = hash_value;
    entry.status = Status::Occupied;
    ++size_;

    return true;
  }

  bool insert (const K &key, V &&val) noexcept {
    if (contents_.empty ()) {
      rehash ();
    }

    const uint32_t hash_value = hash_ (key);
    auto result = locate (key, hash_value, true);

    if (result.found) {
      return false;
    }

    if (needs_rehash ()) {
      // the key is copied, the value is moved: either may alias our storage
      K key_snapshot = key;
      V val_snapshot = ystl::move (val);
      rehash ();
      result = locate (key_snapshot, hash_value, true);

      if (result.found) {
        return false;
      }

      auto &entry = contents_[result.index];

      if (entry.status == Status::Deleted) {
        --deleted_;
      }
      entry.key = ystl::move (key_snapshot);
      entry.val = ystl::move (val_snapshot);
      entry.hash = hash_value;
      entry.status = Status::Occupied;
      ++size_;

      return true;
    }

    auto &entry = contents_[result.index];

    if (entry.status == Status::Deleted) {
      --deleted_;
    }
    entry.key = key;
    entry.val = ystl::move (val);
    entry.hash = hash_value;
    entry.status = Status::Occupied;
    ++size_;

    return true;
  }

  bool insert (K &&key, V &&val) noexcept {
    if (contents_.empty ()) {
      rehash ();
    }
    const uint32_t hash_value = hash_ (key);
    auto result = locate (key, hash_value, true);

    if (result.found) {
      return false;
    }

    if (needs_rehash ()) {
      // both rvalues may still point into our storage, move-snapshot them before growing
      K key_snapshot = ystl::move (key);
      V val_snapshot = ystl::move (val);
      rehash ();
      result = locate (key_snapshot, hash_value, true);

      if (result.found) {
        return false;
      }

      auto &entry = contents_[result.index];

      if (entry.status == Status::Deleted) {
        --deleted_;
      }
      entry.key = ystl::move (key_snapshot);
      entry.val = ystl::move (val_snapshot);
      entry.hash = hash_value;
      entry.status = Status::Occupied;
      ++size_;

      return true;
    }

    auto &entry = contents_[result.index];

    if (entry.status == Status::Deleted) {
      --deleted_;
    }
    entry.key = ystl::move (key);
    entry.val = ystl::move (val);
    entry.hash = hash_value;
    entry.status = Status::Occupied;
    ++size_;

    return true;
  }

  size_t erase (const K &key) noexcept {
    auto result = locate (key, hash_ (key));

    if (!result.found) {
      return 0;
    }
    contents_[result.index].key = K {};
    contents_[result.index].val = V {};
    contents_[result.index].status = Status::Deleted;
    --size_;
    ++deleted_;

    return 1;
  }

  // erases all entries matching the predicate and returns the count
  template <typename Pred> size_t erase_if (Pred &&pred) noexcept {
    size_t removed = 0;

    for (auto &entry : contents_) {
      if (entry.status != Status::Occupied) {
        continue;
      }

      if (pred (entry.key, entry.val)) {
        entry.key = K {};
        entry.val = V {};
        entry.status = Status::Deleted;
        --size_;
        ++deleted_;
        ++removed;
      }
    }
    return removed;
  }

  bool exists (const K &key) const noexcept {
    return locate (key, hash_ (key)).found;
  }

  // c++20-style membership check, alias for exists()
  bool contains (const K &key) const noexcept {
    return exists (key);
  }

  V *find (const K &key) noexcept {
    auto result = locate (key, hash_ (key));

    return result.found ? &contents_[result.index].val : nullptr;
  }

  const V *find (const K &key) const noexcept {
    auto result = locate (key, hash_ (key));

    return result.found ? &contents_[result.index].val : nullptr;
  }

  void clear () noexcept {
    size_ = 0;
    deleted_ = 0;

    for (auto &entry : contents_) {
      if (entry.status == Status::Occupied) {
        entry.key = K {};
        entry.val = V {};
      }
      entry.status = Status::Empty;
    }
  }

  void zap () noexcept {
    clear ();

    contents_.clear ();
    contents_.resize (kInitialSize);
    contents_.shrink ();
  }

  constexpr size_t size () const noexcept {
    return size_;
  }

  constexpr bool empty () const noexcept {
    return size_ == 0;
  }

  size_t capacity () const noexcept {
    return contents_.size ();
  }

  void reserve (size_t n) noexcept {
    // grows only, tombstones stay until an automatic rehash compacts them
    if (n == 0) [[unlikely]] {
      return;
    }

    // the table caps at kmaxsize: clamp first, the float path below stays exact under 2^24
    if (n >= kMaxSize) {
      if (contents_.size () < kMaxSize) {
        rehash_to (kMaxSize);
      }
      return;
    }
    const size_t needed = static_cast<size_t> (static_cast<float> (n) / kLoadFactor);
    const size_t new_size = next_power_of_two (needed + 1);

    if (new_size > contents_.size ()) {
      rehash_to (new_size);
    }
  }
};

}
