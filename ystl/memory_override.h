// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/memory.h>
#include <ystl/platform.h>

// override all the operators to get rid of linking to stdc++
void *operator new (size_t size) {
  return ystl::mem::allocate<void *> (size);
}

void *operator new[] (size_t size) {
  return ystl::mem::allocate<void *> (size);
}

void operator delete (void *ptr) noexcept {
  ystl::mem::release (ptr);
}

void operator delete[] (void *ptr) noexcept {
  ystl::mem::release (ptr);
}

void operator delete (void *ptr, size_t) noexcept {
  ystl::mem::release (ptr);
}

void operator delete[] (void *ptr, size_t) noexcept {
  ystl::mem::release (ptr);
}

YSTL_C_LINKAGE void __cxa_pure_virtual () {
  ystl::plat.abort ("pure virtual function call");
}
