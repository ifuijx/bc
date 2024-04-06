#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#include "overload.hpp"

namespace bc::utils {

struct unexpect_t {
    explicit unexpect_t() = default;
};

inline constexpr unexpect_t unexpect {};

template <typename E>
class unexpected {
public:
    unexpected(unexpected const &) = default;
    unexpected(unexpected &&) = default;
    template <typename G = E>
    explicit unexpected(G &&e) : error_(std::forward<G>(e)) {}

    auto error() const & noexcept -> const E & {
        return error_;
    }
    auto error() & noexcept -> E & {
        return error_;
    }
    auto error() const && noexcept -> const E && {
        return error_;
    }
    auto error() && noexcept -> E && {
        return error_;
    }

private:
    E error_;
};

template <typename T, typename E>
class expected {
public:
    expected() = default;
    expected(expected &&other) : value_(std::move(other.value_)) {}
    template <typename U = T>
    requires std::convertible_to<U, T>
    expected(U &&v) : value_(std::in_place_index_t<0>(), std::forward<U>(v)) {}
    template <typename U = E>
    requires std::convertible_to<U, E>
    expected(U &&v) : value_(std::in_place_index_t<1>(), std::forward<U>(v)) {}
    template <typename ...Args>
    expected(std::in_place_t, Args &&...args) : value_(T(std::forward<Args>(args)...)) {}
    template <typename G = E>
    expected(unexpected<G> &&e) : value_(e.error()) {}
    template <typename ...Args>
    expected(unexpect_t, Args &&...args) : value_(E {std::forward<Args>(args)...}) {}

    template< class U = T >
    auto operator =(U &&v) -> expected & {
        value_ = std::forward<U>(v);
        return *this;
    }

    auto operator->() const noexcept -> T const * {
        assert(has_value());
        return std::get<T>(value_);
    }
    auto operator->() noexcept -> T * {
        assert(has_value());
        return std::get<T>(value_);
    }

    auto operator *() const & noexcept -> T const & {
        assert(has_value());
        return std::get<T>(value_);
    }
    auto operator *() & noexcept -> T & {
        assert(has_value());
        return std::get<T>(value_);
    }
    auto operator *() const && noexcept -> T const && {
        assert(has_value());
        return std::get<T>(std::move(value_));
    }
    auto operator *() && noexcept -> T && {
        assert(has_value());
        return std::get<T>(std::move(value_));
    }

    auto has_value() const -> bool {
        return std::visit(overload([](T const &) {
            return true;
        }, [](E const &) {
            return false;
        }), value_);
    }

    explicit operator bool() const {
        return has_value();
    }

    auto value() & -> T & {
        assert(has_value());
        return std::get<T>(value_);
    }
    auto value() const & -> T const & {
        assert(has_value());
        return std::get<T>(value_);
    }

    auto error() & -> E & {
        assert(!has_value());
        return std::get<E>(value_);
    }
    auto error() const & -> E const & {
        assert(!has_value());
        return std::get<E>(value_);
    }

    template <typename U>
    auto value_or(U &&value) const & -> T {
        if (has_value()) {
            return **this;
        }
        return T {value};
    }
    template <typename U>
    auto value_or(U &&value) && -> T {
        if (has_value()) {
            return **this;
        }
        return T {value};
    }

    template <typename F>
    auto and_then(F &&f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(**this)>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        }
        else {
            return U(unexpect, error());
        }
    }
    template <typename F>
    auto and_then(F &&f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(**this)>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        }
        else {
            return U(unexpect, error());
        }
    }
    template <typename F>
    auto and_then(F &&f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(**this)>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        }
        else {
            return U(unexpect, error());
        }
    }
    template <typename F>
    auto and_then(F &&f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(**this)>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        }
        else {
            return U(unexpect, error());
        }
    }

private:
    std::variant<T, E> value_ = T {};
};

} /* namespace bc::utils */
