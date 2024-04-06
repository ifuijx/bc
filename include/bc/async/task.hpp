#pragma once

#ifndef __BC_ASYNC_TASK_H__
#define __BC_ASYNC_TASK_H__

#include <coroutine>
#include <exception>

#include <bc/log/log.hpp>

#include "scheduler.hpp"

namespace bc::async {

template <typename T = void>
struct promise {
    using coro_handle = std::coroutine_handle<promise>;

    struct final_awaiter {
        auto await_ready() noexcept -> bool { return false; }
        auto await_suspend(coro_handle handle) noexcept -> std::coroutine_handle<> {
            auto prev = handle.promise().prev;
            if (prev) {
                return prev;
            }
            return std::noop_coroutine();
        }
        auto await_resume() noexcept {}
    };

    auto get_return_object() -> std::coroutine_handle<promise> {
        return std::coroutine_handle<promise>::from_promise(*this);
    }

    auto initial_suspend() noexcept -> std::suspend_never {
        return {};
    }

    auto final_suspend() noexcept -> final_awaiter {
        return {};
    }

    auto return_value(T value) -> void {
        result = std::move(value);
    }

    auto unhandled_exception() -> void {
        std::terminate();
    }

    T result;
    std::coroutine_handle<> prev;
};

template <>
struct promise<void> {
    using coro_handle = std::coroutine_handle<promise>;

    struct final_awaiter {
        auto await_ready() noexcept -> bool { return false; }
        auto await_suspend(coro_handle handle) noexcept -> std::coroutine_handle<> {
            auto prev = handle.promise().prev;
            if (prev) {
                return prev;
            }
            return std::noop_coroutine();
        }
        auto await_resume() noexcept {}
    };

    auto get_return_object() -> std::coroutine_handle<promise> {
        return std::coroutine_handle<promise>::from_promise(*this);
    }

    auto initial_suspend() noexcept -> std::suspend_never {
        return {};
    }

    auto final_suspend() noexcept -> final_awaiter {
        return {};
    }

    auto return_void() -> void {}

    auto unhandled_exception() -> void {
        std::terminate();
    }

    std::coroutine_handle<> prev;
};

template <typename T = void>
class task {
public:
    using promise_type = promise<T>;
    using coro_handle = std::coroutine_handle<promise_type>;

    class task_awaiter {
    public:
        task_awaiter(coro_handle handle) : handle_(handle) {}

        auto await_ready() noexcept -> bool {
            return handle_.done();
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            handle_.promise().prev = handle;
        }

        auto await_resume() noexcept -> void {}

    private:
        std::coroutine_handle<promise<T>> handle_;
    };

    task() {}
    task(coro_handle handle) : handle_(handle) { assert(handle_); }
    task(task const &) = delete;
    task(task &&other) : handle_(other.handle_) { other.handle_ = nullptr; }
    auto operator =(task &&other) noexcept -> task & {
        assert(!handle_);
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    auto operator co_await() -> task_awaiter {
        return task_awaiter {handle_};
    }

    auto operator()() -> T
    {
        if (!handle_.done()) {
            handle_.resume();
        }
        return std::move(handle_.promise().result);
    }

    auto done() const -> bool {
        return handle_.done();
    }

private:
    coro_handle handle_;
};

} /* namespace bc::async */

#endif /* __BC_ASYNC_TASK_H__ */
