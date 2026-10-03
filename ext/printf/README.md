# vendored eyalroz/printf (lives in ystl so all downstream projects share it)

Tiny `snprintf` family without libc dependencies. `%n` is compiled
out (`PRINTF_SUPPORT_WRITEBACK_SPECIFIER=0`, see `CMakeLists.txt`),
MIT licensed, see `LICENSE`.

- source: https://github.com/eyalroz/printf (consensus fork of mpaland/printf)
- pinned: tag `v6.4.0`
- files: `printf.h`, `printf.c` (verbatim upstream)

Wiring (yapb):

- `CMakeLists.txt` here builds `ystl_printf`, linked by the
  main target in the top-level `CMakeLists.txt`.
- `cmake/Flags.cmake` sets `YSTL_USE_VENDORED_PRINTF`, which makes
  `crlib/string.h` (`SNPrintfWrap`) and `src/linkage.cpp` use
  `snprintf_` / `vsnprintf_` instead of libc.

Wiring (other ystl consumers):

- add `-DCR_USE_VENDORED_PRINTF`, compile `printf.c` + `stub.c`
  into the binary, keep the ystl root on the include path so
  `<printf/printf.h>` resolves. Without the define the libc calls
  stay, so the module is purely opt-in.
- `printf.h` renames the libc entry points via macros
  (`#define snprintf snprintf_` and friends); both consumers include
  it only to call the `*_` symbols explicitly and `#undef` the macros
  right away, so no tu gets a silent global hijack.
