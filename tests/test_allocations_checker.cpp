#include "allocations_checker/allocations_checker.h"

#include <catch.hpp>

#include <limits>
#include <new>

TEST_CASE("failed nothrow new does not count as allocation") {
    alloc_checker::ResetCounters();

    void* p = ::operator new(std::numeric_limits<std::size_t>::max(), std::nothrow);

    REQUIRE(p == nullptr);
    REQUIRE(alloc_checker::AllocCount() == 0);
}

TEST_CASE("throwing operator new throws bad_alloc on failure") {
    REQUIRE_THROWS_AS(
        ::operator new(std::numeric_limits<std::size_t>::max()),
        std::bad_alloc
    );
}