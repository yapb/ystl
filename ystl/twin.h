// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>

namespace ystl {

template <typename A, typename B> using Twin = std::pair<A, B>;

template <typename A, typename B> constexpr auto make_twin (A &&a, B &&b) {
  return std::make_pair (ystl::forward<A> (a), ystl::forward<B> (b));
}

}
