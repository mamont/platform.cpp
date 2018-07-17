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
    typedef expected<T, E> type;

    explicit expected() { new(&_value) T(); }
    explicit expected(T const& v) { new (&_value) T(v); }
    template<class TT = T> explicit expected(T&& v) noexcept {
        new (&_value) T(std::forward<TT>(v));
    }

    explicit expected(expected const& v) : ok(v.ok) {
        if (ok) new(&_value) T(v.value());
        else new(&_error) E(v.error());
    }

    explicit expected(expected && v) : ok(v.ok) {
        if (ok) new(&_value) T(std::move(v.value()));
        else new(&_error) E(std::move(v.error()));
    }

    explicit expected(unexpected<E> const& e) : ok(false) {
        new(&_error) E(e.error());
    }

    explicit expected(unexpected<E>&& e) noexcept : ok(false) {
        new(&_error) T(std::forward<E>(e.error()));
    }

    operator bool() const { return ok; }

    T& operator*() & {
        if constexpr (std::is_same<std::exception_ptr, E>::value) {
            if (!ok) std::rethrow_exception(_error);
        } else {
            if (!ok) throw _error;
        }
        return _value;
    }

    T&& operator*() && {
        if constexpr (std::is_same<std::exception_ptr, E>::value) {
            if (!ok) std::rethrow_exception(_error);
        } else {
            if (!ok) throw _error;
        }
        return _value;
    }

    T const& operator*() const& {
        if constexpr (std::is_same<std::exception_ptr, E>::value) {
            if (!ok) std::rethrow_exception(_error);
        } else {
            if (!ok) throw _error;
        }
        return _value;
    }

    E const& error() const& {
        assert(!ok);
        return _error;
    }

    E& error() & {
        assert(!ok);
        return _error;
    }

    E&& error() && {
        assert(!ok);
        return _error;
    }

    T& value() & {
        assert(ok);
        return _value;
    }

    T const& value() const& {
        assert(ok);
        return _value;
    }

    T&& value() && {
        assert(ok);
        return _value;
    }

    expected& operator= (expected&& o) noexcept {
        destroy();

        ok = o.ok;
        if (ok) _value = std::move(o._value);
        else _error = std::move(o._error);

        return *this;
    }

    expected& operator= (unexpected<E>&& e) {
        destroy();
        ok = false;
        _error = std::move(e.error);
        return *this;
    }

    expected& operator= (unexpected<E> const& e) {
        destroy();
        ok = false;
        _error = e.error;
        return *this;
    }

    ~expected() {
        destroy();
    }

private:
    void destroy() {
        if (ok) _value.~T();
        else _error.~E();
    }

    bool ok = true;
    union { T _value; E _error; };
};

}
