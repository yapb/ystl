// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/memory.h>
#include <ystl/movable.h>
#include <ystl/traits.h>

namespace ystl {

// element compatibility for derived to base and exact array match
template <typename T, typename U>
concept CompatibleElement =
  ystl::is_same_v<T, U> || (!ystl::is_array_v<T> && !ystl::is_array_v<U> &&
                             ystl::is_convertible_v<typename ystl::clear_extent<U>::type *, typename ystl::clear_extent<T>::type *>);

template <typename T, typename U, typename D, typename E>
concept CompatibleOwner = CompatibleElement<T, U> && ystl::is_constructible_v<D, E &&>;

// default deleter for single objects
template <typename T> struct DefaultDelete {
  constexpr DefaultDelete () = default;

  // allow derived -> base conversions, same rule as the owning uniqueptr
  template <typename U>
    requires (CompatibleElement<T, U>)
  constexpr DefaultDelete (const DefaultDelete<U> &) noexcept {}

  constexpr void operator() (T *ptr) const {
    mem::destruct_and_release (ptr);
  }
};

// default deleter for arrays
template <typename T> struct DefaultDelete<T[]> {
  size_t size {};

  constexpr DefaultDelete () = default;
  constexpr explicit DefaultDelete (size_t count) : size (count) {}

  // note: t and u both denote element types in this specialization
  template <typename U>
    requires (ystl::is_same_v<T, U>)
  constexpr DefaultDelete (const DefaultDelete<U[]> &rhs) noexcept : size (rhs.size) {}

  constexpr void operator() (T *ptr) const {
    mem::destruct_and_release_array (ptr, size);
  }
};

namespace detail {
// sized default array deleter may carry a stale zero length
template <typename D> struct is_default_array_delete : false_type {};
template <typename U> struct is_default_array_delete<DefaultDelete<U[]>> : true_type {};

template <typename T, typename D> constexpr bool needs_array_size = ystl::is_array_v<T> && is_default_array_delete<D>::value;
}

// simple unique pointer covering both object and array usage
template <typename T, typename Deleter = DefaultDelete<T>> class UniquePtr final : public NonCopyable {
  static_assert (!ystl::is_bounded_array_v<T>, "UniquePtr<T[N]> is meaningless, use UniquePtr<T[]>");

public:
  using Element = typename ystl::clear_extent<T>::type;

private:
  Element *ptr_ {};
  Deleter deleter_ {};

public:
  constexpr UniquePtr () = default;

  // bare pointer construction is object-only without array size
  constexpr explicit UniquePtr (Element *ptr)
    requires (!ystl::is_array_v<T>)
    : ptr_ (ptr) {}

  // array-only: size feeds the deleter (defaultdelete or any deleter (size_t))
  constexpr UniquePtr (Element *ptr, size_t size)
    requires (ystl::is_array_v<T> && ystl::is_constructible_v<Deleter, size_t>)
    : ptr_ (ptr), deleter_ (size) {}

  constexpr UniquePtr (Element *ptr, const Deleter &deleter)
    requires (!detail::needs_array_size<T, Deleter>)
    : ptr_ (ptr), deleter_ (deleter) {}

  constexpr UniquePtr (Element *ptr, Deleter &&deleter)
    requires (!detail::needs_array_size<T, Deleter>)
    : ptr_ (ptr), deleter_ (ystl::move (deleter)) {}

  constexpr UniquePtr (UniquePtr &&rhs) noexcept : ptr_ (rhs.release ()), deleter_ (ystl::move (rhs.deleter_)) {}

  // converting move carries the deleter along; unconditionally noexcept to match std::unique_ptr
  template <typename U, typename E>
    requires (CompatibleOwner<T, U, Deleter, E>)
  constexpr UniquePtr (UniquePtr<U, E> &&rhs) noexcept : ptr_ (rhs.release ()), deleter_ (ystl::move (rhs.get_deleter ())) {}

  ~UniquePtr () {
    destroy ();
  }

public:
  constexpr Element *get () const {
    return ptr_;
  }

  constexpr Element *release () {
    auto ret = ptr_;
    ptr_ = nullptr;

    return ret;
  }

  constexpr void reset (Element *ptr = nullptr)
    requires (!ystl::is_array_v<T>)
  {
    if (ptr_ != ptr) {
      destroy ();
      ptr_ = ptr;
    }
  }

  // arrays reset through the size-aware overload below, the size is unknowable here
  constexpr void reset ()
    requires (ystl::is_array_v<T>)
  {
    destroy ();
    ptr_ = nullptr;
  }

  // size-aware reset for arrays whose deleter carries the length
  // note: the deleter is rebuilt from size, any state in a custom deleter is lost (same as std::unique_ptr::reset)
  constexpr void reset (Element *ptr, size_t size)
    requires (ystl::is_array_v<T> && ystl::is_constructible_v<Deleter, size_t>)
  {
    destroy ();
    ptr_ = ptr;
    deleter_ = Deleter (size);
  }

  constexpr void swap (UniquePtr &rhs) noexcept {
    ystl::swap (ptr_, rhs.ptr_);
    ystl::swap (deleter_, rhs.deleter_);
  }

  constexpr Deleter &get_deleter () {
    return deleter_;
  }

  constexpr const Deleter &get_deleter () const {
    return deleter_;
  }

private:
  constexpr void destroy () {
    if (ptr_) {
      deleter_ (ptr_);
      ptr_ = nullptr;
    }
  }

public:
  constexpr UniquePtr &operator= (UniquePtr &&rhs) noexcept {
    if (this != &rhs) {
      // release first: rhs may live inside the owned object (e.g. pool lists)
      auto *taken = rhs.release ();

      if (ptr_ != taken) {
        destroy ();
        ptr_ = taken;
      }
      deleter_ = ystl::move (rhs.deleter_);
    }
    return *this;
  }

  template <typename U, typename E>
    requires (CompatibleOwner<T, U, Deleter, E>)
  constexpr UniquePtr &operator= (UniquePtr<U, E> &&rhs) noexcept {
    // take-then-destroy like above: reset(ptr) does not exist for arrays, and rhs may alias our storage
    auto *taken = rhs.release ();

    if (ptr_ != taken) {
      destroy ();
      ptr_ = taken;
    }
    deleter_ = ystl::move (rhs.get_deleter ());
    return *this;
  }

  constexpr UniquePtr &operator= (nullptr_t) noexcept {
    reset ();
    return *this;
  }

  constexpr bool operator== (const UniquePtr &rhs) const {
    return ptr_ == rhs.ptr_;
  }

  template <typename U, typename E>
    requires (CompatibleElement<T, U> || CompatibleElement<U, T>)
  constexpr bool operator== (const UniquePtr<U, E> &rhs) const {
    return ptr_ == rhs.get ();
  }

  constexpr bool operator== (nullptr_t) const {
    return ptr_ == nullptr;
  }

  // object interface
  constexpr T &operator* () const
    requires (!ystl::is_array_v<T>)
  {
    return *ptr_;
  }

  constexpr Element *operator->() const
    requires (!ystl::is_array_v<T>)
  {
    return ptr_;
  }

  // array interface
  constexpr Element &operator[] (size_t index)
    requires (ystl::is_array_v<T>)
  {
    return ptr_[index];
  }

  constexpr const Element &operator[] (size_t index) const
    requires (ystl::is_array_v<T>)
  {
    return ptr_[index];
  }

  explicit constexpr operator bool () const {
    return ptr_ != nullptr;
  }
};

template <typename E>
  requires (!ystl::is_array_v<E>)
UniquePtr (E *) -> UniquePtr<E>;

template <typename E> UniquePtr (E *, size_t) -> UniquePtr<E[]>;

template <typename T, typename... Args>
  requires (!ystl::is_array_v<T>)
constexpr UniquePtr<T> make_unique (Args &&...args) {
  auto ptr = mem::allocate<T> ();
  mem::construct (ptr, ystl::forward<Args> (args)...);
  return UniquePtr<T> { ptr };
}

template <typename T>
  requires (ystl::is_array_v<T> && ystl::is_same_v<T, typename ystl::clear_extent<T>::type[]>)
constexpr UniquePtr<T> make_unique (const size_t size) {
  using Raw = typename ystl::clear_extent<T>::type;

  // trivial elements use zeroed storage, others get constructed
  if constexpr (ystl::is_trivially_default_constructible_v<Raw>) {
    auto ptr = mem::allocate_zeroed<Raw> (size);
    return UniquePtr<T> { ptr, size };
  }
  else {
    auto ptr = mem::allocate<Raw> (size);
    mem::construct_array (ptr, size);
    return UniquePtr<T> { ptr, size };
  }
}

template <typename T, typename... Args>
  requires (ystl::is_array_v<T> && !ystl::is_same_v<T, typename ystl::clear_extent<T>::type[]>)
void make_unique (Args &&...) = delete;

}
