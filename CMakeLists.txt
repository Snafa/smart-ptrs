cmake_minimum_required(VERSION 3.16)

project(smart_ptrs LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_smart_ptr_test target)
    cmake_parse_arguments(TEST "WITH_ALLOC_CHECKER" "" "SOURCES" ${ARGN})

    add_executable(${target}
            tests/support/catch/catch_main.cpp
            ${TEST_SOURCES}
    )

    target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/support
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/support/catch
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/support/allocations_checker
    )

    if (TEST_WITH_ALLOC_CHECKER)
        target_sources(${target} PRIVATE
                tests/support/allocations_checker/allocations_checker.cpp
        )
    endif()

    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()

    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_smart_ptr_test(test_shared
        WITH_ALLOC_CHECKER
        SOURCES
        tests/test_shared.cpp
)

add_smart_ptr_test(test_weak
        WITH_ALLOC_CHECKER
        SOURCES
        tests/test_weak.cpp
)

add_smart_ptr_test(test_unique
        SOURCES
        tests/test_unique.cpp
)

add_smart_ptr_test(test_intrusive
        WITH_ALLOC_CHECKER
        SOURCES
        tests/test_intrusive.cpp
)

add_smart_ptr_test(test_odr
        SOURCES
        tests/test_odr.cpp
        tests/odr_helper.cpp
)

add_smart_ptr_test(test_shared_from_this
        WITH_ALLOC_CHECKER
        SOURCES
        tests/test_shared_from_this.cpp
)

add_smart_ptr_test(test_allocations_checker
        WITH_ALLOC_CHECKER
        SOURCES
        tests/test_allocations_checker.cpp
)

set_tests_properties(test_allocations_checker PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=allocator_may_return_null=1"
)