#
# ystl, header-only utility library
#
# SPDX-License-Identifier: Unlicense
#
# standalone, one preset per goal (all, test, benchmark):
#   cmake --preset all && cmake --build --preset all && ctest --preset all
#   cmake --preset test && cmake --build --preset test --target run-tests
#   cmake --preset benchmark && cmake --build --preset benchmark --target run-benchmarks
#
# floor is 3.22: android sdk ships 3.22.1 and debian bookworm (the ci armv6
# leg) ships 3.25, and nothing in here needs anything newer
cmake_minimum_required(VERSION 3.22)

project(ystl VERSION 1.0.0 LANGUAGES CXX)

# classic headers, no c++20 modules: skip clang-scan-deps entirely
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_library(ystl INTERFACE)
target_include_directories(ystl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ystl INTERFACE cxx_std_20)

# large-file support: on 32-bit glibc, readdir()/stat() overflow (EOVERFLOW) on
# filesystems with 64-bit inodes unless off_t is 64-bit. ystl/files.h uses both.
target_compile_definitions(ystl INTERFACE _FILE_OFFSET_BITS=64)

option(YSTL_BUILD_TESTS "Build ystl tests and benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(YSTL_WITH_TLS "Enable HTTPS support via vendored mbedtls" ${PROJECT_IS_TOP_LEVEL})

# https via vendored mbedtls (pinned 4.2), consumed by ystl/http.h.
# requires the ext/mbedtls/mbedtls submodule, pass -DYSTL_WITH_TLS=OFF to
# build without it (https requests then report HttpOnly).
if(YSTL_WITH_TLS)
    add_subdirectory(ext/mbedtls)
    target_link_libraries(ystl INTERFACE ystl_mbedtls)
endif()

if(YSTL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
