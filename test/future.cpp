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

TEST_CASE("Future is thenable", "[future]") {
    promise<std::string> p;
    p.set_value("blah");

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).get();

    REQUIRE(result == 33);
}

TEST_CASE("Future is thenable when delayed", "[future]") {
    promise<std::string> p;

    const auto ar = std::async(std::launch::async, [&p]() {
        std::this_thread::sleep_for( std::chrono::milliseconds{100});
        p.set_value("blah");
    });

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).get();

    REQUIRE(result == 33);
}

TEST_CASE("Future is chainable", "[future]") {
    promise<std::string> p;
    p.set_value("blah");

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).then([](auto val) {
                return val * 2;
            }).get();

    REQUIRE(result == 66);
}

TEST_CASE("Future is chainable when delayed", "[future]") {
    promise<std::string> p;

    const auto ap = std::async(std::launch::async, [&p]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        p.set_value("blah");
    });

    auto result = p.get_future()
            .then([](auto val) {
                return 33;
            }).then([](auto val) {
                return val * 2;
            }).get();

    REQUIRE(ap.valid());
    REQUIRE(result == 66);
}

TEST_CASE("Chainable future ignores continuations after throw", "[future]") {
    bool isRun = false;
    promise<std::string> p;
    auto future = p.get_future()
            .then([](auto val) -> decltype(val) {
                throw std::runtime_error("Something happened");
            }).then([&](auto val) {
                isRun = true; // Shouldn't be here
                return 333;
            });
    p.set_value("blah");

    CHECK_THROWS_AS(future.get(), std::runtime_error);
    REQUIRE(!isRun);
}

TEST_CASE("Futures are asynchronously chainable", "[future]") {
    promise<std::string> p;
    auto future = p.get_future()
            .then([](auto val) {
                auto p = std::make_shared<promise<int>>();
                const auto ap = std::async(std::launch::async, [&p]() {
                    std::this_thread::sleep_for( std::chrono::milliseconds{100});
                    p->set_value(123);
                });
                REQUIRE(ap.valid());
                return p->get_future();
            }).then([](auto val) {
                return val * 2;
            });
    p.set_value("blah");
    REQUIRE(future.get() == 123 * 2);
}

TEST_CASE("Futures are asynchronously chainable, void result is allowed", "[future]") {
    bool isRun = false;
    promise<std::string> p;
    auto future = p.get_future()
            .then([](auto val) {
                auto p = std::make_shared<promise<int>>();
                const auto ap = std::async(std::launch::async, [&p]() {
                    std::this_thread::sleep_for( std::chrono::milliseconds{100});
                    p->set_value(123);
                });
                REQUIRE(ap.valid());
                return p->get_future();
            })
            .then([](auto val) {})
            .then([&isRun]() {
                isRun = true;
            });

    REQUIRE(!isRun);
    p.set_value("blah");
    future.get();
    REQUIRE(isRun);
}
