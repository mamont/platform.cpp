#include "expected.hpp"
#include <exception>

namespace hntr::platform {

template <typename T, typename E>
class future {
public:
    using value_type = expected<T, E>;

    explicit future(value_type&& value) 
        : _value(std::move(value))
    {}

    T& get() { 
        return *_value;
    }

private:
    value_type _value;
};

template <typename T, typename E = std::exception_ptr>
class promise {
public:
    void set_value(T&& v) noexcept {
        _value = expected<T, E>(v);
    }

    void set_value(T const& v) {
        _value = expected<T, E>(v);
    }

    void set_exception(E&& e) {
        _value = unexpected(std::forward<E>(e));
    }

    void set_exception(E const& e) {
        _value = unexpected(e);
    }

    template <class TT = E>
    void set_exception(TT&& e) {
        _value = unexpected(std::make_exception_ptr(std::forward<TT>(e)));
    }

    future<T, E> get_future() {
        return future<T, E>(std::move(_value));
    }

private:
    expected<T, E> _value;
};

} // namespace hntr::platform

