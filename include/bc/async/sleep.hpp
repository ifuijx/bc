#pragma once

#ifndef __BC_ASYNC_SLEEP_H__
#define __BC_ASYNC_SLEEP_H__

#include <chrono>
#include <coroutine>
#include <thread>

#include <bc/log/log.hpp>

#include "scheduler.hpp"

namespace bc::async {

namespace detail {

class async_sleep_awaiter {
    using time_point = decltype(std::chrono::steady_clock::now());

public:
    template <typename Duration>
    async_sleep_awaiter(Duration period) : tp_(std::chrono::steady_clock::now() + period) {}

    auto await_ready() -> bool {
        return std::chrono::steady_clock::now() >= tp_;
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept {
        default_scheduler().post_coro(tp_, handle);
        return true;
    }

    auto await_resume() noexcept -> void {}

private:
    time_point tp_;
    std::coroutine_handle<> handle_;
};

} /* namespace bc::async::detail */

template <typename Rep, typename Period>
auto async_sleep(std::chrono::duration<Rep, Period> rtime) -> detail::async_sleep_awaiter {
    return {rtime};
}

} /* namespace bc::async */

#endif /* __BC_ASYNC_SLEEP_H__ */
