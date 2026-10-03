// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/movable.h>

namespace ystl {

// singleton for objects
template <typename T> class Singleton : NonCopyable, NonMovable {
protected:
  explicit Singleton () = default;
  ~Singleton () = default;

public:
  static inline T &instance () {
    static T inst {};
    return inst;
  }
};

// exposes global variable from class singleton
#define YSTL_EXPOSE_GLOBAL_SINGLETON(class_name, global_var)  \
   inline auto &global_var { class_name::instance () }

}
