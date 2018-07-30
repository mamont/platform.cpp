#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "future.hpp"

#include <stdexcept>
#include <iostream>
#include <thread>
#include <future>

using namespace hntr::platform;

TEST_CASE("Future resolves when set before get_future", "[future][basic]") {
    promise<std::string> p;
    p.set_value("blah");
    REQUIRE(p.get_future().get() == "blah");
}

TEST_CASE("Future resolves when set after get_future", "[future][basic]") {
    promise<std::string> p;
    auto future = p.get_future();
    p.set_value("blah");
    REQUIRE(future.get() == "blah");
}

TEST_CASE("Basic future rejects", "[future]") {
    promise<std::string> p;
    p.set_exception(std::logic_error("Custom error"));
    CHECK_THROWS_AS(p.get_future().get(), std::logic_error);
}

#if 0
TEST_CASE("Future is chainable", "[future]") {
    promise<std::string> p;
    p.set_value("blah");

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).get();

    REQUIRE(result == 33);
}

TEST_CASE("Future is chainable when delayed", "[future]") {
    promise<std::string> p;

    std::async(std::launch::async, [&p]() {
        std::this_thread::sleep_for( std::chrono::seconds{1});
        p.set_value("blah");
    });

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).get();

    REQUIRE(result == 33);
}
#endif
