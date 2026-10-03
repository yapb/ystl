#include <ystl/ystl.h>

#if defined(__GNUC__) || defined(__clang__)
__attribute__ ((aligned (4096)))
#endif
int sample_function (int x) {
  return x + 1;
}

int replacement_function (int x) {
  return x * 2;
}
