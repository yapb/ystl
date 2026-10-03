// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/platform.h>
#include <ystl/random.h>
#include <ystl/traits.h>
#include <ystl/utility.h>
#include <initializer_list>

enum class ReservePolicy : uint16_t {
  PowerOfTwo,
  Proportional,
};

namespace ystl {

namespace detail {

// sbo (small buffer optimization) inline storage
template <typename T, size_t N> struct SboStorage {
  alignas (T) char sbo_[N * sizeof (T)];

  T *sbo_ptr () noexcept {
    return reinterpret_cast<T *> (sbo_);
  }
  const T *sbo_ptr () const noexcept {
    return reinterpret_cast<const T *> (sbo_);
  }
};

// empty specialization for s == 0 (no inline buffer, sbo eliminates this base)
template <typename T> struct SboStorage<T, 0> {
  constexpr T *sbo_ptr () noexcept {
    return nullptr;
  }
  constexpr const T *sbo_ptr () const noexcept {
    return nullptr;
  }
};
}

// simple array class like std::vector, with optional sbo when s > 0
template <typename T, ReservePolicy R = ReservePolicy::PowerOfTwo, size_t S = 0>
class Array : public NonCopyable, private detail::SboStorage<T, S> {
private:
  using Sbo = detail::SboStorage<T, S>;

  // note: heap comes from malloc, so only fundamental alignment is guaranteed

  T *contents_ {};
  size_t capacity_ {};
  size_t size_ {};

  // returns true when contents_ lives on the heap (not in sbo)
  bool is_heap () const noexcept {
    if constexpr (S > 0) {
      return contents_ != Sbo::sbo_ptr ();
    }
    else {
      return contents_ != nullptr;
    }
  }

public:
  explicit Array () {
    if constexpr (S > 0) {
      contents_ = Sbo::sbo_ptr ();
      capacity_ = S;
    }
  }

  Array (const size_t amount, const T &default_value) : Array () {
    if (!reserve (amount)) [[unlikely]] {
      plat.abort ("Array: reserve failed");
    }

    // publish each element before the next, so a throw leaves a valid prefix
    for (size_t i = 0; i < amount; ++i) {
      mem::construct (&contents_[i], default_value);
      ++size_;
    }
  }

  // reserves only, elements are not created
  explicit Array (const size_t amount) : Array () {
    if (!reserve (amount)) [[unlikely]] {
      plat.abort ("Array: reserve failed");
    }
  }

  // unconditional like std::vector: conditional would cycle on recursive types (array of self)
  Array (Array &&rhs) noexcept {
    if constexpr (S > 0) {
      if (!rhs.is_heap ()) {
        // source is sbo - move elements into our sbo
        contents_ = Sbo::sbo_ptr ();
        capacity_ = S;
        size_ = rhs.size_;

        mem::transfer (contents_, rhs.contents_, rhs.size_);
        rhs.size_ = 0;

        return;
      }
    }

    // source is heap - steal pointer
    contents_ = rhs.contents_;
    capacity_ = rhs.capacity_;
    size_ = rhs.size_;

    rhs.reset ();
  }

  Array (std::initializer_list<T> list) : Array () {
    if (!reserve (list.size ())) [[unlikely]] {
      plat.abort ("Array: reserve failed");
    }

    for (const auto &elem : list) {
      mem::construct (&contents_[size_], elem);
      ++size_;
    }
  }

  ~Array () {
    destroy ();
  }

private:
  void destruct_elements () noexcept {
    if constexpr (!ystl::is_trivially_destructible_v<T>) {
      for (size_t i = 0; i < size_; ++i) {
        mem::destruct (&contents_[i]);
      }
    }
  }

  void destroy () {
    destruct_elements ();

    if constexpr (S > 0) {
      if (is_heap ()) {
        mem::release (contents_);
      }
    }
    else {
      mem::release (contents_);
    }
  }

  void reset () noexcept {
    if constexpr (S > 0) {
      contents_ = Sbo::sbo_ptr ();
      capacity_ = S;
    }
    else {
      contents_ = nullptr;
      capacity_ = 0;
    }
    size_ = 0;
  }

  // true if ptr is inside our storage, used to detect self references
  bool points_inside (const void *ptr) const noexcept {
    if (!ptr || !contents_ || capacity_ == 0) {
      return false;
    }
    const auto addr = reinterpret_cast<uintptr_t> (ptr);
    const auto base = reinterpret_cast<uintptr_t> (static_cast<const void *> (contents_));

    // capacity_ <= maxcapacity (~size_max/2/sizeof(t)), so no overflow here
    return addr >= base && addr < base + capacity_ * sizeof (T);
  }

  // true if the element range [ptr, ptr + count) overlaps our storage
  template <typename U> bool range_overlaps (const U *ptr, size_t count) const noexcept {
    if (!ptr || !count || !contents_ || capacity_ == 0) {
      return false;
    }
    if (count > numeric_limits<size_t>::max () / sizeof (U)) {
      return true; // size computation would wrap: take the safe path
    }
    const auto range_bytes = count * sizeof (U);
    const auto addr = reinterpret_cast<uintptr_t> (static_cast<const void *> (ptr));
    const auto base = reinterpret_cast<uintptr_t> (static_cast<const void *> (contents_));

    if (addr > numeric_limits<size_t>::max () - range_bytes) {
      return true; // range end would wrap: take the safe path
    }
    return addr < base + capacity_ * sizeof (T) && base < addr + range_bytes;
  }

  // true if a u value aliases our element and would dangle after regrow
  template <typename U> bool value_aliases (const U &value) const noexcept {
    if constexpr (ystl::is_same_v<ystl::remove_cvref_t<U>, T>) {
      return points_inside (&value);
    }
    else {
      return false;
    }
  }

public:
  // amount is additional, not total
  bool reserve (const size_t amount) {
    if (amount == 0) {
      return true;
    }

    if (capacity_ != 0 && capacity_ - size_ >= amount) [[likely]] {
      return true;
    }

    if (amount > numeric_limits<size_t>::max () - size_) [[unlikely]] {
      return false;
    }
    const size_t required = size_ + amount;
    size_t new_capacity {};

    // maximum number of elements allocatable as a single object (keep within ptrdiff range)
    constexpr auto max_capacity = static_cast<size_t> (~static_cast<size_t> (0) >> 1) / sizeof (T);

    if constexpr (R == ReservePolicy::PowerOfTwo) {
      new_capacity = ystl::bit_ceil (required);

      if (new_capacity < 12) {
        new_capacity = 12;
      }
    }
    else {
      size_t growth = capacity_ >> 1;

      if (growth < 4) {
        growth = 4;
      }

      if (growth > numeric_limits<size_t>::max () - capacity_) [[unlikely]] {
        new_capacity = numeric_limits<size_t>::max ();
      }
      else {
        new_capacity = capacity_ + growth;
      }

      if (new_capacity < required) {
        new_capacity = required;
      }
    }

    // clamp against the maximum allocatable object size
    if (new_capacity > max_capacity) [[unlikely]] {
      new_capacity = max_capacity;
    }

    // the clamp may have cut capacity short, so fail instead of truncating
    if (new_capacity < required) [[unlikely]] {
      return false;
    }

    // sbo -> heap transition
    if constexpr (S > 0) {
      if (!is_heap ()) {
        auto new_contents = mem::allocate<T> (new_capacity);

        if (size_ > 0) {
          mem::transfer (new_contents, contents_, size_);
        }
        contents_ = new_contents;
        capacity_ = new_capacity;

        return true;
      }
    }

    // heap reallocation
    if constexpr (ystl::is_trivially_copyable_v<T>) {
      if (contents_) {
        contents_ = mem::reallocate (contents_, new_capacity);
      }
      else {
        contents_ = mem::allocate<T> (new_capacity);
      }
    }
    else {
      auto new_contents = mem::allocate<T> (new_capacity);

      if (contents_) {
        mem::transfer (new_contents, contents_, size_);
        mem::release (contents_);
      }
      contents_ = new_contents;
    }
    capacity_ = new_capacity;

    return true;
  }

  bool resize (const size_t amount) {
    if (amount < size_) {
      if constexpr (!ystl::is_trivially_destructible_v<T>) {
        for (size_t i = amount; i < size_; ++i) {
          mem::destruct (&contents_[i]);
        }
      }
      size_ = amount;
    }
    else if (amount > size_) {
      if (!ensure (amount)) [[unlikely]] {
        return false;
      }
      mem::construct_array (&contents_[size_], amount - size_);
      size_ = amount;
    }
    return true;
  }

  bool ensure (const size_t amount) {
    if (amount <= size_) [[likely]] {
      return true;
    }
    return reserve (amount - size_);
  }

  template <typename U = size_t> U size () const {
    return static_cast<U> (size_);
  }

  size_t capacity () const {
    return capacity_;
  }

  // set: lvalues (incl
  bool set (size_t index, const T &object) {
    if (index == numeric_limits<size_t>::max ()) [[unlikely]] {
      return false; // index + 1 below would wrap around
    }

    // assigning over a live element must not placement-new; assign in place instead
    if (index < size_) {
      contents_[index] = object;
      return true;
    }

    // snapshot a self reference, as ensure below may relocate our storage
    if (value_aliases (object)) {
      T snapshot (object);
      return set (index, ystl::move (snapshot));
    }

    // ensure fits the total length, reserve would over-allocate the request
    if (!ensure (index + 1)) [[unlikely]] {
      return false;
    }

    // value-init the gap, so skipped elements stay safely destructible
    if (index > size_) {
      mem::construct_array (&contents_[size_], index - size_);
    }
    mem::construct (&contents_[index], object);
    size_ = index + 1;

    return true;
  }

  bool set (size_t index, T &&object) {
    if (index == numeric_limits<size_t>::max ()) [[unlikely]] {
      return false; // index + 1 below would wrap around
    }

    if (index < size_) {
      contents_[index] = ystl::move (object);
      return true;
    }

    // rvalue source the caller gave away: move (don't copy) into the snapshot
    if (value_aliases (object)) {
      T snapshot (ystl::move (object));
      return set (index, ystl::move (snapshot));
    }

    if (!ensure (index + 1)) [[unlikely]] {
      return false;
    }

    if (index > size_) {
      mem::construct_array (&contents_[size_], index - size_);
    }
    mem::construct (&contents_[index], ystl::move (object));
    size_ = index + 1;

    return true;
  }

  // fallback for types convertible to t: materialize a t first, then move from that local
  template <typename U>
  bool set (size_t index, U &&object)
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, T>)
  {
    T staging (ystl::forward<U> (object));
    return set (index, ystl::move (staging));
  }

  // single-element insert: lvalues (incl
  bool insert (size_t index, const T &object) {
    return insert (index, &object, 1);
  }

  bool insert (size_t index, T &&object) {
    return insert (index, &object, 1);
  }

  // fallback for types convertible to t (e.g. stringref); arrays bind the overload below
  template <typename U>
  bool insert (size_t index, U &&object)
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, T> && ystl::is_constructible_v<T, U &&>)
  {
    T staging (ystl::forward<U> (object));
    return insert (index, ystl::move (staging));
  }

  // bulk insert from a raw range: u = t moves from the source range, u = const t copies
  template <typename U>
  bool insert (size_t index, U *objects, size_t count = 1)
    requires (ystl::is_same_v<ystl::remove_cv_t<U>, T>)
  {
    if (!objects) [[unlikely]] {
      return false;
    }

    if (!count) {
      return true;
    }
    const size_t base = size_ > index ? size_ : index;

    if (count > numeric_limits<size_t>::max () - base) [[unlikely]] {
      return false; // total length below would wrap around
    }
    const size_t total = base + count;

    // stash overlapping sources, as reserve and shifting would corrupt them
    if (range_overlaps (objects, count)) {
      Array<T> stash;

      if (!stash.reserve (count)) {
        return false;
      }
      for (size_t i = 0; i < count; ++i) {
        if (!stash.push (ystl::forward<U> (objects[i]))) {
          return false;
        }
      }

      if (stash.size () != count) {
        return false;
      }
      return insert (index, stash.data (), stash.size ());
    }

    // total is the resulting length, so ensure is the right fit; reserve would allocate length_ + total
    if (total > capacity_ && !ensure (total)) [[unlikely]] {
      return false;
    }

    if (index >= size_) {
      // keep the sparse-append behavior: value-init the gap (see set())
      if (index > size_) {
        mem::construct_array (&contents_[size_], index - size_);
      }
      for (size_t i = 0; i < count; ++i) {
        mem::construct (&contents_[i + index], ystl::forward<U> (objects[i]));
      }
      size_ = total;
    }
    else {
      if constexpr (ystl::is_trivially_copyable_v<T>) {
        memmove (&contents_[index + count], &contents_[index], (size_ - index) * sizeof (T));

        for (size_t i = 0; i < count; ++i) {
          contents_[i + index] = ystl::forward<U> (objects[i]);
        }
      }
      else {
        for (size_t i = size_; i > index; --i) {
          const size_t dst = i + count - 1;

          if (dst >= size_) {
            mem::construct (&contents_[dst], ystl::move (contents_[i - 1]));
          }
          else {
            contents_[dst] = ystl::move (contents_[i - 1]);
          }
        }

        for (size_t i = 0; i < count; ++i) {
          if (i + index < size_) {
            mem::destruct (&contents_[i + index]);
          }
          mem::construct (&contents_[i + index], ystl::forward<U> (objects[i]));
        }
      }
      size_ += count;
    }
    return true;
  }

  bool insert (size_t at, const Array &rhs) {
    if (rhs.empty ()) [[unlikely]] {
      return true;
    }
    // self insert flows into the overlapping-range stash below
    return insert (at, static_cast<const T *> (&rhs.contents_[0]), rhs.size_);
  }

  bool erase (const size_t index, const size_t count) {
    if (index >= size_) [[unlikely]] {
      return false;
    }

    if (count == 0) {
      return true;
    }

    const size_t tail = size_ - index;
    if (count > tail) [[unlikely]] {
      return false;
    }

    if constexpr (ystl::is_trivially_copyable_v<T>) {
      const size_t num_to_move = tail - count;
      if (num_to_move != 0) {
        memmove (&contents_[index], &contents_[index + count], num_to_move * sizeof (T));
      }
    }
    else {
      for (size_t i = index; i < index + count; ++i) {
        mem::destruct (&contents_[i]);
      }

      for (size_t i = index; i < size_ - count; ++i) {
        mem::construct (&contents_[i], ystl::move (contents_[i + count]));
        mem::destruct (&contents_[i + count]);
      }
    }

    size_ -= count;
    return true;
  }

  bool shift () {
    return erase (0, 1);
  }

  // unshift mirrors the single-element insert contract: lvalues copy, rvalues move
  bool unshift (const T &object) {
    return insert (0, object);
  }

  bool unshift (T &&object) {
    return insert (0, ystl::move (object));
  }

  template <typename U>
  bool unshift (U &&object)
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, T>)
  {
    T staging (ystl::forward<U> (object));
    return unshift (ystl::move (staging));
  }

  // erases our own element by identity (object must reference an element of this array)
  bool remove (const T &object) {
    // uintptr_t arithmetic keeps the check well-defined for unrelated pointers
    if (!points_inside (&object)) [[unlikely]] {
      return false;
    }
    const auto offset =
      reinterpret_cast<uintptr_t> (static_cast<const void *> (&object)) - reinterpret_cast<uintptr_t> (static_cast<const void *> (contents_));

    if ((offset % sizeof (T)) != 0) [[unlikely]] {
      return false; // inside storage but not element-aligned: not our element
    }
    const size_t idx = offset / sizeof (T);

    if (idx >= size_) [[unlikely]] {
      return false; // in capacity slack, not a live element
    }
    return erase (idx, 1);
  }

  // erase all matches in order and return the removed count
  template <typename Pred> size_t erase_if (Pred &&pred) {
    size_t kept = 0;

    for (size_t i = 0; i < size_; ++i) {
      if (pred (contents_[i])) {
        if constexpr (!ystl::is_trivially_copyable_v<T>) {
          mem::destruct (&contents_[i]);
        }
        continue;
      }

      if (kept != i) {
        if constexpr (ystl::is_trivially_copyable_v<T>) {
          contents_[kept] = contents_[i];
        }
        else {
          mem::construct (&contents_[kept], ystl::move (contents_[i]));
          mem::destruct (&contents_[i]);
        }
      }
      ++kept;
    }

    const size_t removed = size_ - kept;
    size_ = kept;

    return removed;
  }

  // push: lvalues (incl
  YSTL_FORCE_INLINE bool push (const T &object) {
    if (size_ < capacity_) [[likely]] {
      mem::construct (&contents_[size_], object);
      ++size_;
      return true;
    }

    // snapshot a self reference first, or growth would dangle the source
    if (value_aliases (object)) {
      T snapshot (object);

      if (!reserve (1)) [[unlikely]] {
        return false;
      }
      mem::construct (&contents_[size_], ystl::move (snapshot));
      ++size_;

      return true;
    }

    if (!reserve (1)) [[unlikely]] {
      return false;
    }
    mem::construct (&contents_[size_], object);
    ++size_;

    return true;
  }

  YSTL_FORCE_INLINE bool push (T &&object) {
    if (size_ < capacity_) [[likely]] {
      mem::construct (&contents_[size_], ystl::move (object));
      ++size_;
      return true;
    }

    // same hazard for rvalues, so move the source into a snapshot first
    if (value_aliases (object)) {
      T snapshot (ystl::move (object));

      if (!reserve (1)) [[unlikely]] {
        return false;
      }
      mem::construct (&contents_[size_], ystl::move (snapshot));
      ++size_;

      return true;
    }

    if (!reserve (1)) [[unlikely]] {
      return false;
    }
    mem::construct (&contents_[size_], ystl::move (object));
    ++size_;

    return true;
  }

  // fallback for types convertible to t: materialize a t first, then move from that local
  template <typename U>
  YSTL_FORCE_INLINE bool push (U &&object)
    requires (!ystl::is_same_v<ystl::remove_cvref_t<U>, T>)
  {
    T staging (ystl::forward<U> (object));
    return push (ystl::move (staging));
  }

  template <typename... Args> YSTL_FORCE_INLINE bool emplace (Args &&...args) {
    if (size_ < capacity_) [[likely]] {
      mem::construct (&contents_[size_], ystl::forward<Args> (args)...);
      ++size_;
      return true;
    }

    // same hazard as push, so stage args before reserve can relocate them
    if ((value_aliases (args) || ...)) {
      T staging (ystl::forward<Args> (args)...);

      if (!reserve (1)) [[unlikely]] {
        return false;
      }
      mem::construct (&contents_[size_], ystl::move (staging));
      ++size_;

      return true;
    }

    if (!reserve (1)) [[unlikely]] {
      return false;
    }
    mem::construct (&contents_[size_], ystl::forward<Args> (args)...);
    ++size_;

    return true;
  }

  T pop () {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::pop() called on empty array");
    }
    auto object = ystl::move (contents_[size_ - 1]);
    discard ();

    return object;
  }

  void fill (const T &value) {
    // assign in place: no destroy/recreate cycle, no reserve failure modes
    for (size_t i = 0; i < size_; ++i) {
      contents_[i] = value;
    }
  }

  void discard () {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::discard() called on empty array");
    }

    if constexpr (!ystl::is_trivially_destructible_v<T>) {
      mem::destruct (&contents_[size_ - 1]);
    }
    --size_;
  }

  // returns the position of object; object must be a reference to an element of this array
  size_t index (const T &object) const {
    assert (contents_ && &object >= &contents_[0] && &object < &contents_[size_]);
    return static_cast<size_t> (&object - &contents_[0]);
  }

  // find first element matching value, returns pointer or nullptr
  const T *find (const T &value) const {
    for (size_t i = 0; i < size_; ++i) {
      if (contents_[i] == value) {
        return &contents_[i];
      }
    }
    return nullptr;
  }

  T *find (const T &value) {
    for (size_t i = 0; i < size_; ++i) {
      if (contents_[i] == value) {
        return &contents_[i];
      }
    }
    return nullptr;
  }

  // find first element matching the predicate (std::find_if analogue), returns pointer or nullptr
  template <typename Pred> const T *find_if (Pred &&pred) const {
    for (size_t i = 0; i < size_; ++i) {
      if (pred (contents_[i])) {
        return &contents_[i];
      }
    }
    return nullptr;
  }

  template <typename Pred> T *find_if (Pred &&pred) {
    for (size_t i = 0; i < size_; ++i) {
      if (pred (contents_[i])) {
        return &contents_[i];
      }
    }
    return nullptr;
  }

  // check if array contains value
  bool contains (const T &value) const {
    return find (value) != nullptr;
  }

  void shuffle () {
    if (static_cast<size_t> (static_cast<int32_t> (size_)) != size_) [[unlikely]] {
      plat.abort ("Array::shuffle() array too large for int32 indices");
    }
    const int32_t n = static_cast<int32_t> (size_);

    for (int32_t i = n - 1; i > 0; --i) {
      const int32_t j = RWrand::instance () (0, i);

      ystl::swap (contents_[i], contents_[j]);
    }
  }

  void reverse () {
    for (size_t i = 0; i < size_ / 2; ++i) {
      ystl::swap (contents_[i], contents_[size_ - 1 - i]);
    }
  }

  template <typename U> bool extend (const U &rhs) {
    // self append must snapshot first, or reserve would invalidate the source
    if (static_cast<const void *> (&rhs) == static_cast<const void *> (this)) {
      Array<T> snapshot;
      const size_t count = rhs.size ();

      if (!snapshot.reserve (count)) {
        return false;
      }
      for (size_t i = 0; i < count; ++i) {
        if (!snapshot.push (rhs[i])) {
          return false;
        }
      }
      return extend (snapshot);
    }
    const size_t count = rhs.size ();

    if (!reserve (count)) [[unlikely]] {
      return false;
    }

    for (size_t i = 0; i < count; ++i) {
      mem::construct (&contents_[size_], rhs[i]);
      ++size_;
    }
    return true;
  }

  // rvalue containers move elementwise, const-qualified ones still copy
  template <typename U>
    requires (!ystl::is_lvalue_reference_v<U>)
  bool extend (U &&rhs) {
    // moving from self would gut live elements, snapshot instead
    if (static_cast<const void *> (&rhs) == static_cast<const void *> (this)) {
      return extend (static_cast<const U &> (rhs));
    }
    const size_t count = rhs.size ();

    if (!reserve (count)) [[unlikely]] {
      return false;
    }

    for (size_t i = 0; i < count; ++i) {
      mem::construct (&contents_[size_], ystl::move (rhs[i]));
      ++size_;
    }
    return true;
  }

  // copies or moves (by value category) another container in; Array itself stays NonCopyable
  template <typename U> bool assign (U &&rhs) {
    // self-assign is a no-op (clear() first would destroy the source)
    if (static_cast<const void *> (&rhs) == static_cast<const void *> (this)) {
      return true;
    }
    clear ();
    return extend (ystl::forward<U> (rhs));
  }

  void clear () {
    destruct_elements ();
    size_ = 0;
  }

  bool empty () const {
    return size_ == 0;
  }

  bool shrink () {
    if (size_ == capacity_) {
      return false;
    }

    if (!size_) {
      if (is_heap ()) {
        mem::release (contents_);
        reset ();
      }
      return true;
    }

    // try to shrink back into sbo
    if constexpr (S > 0) {
      if (!is_heap ()) {
        return false; // already using sbo, nothing to shrink
      }
      if (size_ <= S) {
        auto heap_contents = contents_;

        contents_ = Sbo::sbo_ptr ();
        mem::transfer (contents_, heap_contents, size_);
        mem::release (heap_contents);

        capacity_ = S;
        return true;
      }
    }

    auto data = mem::allocate<T> (size_);

    mem::transfer (data, contents_, size_);
    mem::release (contents_);

    contents_ = data;
    capacity_ = size_;

    return true;
  }

  template <typename U> const T &at (U index) const {
    assert (static_cast<size_t> (index) < size_);

    if (static_cast<size_t> (index) >= size_) [[unlikely]] {
      plat.abort ("Array::at() index out of range");
    }
    return operator[] (index);
  }

  template <typename U> T &at (U index) {
    assert (static_cast<size_t> (index) < size_);

    if (static_cast<size_t> (index) >= size_) [[unlikely]] {
      plat.abort ("Array::at() index out of range");
    }
    return operator[] (index);
  }

  const T &first () const {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::first() called on empty array");
    }
    return contents_[0];
  }

  T &first () {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::first() called on empty array");
    }
    return contents_[0];
  }

  T &last () {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::last() called on empty array");
    }
    return contents_[size_ - 1];
  }

  const T &last () const {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::last() called on empty array");
    }
    return contents_[size_ - 1];
  }

  const T &random () const {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::random() called on empty array");
    }

    if (size_ <= 1) [[unlikely]] {
      return contents_[0];
    }

    if (static_cast<size_t> (static_cast<int32_t> (size_)) != size_) [[unlikely]] {
      plat.abort ("Array::random() array too large for int32 indices");
    }
    return contents_[RWrand::instance () (0, size<int32_t> () - 1)];
  }

  T &random () {
    assert (!empty ());

    if (empty ()) [[unlikely]] {
      plat.abort ("Array::random() called on empty array");
    }

    if (size_ <= 1) [[unlikely]] {
      return contents_[0];
    }

    if (static_cast<size_t> (static_cast<int32_t> (size_)) != size_) [[unlikely]] {
      plat.abort ("Array::random() array too large for int32 indices");
    }
    return contents_[RWrand::instance () (0, size<int32_t> () - 1)];
  }

  T *data () {
    return contents_;
  }

  const T *data () const {
    return contents_;
  }

public:
  Array &operator= (Array &&rhs) noexcept {
    if (this != &rhs) [[likely]] {
      destroy ();

      if constexpr (S > 0) {
        if (!rhs.is_heap ()) {
          // source is sbo - move elements into our sbo
          contents_ = Sbo::sbo_ptr ();
          capacity_ = S;
          size_ = rhs.size_;

          mem::transfer (contents_, rhs.contents_, rhs.size_);
          rhs.size_ = 0;

          return *this;
        }
      }

      contents_ = rhs.contents_;
      size_ = rhs.size_;
      capacity_ = rhs.capacity_;

      rhs.reset ();
    }
    return *this;
  }

public:
  template <typename U> const T &operator[] (U index) const {
    if constexpr (ystl::is_enum_v<U>) {
      return contents_[ystl::to_underlying (index)];
    }
    else {
      return contents_[index];
    }
  }

  template <typename U> T &operator[] (U index) {
    if constexpr (ystl::is_enum_v<U>) {
      return contents_[ystl::to_underlying (index)];
    }
    else {
      return contents_[index];
    }
  }

  // for range-based loops
public:
  T *begin () {
    return contents_;
  }

  const T *begin () const {
    return contents_;
  }

  T *end () {
    return contents_ ? contents_ + size_ : nullptr;
  }

  const T *end () const {
    return contents_ ? contents_ + size_ : nullptr;
  }
};

// small array with sbo - stores up to n elements inline, spills to heap when exceeded
template <typename T, size_t N = 64> using SmallArray = Array<T, ReservePolicy::Proportional, N>;

}
