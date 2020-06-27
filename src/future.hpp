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

template <typename T, typename E> class promise;
template <typename T, typename E> class future;
template <typename T, typename E> class cancellable_future;

namespace details {

template <typename T, typename E>
class continuation {
public:
    using value_type = expected<T, E>;
    virtual ~continuation() = default;

    virtual void handle(expected<T, E> const& value) = 0;
    virtual void handle(expected<T, E> && value) = 0;
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
        if (_cont) {
            _cont->handle(*_value);
        }
        _cond.notify_all();
    }

    void set_value(value_type&& v) {
        std::lock_guard lock(_mutex);
        _value = std::move(v);
        if (_cont) {
            _cont->handle(*_value);
        }
        _cond.notify_all();
    }

    void set_continuation(std::shared_ptr<continuation<T, E>> cont) {
        std::lock_guard lock(_mutex);
        assert(!_cont);
        _cont = cont;
        if (_value) {
            _cont->handle(std::move(*_value));
        }
    }

private:
    std::condition_variable _cond;
    mutable std::mutex _mutex;
    optional<value_type> _value;
    std::shared_ptr<continuation<T, E>> _cont;
};

template <typename T, typename E> using precursor_ptr = std::shared_ptr<precursor<T,E>>;

template<typename handler_type, typename arg_type, typename error_type> struct resolver;

template <typename Handler_t, typename I, typename O, typename E>
class link
    : public continuation<I, E>
    , public precursor<O, E>
    , public std::enable_shared_from_this<link<Handler_t, I, O, E>>
{
public:
    link(Handler_t && handler) : _handler(std::forward<Handler_t>(handler)) {}
    link(Handler_t const& handler) : _handler(handler) {}

    void handle(expected<I, E> const& value) override {
        execute_handler(value);
    }

    void handle(expected<I, E> && value) override {
        execute_handler(std::move(value));
    }

private:
    template <typename T = expected<I, E>>
    void execute_handler(T&& value) {
        if (value) {
            execute_handler_impl(std::forward<T>(value));
        } else {
            precursor<O, E>::set_value(unexpected(value.error()));
        }
    }

    template <typename T = expected<I, E>>
    void execute_handler_impl(T&& value) requires std::negation_v<std::is_void<I>> {
        if constexpr (!resolver<Handler_t, I, E>::is_future::value) {
            try {
                if constexpr (!std::is_void_v<O>) {
                    precursor<O, E>::set_value(expected<O, E>(_handler(*value)));
                } else {
                    _handler(*value);
                    precursor<O, E>::set_value(expected<O, E>());
                }
            } catch (...) {
                precursor<O, E>::set_value(unexpected(std::current_exception()));
            }
        } else {
            if constexpr (!std::is_void_v<O>) {
                _handler(*value).then([w = link::weak_from_this()](O result) {
                    if (auto self = w.lock()) {
                        self->set_value(expected<O, E>(result));
                    }
                });
            } else {
                _handler(*value).then([w = link::weak_from_this()]() {
                    if (auto self = w.lock()) {
                        self->set_value(expected<O, E>());
                    }
                });
            }
        }
    }

    template <typename T = expected<void, E>>
    void execute_handler_impl(T&& value) requires std::is_void_v<I> {
        if constexpr (!resolver<Handler_t, I, E>::is_future::value) {
            try {
                if constexpr (!std::is_void_v<O>) {
                    precursor<O, E>::set_value(expected<O, E>(_handler()));
                } else {
                    _handler();
                    precursor<O, E>::set_value(expected<O, E>());
                }
            } catch (...) {
                precursor<O, E>::set_value(unexpected(std::current_exception()));
            }
        } else {
            if constexpr (!std::is_void_v<O>) {
                _handler().then([w = link::weak_from_this()](O result) {
                    if (auto self = w.lock()) {
                        self->set_value(expected<O, E>(result));
                    }
                });
            } else {
                _handler().then([w = link::weak_from_this()]() {
                    if (auto self = w.lock()) {
                        self->set_value(expected<O, E>());
                    }
                });
            }
        }
    }

    Handler_t _handler;
};


template<typename T, typename E> struct extract_future_type { typedef T value_type; typedef E error_type; typedef std::false_type is_future; };
template<typename T, typename E> struct extract_future_type<future<T, E>, E> { typedef T value_type; typedef E error_type; typedef std::true_type is_future; };
template<typename T, typename E> struct extract_future_type<cancellable_future<T, E>, E> { typedef T value_type; typedef E error_type; typedef std::true_type is_future; };

template<typename handler_type, typename arg_type, typename error_type>
struct resolver {
    using raw_return_type = decltype(std::declval<handler_type>()(std::declval<arg_type>()));
    using return_type = typename extract_future_type<raw_return_type, error_type>::value_type;
    using is_future = typename extract_future_type<raw_return_type, error_type>::is_future;

    template <typename F>
    static auto make_continuation(F&& handler) {
        return std::make_shared<details::link<handler_type, arg_type, return_type, error_type>>(handler);
    }
};

template<typename handler_type, typename error_type>
struct resolver<handler_type, void, error_type> {
    using raw_return_type = decltype(std::declval<handler_type>()());
    using return_type = typename extract_future_type<raw_return_type, error_type>::value_type;
    using is_future = typename extract_future_type<raw_return_type, error_type>::is_future;

    template <typename F>
    static auto make_continuation(F&& handler) {
        return std::make_shared<details::link<handler_type, void, return_type, error_type>>(handler);
    }
};

} // namespace details

template <typename T, typename E>
class future {
public:
    using value_type = expected<T, E>;

    explicit future() {}
    explicit future(details::precursor_ptr<T, E> value) : _value(std::move(value)) {}

    template <typename TT = T>
    TT& get() & requires std::is_void_v<TT> {
        return *_value->get();
    }

    template <typename TT = T>
    TT const& get() const& requires std::negation_v<std::is_void<TT>> {
        return *_value->get();
    }

    template <typename TT = T>
    void get() requires std::is_void_v<TT> {
        *_value->get();
    }

    template <class Rep, class Period>
    optional<T> get(std::chrono::duration<Rep, Period> const& duration) {
        return _value->get(duration);
    }

    template<typename F, typename R = typename details::resolver<F, T, E>::return_type>
    future<R, E> then(F&& handler) {
        auto cont = details::resolver<F, T, E>::make_continuation(std::forward<F>(handler));
        _value->set_continuation(std::static_pointer_cast<details::continuation<T, E>>(cont));
        return future<R, E>(cont);
    }

private:
    details::precursor_ptr<T, E> _value;
};

template <typename T, typename E = std::exception_ptr>
class promise {
public:
    explicit promise() : _vc(std::make_shared<details::precursor<T,E>>()) {}
    promise(promise&& o) : _vc(std::move(o._continuation)) {}

    void set_value(T&& v) {
        assert(_vc);
        _vc->set_value(expected<T, E>(std::forward<T>(v)));
    }

    void set_value(T const& v) {
        assert(_vc);
        _vc->set_value(expected<T, E>(v));
    }

    void set_exception(E&& e) {
        assert(!_vc);
        _vc->set_value(unexpected(std::forward<E>(e)));
    }

    void set_exception(E const& e) {
        assert(!_vc);
        _vc->set_value(unexpected(e));
    }

    template <typename EE = E>
    void set_exception(EE&& e) {
        assert(!*_vc);
        if constexpr (std::is_base_of_v<std::exception, EE> && std::is_same_v<std::exception_ptr, E>) {
            _vc->set_value(unexpected(std::make_exception_ptr(e)));
        } else {
            _vc->set_value(unexpected(std::forward<E>(e)));
        }
    }

    future<T, E> get_future() {
        assert(_vc);
        return future<T, E>(_vc);
    }

private:
    details::precursor_ptr<T, E> _vc;
};

} // namespace hntr::platform

