#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "expected.hpp"

using namespace hntr::platform;

TEST_CASE("Expected is expected", "[expected]") {
    constexpr auto exp = expected(true);
    REQUIRE(exp);
}

TEST_CASE("Unexpected is unexpected", "[expected]") {
    constexpr auto unexp = unexpected("Something wrong");
    REQUIRE(!unexp);
}
