#include "../include/shared.h"
#include "../include/weak.h"

#include <catch.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////

void odr_helper_use();

TEST_CASE("Just checking for ODR issues") {
    odr_helper_use();

    auto sp = MakeShared<int>(42);
    WeakPtr<int> wp(sp);
    REQUIRE(!wp.Expired());

    auto sp2 = wp.Lock();
    REQUIRE(*sp2 == 42);
}