#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "expected.hpp"

using namespace hntr::platform;

TEST_CASE("expected is expected", "[expected]") {
    auto exp = expected<int, int>();
    REQUIRE(exp);
}

TEST_CASE("expected value is correct", "[expected]") {
    auto exp = expected<int, int>(123);
    REQUIRE(exp);
    REQUIRE(*exp == 123);
}

TEST_CASE("Unexpected is unexpected", "[expected]") {
    auto unexp = expected<char, int>(unexpected(22));
    REQUIRE(!unexp);
    CHECK_THROWS_AS(*unexp, int);
}

TEST_CASE("Non-trivial types are ok", "[expected]") {
    auto exp = expected<std::string, std::string>(std::string("aaa"));
    REQUIRE(*exp == "aaa");

    auto unexp = expected<int, std::string>(unexpected(std::string("bbb")));
    REQUIRE(unexp.error() == "bbb");
    CHECK_THROWS_AS(*unexp, std::string);
}

