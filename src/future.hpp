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
class continuation {
public:
    using value_type = expected<T, E>;

    virtual void set_value(expected<T, E> const& value) = 0;
    virtual void set_value(expected<T, E> && value) = 0;
};


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

    template <class _Rep, class _Period>
    optional<T> get(std::chrono::duration<_Rep, _Period> const& duration) {
        std::unique_lock lock(_mutex);
        auto result = _cond.wait_for(lock, duration, [this]() { return !!_value; });
        if (!result) {
            return {};
        }
        return *_value;
    }

    void set_value(value_type const& v) {
        std::lock_guard lock(_mutex);
        assert(!_value);
        _value = v;
        _cond.notify_all();
    }

    void set_value(value_type&& v) {
        std::lock_guard lock(_mutex);
        _value = std::move(v);
        _cond.notify_all();
    }

    void set_continuation(std::shared_ptr<continuation<T, E>> cont) {
        std::lock_guard lock(_mutex);
        assert(!_cont);
        _cont = cont;
        if (_value) {
            _cont->set_value(*_value);
        }
    }

private:
    std::condition_variable _cond;
    mutable std::mutex _mutex;
    optional<value_type> _value;
    std::shared_ptr<continuation<T, E>> _cont;
};

template <typename T, typename E> using precursor_ptr = std::shared_ptr<precursor<T,E>>;


template <typename I, typename O, typename E>
class link
        : public continuation<I, E>
        , public precursor<O, E>
{
public:
    link(std::function<O(I)> const& handler) : _handler(handler) {}
    link(std::function<O(I)>&& handler) : _handler(handler) {}

    void set_value(expected<I, E> const& value) override {
        if (value) {
            precursor<O, E>::set_value(expected<O, E>(_handler(*value)));
        } else {
            precursor<O, E>::set_value(unexpected(value.error()));
        }
    };

    void set_value(expected<I, E> && value) override {
        if (value) {
            precursor<O, E>::set_value(expected<O, E>(_handler(*value)));
        } else {
            precursor<O, E>::set_value(unexpected(value.error()));
        }
    };

    std::function<O(I)> _handler;
};

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

    template <class _Rep, class _Period>
    optional<T> get(std::chrono::duration<_Rep, _Period> const& duration) {
        return _value->get(duration);
    }

    template<typename F, typename R = decltype(std::declval<F>()(std::declval<T>()))>
    future<R, E> then(F&& handler) {
        auto l = std::make_shared<details::link<T, R, E>>(std::forward<std::function<R(T)>>(handler));
        _value->set_continuation(std::static_pointer_cast<details::continuation<T, E>>(l));
        return future<R, E>(l);
    }

private:
    details::precursor_ptr<T, E> _value;
};

template <typename T, typename E = std::exception_ptr>
class promise {
public:
    explicit promise() : _continuation(std::make_shared<details::precursor<T,E>>()) {}
    promise(promise&& o) : _continuation(std::move(o._continuation)) {}

    void set_value(T&& v) {
        assert(_continuation);
        _continuation->set_value(expected<T, E>(std::forward<T>(v)));
    }

    void set_value(T const& v) {
        assert(_continuation);
        _continuation->set_value(expected<T, E>(v));
    }

    void set_exception(E&& e) {
        assert(!_continuation);
        _continuation->set_value(unexpected(std::forward<E>(e)));
    }

    void set_exception(E const& e) {
        assert(!_continuation);
        _continuation->set_value(unexpected(e));
    }

    template <typename EE = E>
    void set_exception(EE&& e) {
        assert(!*_continuation);
        if constexpr (std::is_base_of_v<std::exception, EE> && std::is_same_v<std::exception_ptr, E>) {
            _continuation->set_value(unexpected(std::make_exception_ptr(e)));
        } else {
            _continuation->set_value(unexpected(std::forward<E>(e)));
        }
    }

    future<T, E> get_future() {
        assert(_continuation);
        return future<T, E>(_continuation);
    }

private:
    details::precursor_ptr<T, E> _continuation;
};

} // namespace hntr::platform

