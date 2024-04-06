#include <bits/chrono.h>
#include <sys/epoll.h>
#include <cerrno>

#include <bc/async/scheduler.hpp>
#include <thread>

namespace bc::async {

auto poller::subscribe(int fd, event e) -> void {
    log::debug("subscribe, fd: {}, event: {}", fd, e);
    assert(fd > 0);
    adjust_size_(fd);

    auto op = [&](int fd, int target) {
        int now = focus_[fd];
        if (focus_[fd] == target) {
            return 0;
        }
        if (focus_[fd]) {
            return target ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        }
        return EPOLL_CTL_ADD;
    }(fd, e);

    epoll_event ev {
        .events = e,
        .data {
            .fd = fd,
        },
    };
    if (op && ::epoll_ctl(epfd_, op, fd, &ev) == -1) {
        log::error("epoll_ctl failed, op: {}, fd: {}, event: {}", op, fd, e);
        throw utils::trans_error_code(errno);
    }
    focus_[fd] = e;
}

auto scheduler::run() -> void {
    while (coro_count_) {
        log::debug("one iteration of scheduler, time coroutines count: {}, fd coroutines count: {}", time_nodes_.size(), coro_count_ - time_nodes_.size());
        if (handle_expired_time_nodes_()) {
            continue;
        }
        auto period = [&] {
            auto now = std::chrono::steady_clock::now();
            auto default_period = std::chrono::duration_cast<duration>(s_period);
            if (!time_nodes_.empty() && time_nodes_.top().time <= now + default_period) {
                return time_nodes_.top().time - now;
            }
            return default_period;
        }();
        if (coro_count_ > time_nodes_.size()) {
            handle_triggered_descriptor_nodes_(period);
        }
        else {
            std::this_thread::sleep_for(period);
        }
    }
}

auto scheduler::post_coro(int fd, event e, event &re, std::coroutine_handle<> coro) -> void {
    log::debug("post coroutine, fd: {}, events: {}", fd, e);
    assert(fd > 0);
    adjust_size_(fd);
    descriptor_nodes_[fd].emplace_back(e, re, coro);
    update_descriptor_(fd);
    ++coro_count_;
}

auto scheduler::update_descriptor_(int fd) -> void {
    event e = NONE;
    for (auto &node : descriptor_nodes_[fd]) {
        e |= node.ev;
    }
    poller_.subscribe(fd, e);
}

auto scheduler::handle_expired_time_nodes_() -> bool {
    size_t expired = 0;
    while (!time_nodes_.empty() && time_nodes_.top().time <= std::chrono::steady_clock::now()) {
        assert(!time_nodes_.top().next.done());
        auto handle = time_nodes_.top().next;
        time_nodes_.pop();
        handle.resume();
        ++expired;
        --coro_count_;
    }
    if (expired) {
        log::debug("handle {} expired time node(s)", expired);
    }
    return expired > 0;
}

auto default_scheduler() -> scheduler & {
    static scheduler s_scheduler;
    return s_scheduler;
}

} /* namespace bc::async */
