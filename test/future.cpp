#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "future.hpp"

#include <stdexcept>
#include <iostream>

using namespace hntr::platform;

TEST_CASE("Basic future", "[future]") {
    promise<std::string> p;
    p.set_value("blah");
    REQUIRE(p.get_future().get() == "blah");
}

TEST_CASE("Basic future rejects", "[future]") {
    promise<std::string> p;
    p.set_exception(std::logic_error("Custom error"));
    CHECK_THROWS_AS(p.get_future().get(), std::logic_error);
}

TEST_CASE("Future is chainable", "[future]") {
    promise<std::string> p;
    p.set_value("blah");

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).get();

    REQUIRE(result == 33);
}
