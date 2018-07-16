#pragma once

#include <string>
#include <iostream>

namespace hntr::platform {

template<typename T, typename E> class expected;

template <typename E>
class unexpected {
public:
    unexpected(E&& e) : error(std::forward<E>(e)) {}
    unexpected(E const& e) : error(e) {}

private: 
    template<typename, typename> friend class expected;
    E error;
};

template<typename T, typename E>
class expected {
public:
    explicit expected() : _val(T()) {}
    explicit expected(T&& v) noexcept : _val(std::move(v)) {}

    explicit expected(T const& v) : _val(v) {}

    explicit expected(expected<T, E>&& o) noexcept {
        // ok = o.ok;
        // if (ok) {
        //     _val = std::move(o._val);
        // } else {
        //     _exc = std::move(o._exc);
        // }
    }

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

    expected<T,E>& operator= (expected<T, E>&& o) {
        // destroy();
        // 
        // ok = o.ok;
        // if (ok) {
        //     _val = std::move(o._val);
        // } else {
        //     _exc = std::move(o._exc);
        // }

        // o.ok = true;
        // new(&o._val) T();

        return *this;
    }

    ~expected() {
        destroy();
    }

private:
    void destroy() {
        if (ok) _val.~T();
        else _exc.~E();
        ok = false;
    }

    bool ok = true;
    union { T _val; E _exc; };
};

}

