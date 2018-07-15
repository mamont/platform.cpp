#pragma once

#include <string>

namespace hntr::platform {

template<typename T, typename E> class expected;

template <typename E>
class unexpected {
public:
    unexpected(E&& e) : error(std::forward<E>(e)) {}
    unexpected(E const& e) : error(e) {}

    E error;
};

template<typename T, typename E>
class expected {
public:
    explicit expected() : _val(T()) {}
    explicit expected(T&& v) noexcept : _val(std::forward<T>(v)) {}


    explicit expected(unexpected<E> const& e) noexcept 
        : ok(false), _exc(e.error) {}

    explicit expected(unexpected<E>&& e) noexcept 
        : ok(false), _exc(std::move(e.error)) {}

    operator bool() const { return ok; }

    T& operator*() {
        if (!ok) throw _exc;
        return _val;
    }

    E& error() {
        assert(!ok);
        return _exc;
    }

    ~expected() {
        if (ok) _val.~T();
        else _exc.~E();
    }

private:
    const bool ok = true;
    union { T _val; E _exc; };
};


}

