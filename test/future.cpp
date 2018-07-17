#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "future.hpp"

#include <stdexcept>
#include <iostream>

using namespace hntr::platform;

TEST_CASE("Basic future", "[future]") {
    promise<std::string, std::exception> p;   
    p.set_value("blah");
    auto result = p.get_future().get();

    REQUIRE(result == "blah");
}

TEST_CASE("Basic future rejects", "[future]") {
    promise<std::string> p;
    p.set_exception(std::logic_error("Custom error"));
    CHECK_THROWS_AS(p.get_future().get(), std::logic_error);
}
