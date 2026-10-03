#
# YaPB vendored eyalroz/printf (MIT, see LICENSE).
#
# Standalone snprintf family without libc dependencies. Consumers link
# ystl_printf; ystl picks snprintf_/vsnprintf_ over libc when
# YSTL_USE_VENDORED_PRINTF is set (see cmake/Flags.cmake).
# Kept as a wrapper (not bare sources) so warning/toolchain quirks of the
# third-party tu stay local and never leak into our -Werror builds.
#
# SPDX-License-Identifier: Unlicense
#
cmake_minimum_required(VERSION 3.22)

# the printf tu is C and does not inherit a consumer's target-only -m32: pin it
# to the consumer's bitness (YSTL_32BIT) or infer it from a 32-bit x86 target.
set(_ystl_x86 FALSE)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86|x86_64|amd64|AMD64)$")
    set(_ystl_x86 TRUE)
endif ()

if (NOT DEFINED YSTL_32BIT)
    if (CMAKE_SIZEOF_VOID_P EQUAL 4 AND _ystl_x86)
        set(YSTL_32BIT ON)
    else ()
        set(YSTL_32BIT OFF)
    endif ()
endif ()

# set before enabling C so the compiler check matches any -m32 link flags
if (YSTL_32BIT AND _ystl_x86
    AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif ()

enable_language(C)

add_library(ystl_printf STATIC printf.c stub.c)

# no %n write-back specifier, by design (see README.md)
target_compile_definitions(ystl_printf PRIVATE PRINTF_SUPPORT_WRITEBACK_SPECIFIER=0)

# <printf/printf.h> for consumers (ystl root is on every consumer path already)
target_include_directories(ystl_printf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(ystl_printf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# match the static crt our msvc builds use, else LNK4098 under /WX
if(MSVC)
    set_property(TARGET ystl_printf PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# third-party tu stays warning-neutral: C4146 (intentional unsigned wraparound
# in the float code) and C4701 (heuristic, upstream-clean) under cl/clang-cl
if(MSVC)
    target_compile_options(ystl_printf PRIVATE /wd4146 /wd4701)
endif()
