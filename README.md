# ystl

ystl is a small, header-only utility library for the YaPB bot and tools.

Originally written as part of PODbot back in 2000, when STL support was
inconsistent across compilers. That is no longer the case, so the library is
being trimmed down as the bot moves to STL. Until then it stays as the common
foundation for YaPB and its tooling.

## Requirements

- C++20 capable compiler
- CMake 3.22 or newer

## Layout

- `ystl/` - the headers, include the umbrella `<ystl/ystl.h>`
- `ext/printf/` - vendored standalone `snprintf` family (MIT)
- `ext/mbedtls/` - vendored mbedtls used by `ystl/http.h` for HTTPS
  (optional, see `YSTL_WITH_TLS`)
- `tests/` - unit tests and micro benchmarks

## Building the tests

```sh
cmake --preset test
cmake --build --preset test
ctest --preset test
```

Benchmarks are built with the `all` preset:

```sh
cmake --preset all && cmake --build --preset all
ctest --preset all -R ^ystl_tests$
```

## HTTPS

`ystl/http.h` can speak HTTPS through the vendored mbedtls. It is enabled by
configuring with `-DYSTL_WITH_TLS=ON` (default when ystl is the top-level
project). Make sure submodules are initialized first:

```sh
git submodule update --init --recursive
```

## License

Released into the public domain under the [Unlicense](UNLICENSE).
