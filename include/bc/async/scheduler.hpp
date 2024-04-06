#pragma once

#include <bit>
#include <bits/chrono.h>
#ifndef __BC_ASYNC_SCHEDULER_H__
#define __BC_ASYNC_SCHEDULER_H__

#include <sys/epoll.h>
#include <sys/types.h>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <compare>
#include <coroutine>
#include <cstddef>
#include <cstring>
#include <functional>
#include <list>
#include <queue>
#include <ranges>
#include <set>
#include <system_error>
#include <thread>
#include <variant>
#include <vector>

#include <bc/utils/error.hpp>
#include <bc/utils/noncopyable.hpp>
#include <bc/log/log.hpp>

namespace bc::network {

enum class protocol;

template <protocol>
class socket;

}

namespace bc::async {

using event = u_int32_t;

constexpr event NONE = 0;
constexpr event READ = EPOLLIN;
constexpr event WRITE = EPOLLOUT;
constexpr event ERROR = EPOLLERR;
constexpr event HANGUP = EPOLLHUP;
constexpr event RDHANGUP = EPOLLRDHUP;

class poller {
public:
    poller() {
        epfd_ = ::epoll_create1(EPOLL_CLOEXEC);
        if (epfd_ == -1) {
            log::error("epoll_create1 failed, errno: {}, message: {}", errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
    }
    ~poller() noexcept {
        if (::close(epfd_) == -1) {
            log::fatal("failed to close epoll fd, fd: {}, errno: {}, message: {}", epfd_, errno, ::strerror(errno));
        }
    }

    auto subscribe(int fd, event e) -> void;

    auto unsubscribe(int fd) -> void {
        subscribe(fd, NONE);
    }

    template <typename Rep, typename Period, typename EventHandler>
    auto poll(std::chrono::duration<Rep, Period> rtime, EventHandler &&handler) -> void {
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(rtime).count();
        if (timeout < 0) {
            return;
        }
        auto nfds = ::epoll_wait(epfd_, evs_.data(), evs_.size(), timeout);
        if (nfds == -1) {
            if (errno == EINTR) {
                log::info("epoll_wait was interrupted");
                return;
            }
            log::error("epoll_wait failed, errno: {}, message: {}", errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
        for (auto &ev : evs_ | std::ranges::views::take(nfds)) {
            log::debug("got epoll event, fd: {}, event: {}", ev.data.fd, ev.events);
            handler(ev.data.fd, static_cast<event>(ev.events));
        }
    }

private:
    auto adjust_size_(size_t index) -> void {
        if (focus_.size() > index) {
            return;
        }
        auto need = std::bit_ceil(index);
        log::debug("poller adjust size to {}", need);
        focus_.resize(need);
        evs_.resize(need, {
            .events = NONE,
            .data {
                .fd = -1,
            },
        });
    }

private:
    int epfd_;
    std::vector<event> focus_;
    std::vector<epoll_event> evs_;
};

class scheduler : utils::noncopyable {
    template <network::protocol>
    friend class network::socket;

    using time_point = decltype(std::chrono::steady_clock::now());
    using duration = time_point::duration;

    struct time_node {
        time_point time;
        std::coroutine_handle<> next;

        auto operator<=>(time_node const &other) const -> std::partial_ordering {
            return time <=> other.time;
        }
    };

    struct descriptor_node {
        event ev;
        event &revent;
        std::variant<std::coroutine_handle<>, std::function<auto () -> bool>> next;
    };

public:
    constexpr static auto s_period = std::chrono::seconds(1);

public:
    auto run() -> void;

    template <typename Duration>
    auto post_coro(std::chrono::time_point<std::chrono::steady_clock, Duration> tp, std::coroutine_handle<> coro) -> void {
        time_nodes_.emplace(std::chrono::time_point_cast<duration>(tp), coro);
        ++coro_count_;
    }
    auto post_coro(int fd, event e, event &re, std::coroutine_handle<> coro) -> void;
    template <typename Proxy>
    auto post_coro(int fd, event e, event &re, Proxy &&proxy) -> void {
        log::debug("post proxy, fd: {}, events: {}", fd, e);
        assert(fd > 0);
        adjust_size_(fd);
        descriptor_nodes_[fd].emplace_back(e, re, proxy);
        update_descriptor_(fd);
        ++coro_count_;
    }

private:
    auto adjust_size_(size_t index) -> void {
        if (descriptor_nodes_.size() > index) {
            return;
        }
        auto need = std::bit_ceil(index);
        log::debug("scheduler adjust size to {}", need);
        descriptor_nodes_.resize(need);
    }

    auto update_descriptor_(int fd) -> void;
    auto handle_expired_time_nodes_() -> bool;

    template <typename Rep, typename Period>
    auto handle_triggered_descriptor_nodes_(std::chrono::duration<Rep, Period> rtime) -> void {
        auto handler = [&](int fd, event e) {
            std::list<descriptor_node> list;
            list.splice(list.end(), descriptor_nodes_[fd]);
            auto it = list.begin();
            log::debug("coroutines of fd before resume: {}, count: {}", fd, list.size());
            while (it != list.end()) {
                if (it->ev & e) {
                    it->revent = e;
                    bool finished = std::visit(utils::overload([](std::coroutine_handle<> handle) {
                        handle.resume();
                        return true;
                    }, [](auto &proxy) {
                        return proxy();
                    }), it->next);
                    log::debug("resume coroutine / proxy, fd: {}, events: {}, revent: {}, finished: {}", fd, it->ev, e, finished);
                    if (finished) {
                        list.erase(it++);
                        --coro_count_;
                    }
                }
                else {
                    ++it;
                }
            }
            descriptor_nodes_[fd].splice(descriptor_nodes_[fd].begin(), list);
            log::debug("coroutines of fd after resume: {}, count: {}", fd, descriptor_nodes_[fd].size());
            update_descriptor_(fd);
        };
        poller_.poll(rtime, handler);
    }

    auto subscribe(int fd, event e) -> void {
        poller_.subscribe(fd, e);
    }

    auto unsubscribe(int fd) -> void {
        poller_.unsubscribe(fd);
    }

private:
    size_t coro_count_ {0};
    std::priority_queue<time_node, std::vector<time_node>, std::greater<time_node>> time_nodes_;
    std::vector<std::list<descriptor_node>> descriptor_nodes_;
    poller poller_;
};

auto default_scheduler() -> scheduler &;

} /* namespace bc::async */

#endif /* __BC_ASYNC_SCHEDULER_H__ */
