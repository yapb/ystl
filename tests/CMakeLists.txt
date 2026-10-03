#
# ystl tests and benchmarks (mirrors the old tests/meson.build)
#
# SPDX-License-Identifier: Unlicense
#

set(YSTL_TEST_SOURCES
    main.cpp
    test_utility.cpp
    test_singleton.cpp
    test_algorithm.cpp
    test_atomic.cpp
    test_array.cpp
    test_binheap.cpp
    test_color.cpp
    test_deque.cpp
    test_hashmap.cpp
    test_optparse.cpp
    test_confparse.cpp
    test_tokenizer.cpp
    test_twin.cpp
    test_uniqueptr.cpp
    test_string.cpp
    test_mathlib.cpp
    test_vector.cpp
    test_random.cpp
    test_lambda.cpp
    test_timers.cpp
    test_ulz.cpp
    test_traits.cpp
    test_movable.cpp
    test_memory.cpp
    test_wavehelper.cpp
    test_cpuflags.cpp
    test_platform.cpp
    test_files.cpp
    test_endian.cpp
    test_span.cpp
    test_library.cpp
    test_thread.cpp
    test_logger.cpp
    test_http.cpp
    test_detour.cpp
    test_detour_targets.cpp
    test_simd.cpp
    test_fixedarray.cpp
    test_optional.cpp
    test_tuple.cpp
    test_sort.cpp
    test_flags.cpp
    test_inlinelist.cpp
    test_hashset.cpp
)

set(YSTL_BENCHMARK_SOURCES
    main.cpp
    benchmark_hashmap.cpp
    benchmark_binheap.cpp
    benchmark_array.cpp
    benchmark_string.cpp
    benchmark_deque.cpp
    benchmark_random.cpp
    benchmark_sort.cpp
    benchmark_mathlib.cpp
    benchmark_vector.cpp
    benchmark_confparse.cpp
)

# shared setup for both binaries: same flags the meson build used
function(ystl_setup_target _tgt)
    target_link_libraries(${_tgt} PRIVATE ystl)

    target_include_directories(${_tgt} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(${_tgt} PRIVATE YSTL_COMPAT_STL)

    # force-include wrapper header into every tu like old build did
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${_tgt} PRIVATE /FIforce_include.h)
    else()
        target_compile_options(${_tgt} PRIVATE -include force_include.h)
    endif()

    if(WIN32)
        target_link_libraries(${_tgt} PRIVATE ws2_32)
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(${_tgt} PRIVATE Threads::Threads)

        # 64-bit atomics lower to libatomic calls on targets without native
        # support (e.g. armv6). probe once, then link it when needed.
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles ("
            int main () { unsigned long long v = 0; __atomic_fetch_add (&v, 1, __ATOMIC_SEQ_CST); return static_cast<int> (v); }
        " YSTL_HAVE_NATIVE_64BIT_ATOMICS)
        if(NOT YSTL_HAVE_NATIVE_64BIT_ATOMICS)
            target_link_libraries(${_tgt} PRIVATE atomic)
        endif()
    endif()

    # mirror the old mingw link (static runtimes)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(${_tgt} PRIVATE -static-libstdc++ -static-libgcc)
    endif()
endfunction()

add_executable(ystl_tests ${YSTL_TEST_SOURCES})
ystl_setup_target(ystl_tests)

add_executable(ystl_benchmarks ${YSTL_BENCHMARK_SOURCES})
ystl_setup_target(ystl_benchmarks)

add_test(NAME ystl_tests COMMAND ystl_tests --reporter console)
set_tests_properties(ystl_tests PROPERTIES TIMEOUT 1200)

add_test(NAME ystl_benchmarks COMMAND ystl_benchmarks --reporter console)
set_tests_properties(ystl_benchmarks PROPERTIES TIMEOUT 300)

# run targets: build the binary if needed, then run it straight away
add_custom_target(run-tests
    COMMAND $<TARGET_FILE:ystl_tests> --reporter console
    DEPENDS ystl_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ystl unit tests"
    USES_TERMINAL
    VERBATIM
)

add_custom_target(run-benchmarks
    COMMAND $<TARGET_FILE:ystl_benchmarks> --reporter console
    DEPENDS ystl_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ystl benchmarks"
    USES_TERMINAL
    VERBATIM
)
