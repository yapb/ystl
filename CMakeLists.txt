# SPDX-License-Identifier: Unlicense
cmake_minimum_required(VERSION 3.22)

project(ystl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_library(ystl INTERFACE)
target_include_directories(ystl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ystl INTERFACE cxx_std_20)

target_compile_definitions(ystl INTERFACE _FILE_OFFSET_BITS=64)

option(YSTL_BUILD_TESTS "Build ystl tests and benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(YSTL_WITH_TLS "Enable HTTPS support via vendored mbedtls" ${PROJECT_IS_TOP_LEVEL})

if(YSTL_WITH_TLS)
    add_subdirectory(ext/mbedtls)
    target_link_libraries(ystl INTERFACE ystl_mbedtls)
endif()

if(YSTL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
