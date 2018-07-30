#pragma once

#include "expected.hpp"

#include "optional.hpp"

#include <exception>
#include <condition_variable>
#include <type_traits>
#include <functional>
#include <memory>
#include <utility>

namespace hntr::platform {

template<typename T> void ptype(T&& parameter);

namespace details {

template <typename T, typename E>
class precursor {
public:
    using value_type = expected<T, E>;

    operator bool() const {
        std::lock_guard lock(_mutex);
        return !!_value;
    }

    value_type const& get() const& {
        std::unique_lock lock(_mutex);
        _cond.wait(lock, [this]() { return !!_value; });
        return *_value;
    }

    value_type& get()& {
        std::unique_lock lock(_mutex);
        _cond.wait(lock, [this]() { return !!_value; });
        return *_value;
    }

    value_type&& get()&& {
        std::unique_lock lock(_mutex);
        _cond.wait(lock, [this]() { return !!_value; });
        return *_value;
    }

    precursor& operator= (value_type const& v) {
        std::lock_guard lock(_mutex);
        assert(!_value);
        _value = v;
        _cond.notify_all();
        return *this;
    }

    template <typename TT = value_type>
    precursor& operator= (TT&& v) {
        std::lock_guard lock(_mutex);
        _value = std::forward<value_type>(v);
        _cond.notify_all();
        return *this;
    }

private:
    std::condition_variable _cond;
    mutable std::mutex _mutex;
    optional<value_type> _value;
};

template <typename T, typename E> using precursor_ptr = std::shared_ptr<precursor<T,E>>;

#if 0
template<typename T, typename E>
class continuation {
public:
    using value_type = expected<T, E>;

    virtual void set_value(value_type const& value) = 0;
    virtual void set_value(value_type && value) = 0;

    continuation& operator= (value_type const& value) {
        set_value(value);
        return *this;
    }

    continuation& operator= (value_type && value) {
        set_value(std::forward<value_type>(value));
        return *this;
    }
};


template<typename T, typename E>
class continuation_impl : public continuation<T, E> {
public:
    using value_type = typename continuation<T, E>::value_type;

    explicit continuation_impl() {}
    explicit continuation_impl(value_type &&v) : _value(std::forward<value_type>(v)) {};

    value_type &get() {
        std::unique_lock lock(_mutex);
        _cond.wait(lock, [this]() { return !!_value; });
        return _value;
    }

    void set_value(value_type const& value) override {
        std::unique_lock lock(_mutex);
        _value = value;
        _cond.notify_all();
    }

    void set_value(value_type && value) override {
        std::unique_lock lock(_mutex);
        _value = value;
        _cond.notify_all();
    }

private:
    std::condition_variable _cond;
    std::mutex _mutex;
    optional<value_type> _value;
};

#endif
} // namespace details

template <typename T, typename E>
class future {
public:
    using value_type = expected<T, E>;

    explicit future() {}
    explicit future(details::precursor_ptr<T, E> value) : _value(std::move(value)) {}

    T& get() & {
        return *_value->get();
    }

    T const& get() const& {
        return *_value->get();
    }

    //template<typename F, typename R = typename std::result_of<F(T())>::type>
    //future<R, E> then(F&& cont) {
    //    return future<R, E>(expected<R, E>(cont(*_value)));
    //}

private:
    details::precursor_ptr<T, E> _value;
};

template <typename T, typename E = std::exception_ptr>
class promise {
public:
    explicit promise() : _value(std::make_shared<details::precursor<T,E>>()) {}
    promise(promise&& o) : _value(std::move(o._value)) {}

    void set_value(T&& v) noexcept {
        assert(_value);
        *_value = expected<T, E>(std::forward<T>(v));
    }

    void set_value(T const& v) {
        assert(_value);
        *_value = expected<T, E>(v);
    }

    void set_exception(E&& e) {
        assert(_value);
        *_value = unexpected(std::forward<E>(e));
    }

    void set_exception(E const& e) {
        assert(_value);
        *_value = unexpected(e);
    }

    template <class TT = E>
    void set_exception(TT&& e) {
        assert(_value);
        *_value = unexpected(std::make_exception_ptr(std::forward<TT>(e)));
    }

    future<T, E> get_future() {
        assert(_value);
        return future<T, E>(_value);
    }

private:
    details::precursor_ptr<T, E> _value;
};

} // namespace hntr::platform

