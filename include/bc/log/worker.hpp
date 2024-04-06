#pragma once

#ifndef __BC_LOG_WORKER_H__
#define __BC_LOG_WORKER_H__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <bc/utils/noncopyable.hpp>

#include "record.hpp"

namespace bc::log {

class logger;

class worker : private utils::noncopyable {
    using logs = std::vector<std::pair<logger const &, record>>;
    using waiter = std::pair<std::condition_variable &, std::atomic_bool &>;

public:
    worker() : thread_(&worker::loop_, this) {}
    ~worker() {
        if (!stop_) {
            stop_ = true;
            cv_.notify_one();
        }
    }

    auto append(logger const &logger, record &&record) -> void;
    auto flush() -> void;

private:
    auto drain_(logs &logs, std::vector<waiter> &waiters) -> void;
    auto loop_() -> void;

private:
    logs logs_;
    std::vector<waiter> waiters_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::jthread thread_;
    std::atomic_bool stop_ {false};
    std::atomic_uint64_t length_ {0};
};

auto background_worker() -> worker &;

} /* namespace bc::log */

#endif /* __BC_LOG_WORKER_H__ */
