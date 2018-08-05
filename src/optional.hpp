#pragma once

#include <utility>
#include <exception>

namespace hntr::platform {

template <typename T>
class optional {
public:
    optional() : _ok(false) {}
    optional(T const& v) : _value(v) {}
    optional(T && v) : _value(std::forward<T>(v)) {}

    optional operator= (optional<T> const& o) {
        destroy();
        _ok = o._ok;
        new(&_value) T(o._value);
    }

    optional operator= (optional<T>&& o) {
        destroy();
        _ok = std::move(o._ok);
        if(_ok) new(&_value) T(std::move(o._value));
    }

    optional& operator= (T const& v) {
        destroy();
        _ok = true;
        new(&_value) T(v);
        return *this;
    }

    optional& operator= (T && v) {
        destroy();
        _ok = true;
        new(&_value) T(std::move(v));
        return *this;
    }

    T const& operator* () const& {
        assert(_ok);
        return _value;
    }

    T& operator* () & {
        assert(_ok);
        return _value;
    }

    T&& operator* () && {
        assert(_ok);
        return _value;
    }

    T const& value() const& {
        if (_ok) return _value;
        throw std::runtime_error("bad optional access");
    }

    T& value() & {
        if(_ok) return _value;
        throw std::runtime_error("bad optional access");
    }

    T&& value() && {
        assert(_ok);
        return _value;
    }

    operator bool() const {
        return _ok;
    }

    ~optional() {
        destroy();
    }

private:
    void destroy() {
        if (_ok) _value.~T();
    }

    bool _ok = true;
    union { T _value; };
};

}