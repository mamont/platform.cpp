#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "future.hpp"

using namespace hntr::platform;

TEST_CASE("Basic future", "[future]") {
    promise<std::string, std::exception> p;   
    p.set_value("blah");
    auto result = p.get_future();

   // REQUIRE(result == "blah");
}

