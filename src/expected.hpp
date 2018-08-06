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

    expected(expected const& v) : ok(v.ok) {
        if (ok) new(&_value) T(v.value());
        else new(&_error) E(v.error());
    }

    expected(expected && v) : ok(v.ok) {
        if (ok) new(&_value) T(std::move(v.value()));
        else new(&_error) E(std::move(v.error()));
    }

    expected(unexpected<E> const& e) : ok(false) {
        new(&_error) E(e.error);
    }

    expected(unexpected<E>&& e) noexcept : ok(false) {
        new(&_error) E(std::move(e.error));
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

    expected& operator= (expected const& o) noexcept {
        destroy();

        ok = o.ok;
        if (ok) new(&_value) T(o._value);
        else _error = new(&_error) E(o._error);

        return *this;
    }

    expected& operator= (expected&& o) noexcept {
        destroy();

        ok = o.ok;
        if (ok) new(&_value) T(std::move(o._value));
        else new(&_error) E(std::move(o._error));

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

        new(&_error) E();
    }

    bool ok = true;
    union { T _value; E _error; };
};


// overload for void
// Not sure how standard implementation going to handle this
// but in my understanding void is valid non-exception
template<typename E>
class expected<void, E> {
public:
    typedef expected<void, E> type;

    explicit expected() noexcept {}

    expected(expected const& v) : ok(v.ok) {}

    expected(expected && v) : ok(v.ok) {
        if (!ok) new(&_error) E(std::move(v.error()));
    }

    expected(unexpected<E> const& e) : ok(false) {
        new(&_error) E(e.error);
    }

    expected(unexpected<E>&& e) noexcept : ok(false) {
        new(&_error) E(std::forward<E>(e.error));
    }

    operator bool() const { return ok; }

    void operator*() const& {
        if constexpr (std::is_same<std::exception_ptr, E>::value) {
            if (!ok) std::rethrow_exception(_error);
        } else {
            if (!ok) throw _error;
        }
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

    expected& operator= (expected const& o) noexcept {
        destroy();

        ok = o.ok;
        if (!ok) new(&_error) E(o._error);

        return *this;
    }

    expected& operator= (expected&& o) noexcept {
        destroy();

        ok = o.ok;
        if (!ok) new(&_error) E(std::move(o._error));

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
        if (!ok) _error.~E();
    }

    bool ok = true;
    union { E _error; };
};

}
