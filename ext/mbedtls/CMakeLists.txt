# SPDX-License-Identifier: Unlicense
cmake_minimum_required(VERSION 3.22)

set(CMAKE_WARN_DEPRECATED OFF)
# cache defaults (no force, -D still overrides): upstream reads these inside
# functions, where only cache is visible
set(ENABLE_PROGRAMS OFF CACHE BOOL "")
set(ENABLE_TESTING OFF CACHE BOOL "")
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "")
set(GEN_FILES OFF CACHE BOOL "")
set(MBEDTLS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h" CACHE FILEPATH "")
set(TF_PSA_CRYPTO_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/crypto_config.h" CACHE FILEPATH "")

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/CMakeLists.txt")
    message(FATAL_ERROR
        "mbedtls submodule is empty at ext/mbedtls/mbedtls. "
        "fetch it with: git submodule update --init ext/mbedtls/mbedtls "
        "(or pass -DYSTL_WITH_TLS=OFF to build explicitly without https).")
endif ()

# the C side inherits none of the consumer's target-only flags, pin its bitness
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

# the compiler check has to compile 32-bit too when the consumer links -m32
if (YSTL_32BIT AND _ystl_x86
    AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif ()

enable_language(C)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if (NOT DEFINED YSTL_MBEDTLS_ENTROPY_DRIVER)
    if (WINXP OR VITA OR CMAKE_SYSTEM_NAME MATCHES "^(Vita|PSVita|psp2)$")
        set(YSTL_MBEDTLS_ENTROPY_DRIVER ON)
    else ()
        set(YSTL_MBEDTLS_ENTROPY_DRIVER OFF)
    endif ()
endif ()

if (YSTL_MBEDTLS_ENTROPY_DRIVER)
    add_compile_definitions(YSTL_MBEDTLS_ENTROPY_DRIVER=1)
endif ()

# static archives land in shared libs, need pic even when the consumer passes none
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(mbedtls)

if (YSTL_MBEDTLS_ENTROPY_DRIVER)
    target_sources(tfpsacrypto PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/platform_shim.c")
endif ()

set(_ystl_drop "everest|p256")
if (YSTL_MBEDTLS_ENTROPY_DRIVER)
    set(_ystl_drop "${_ystl_drop}|bcrypt")
endif ()

foreach (_tgt mbedtls mbedx509 tfpsacrypto)
    if (TARGET ${_tgt})
        get_target_property (_deps ${_tgt} INTERFACE_LINK_LIBRARIES)

        if (_deps)
            list(FILTER _deps EXCLUDE REGEX "${_ystl_drop}")
            set_target_properties (${_tgt} PROPERTIES INTERFACE_LINK_LIBRARIES "${_deps}")
        endif ()
    endif ()
endforeach ()

add_library(ystl_mbedtls INTERFACE)
target_link_libraries(ystl_mbedtls INTERFACE mbedtls mbedx509 tfpsacrypto)
target_compile_definitions(ystl_mbedtls INTERFACE YSTL_WITH_TLS)

if (YSTL_MBEDTLS_ENTROPY_DRIVER AND WIN32)
    target_link_libraries(ystl_mbedtls INTERFACE advapi32)
endif ()
