#pragma once

#include <utility>

namespace hntr::platform {

template <typename T>
class optional {
public:
    optional() : _ok(false) {}
    optional(T const& v) : _value(v) {}
    optional(T && v) : _value(std::forward<T>(v)) {}

    optional& operator= (T const& v) {
        _ok = true;
        _value = v;
        return *this;
    }

    optional& operator= (T && v) {
        _ok = true;
        _value = std::move(v);
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
        assert(_ok);
        return _value;
    }

    T& value() & {
        assert(_ok);
        return _value;
    }

    T&& value() && {
        assert(_ok);
        return _value;
    }

    operator bool() const { return _ok; }

private:
    bool _ok = true;
    T _value;
};

}