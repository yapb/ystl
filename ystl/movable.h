// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/traits.h>

namespace ystl {

using std::forward;
using std::move;
using std::swap;

// simple non-copying base class
class NonCopyable {
protected:
  explicit NonCopyable () = default;
  ~NonCopyable () = default;

public:
  NonCopyable (const NonCopyable &) = delete;
  NonCopyable &operator= (const NonCopyable &) = delete;
};

// simple non-movable base class
class NonMovable {
protected:
  explicit NonMovable () = default;
  ~NonMovable () = default;

public:
  NonMovable (NonMovable &&) = delete;
  NonMovable &operator= (NonMovable &&) = delete;
};

}
