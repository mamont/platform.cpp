#pragma once

#include <string>

namespace hntr::platform {

template<typename T>
class expected {
public:
    constexpr explicit expected(T&& v) noexcept : _v(v) {}
    constexpr explicit expected(int code, std::string_view) noexcept {}

    operator bool() const { return true; }

private:
    T _v;
};

template<>
class expected<void> {
public:
    constexpr explicit expected(int code, std::string_view) noexcept {}
    operator bool() const { return true; }
};

class unexpected {
public:
    constexpr explicit unexpected(std::string_view) noexcept {}
    constexpr operator bool() const { return false; }
};

}

