# SPDX-License-Identifier: Unlicense
cmake_minimum_required(VERSION 3.22)

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

if (YSTL_32BIT AND _ystl_x86
    AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif ()

enable_language(C)

add_library(ystl_printf STATIC printf.c stub.c)

target_compile_definitions(ystl_printf PRIVATE PRINTF_SUPPORT_WRITEBACK_SPECIFIER=0)

target_include_directories(ystl_printf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(ystl_printf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    set_property(TARGET ystl_printf PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(MSVC)
    target_compile_options(ystl_printf PRIVATE /wd4146 /wd4701)
endif()
