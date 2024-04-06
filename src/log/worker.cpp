#include <cassert>
#include <chrono>
#include <map>
#include <string>

#include <bc/log/logger.hpp>
#include <bc/log/worker.hpp>

namespace bc::log {

auto worker::append(logger const &logger, record &&record) -> void {
    assert(!stop_);
    bool need_flush = false;
    {
        std::unique_lock lock(mutex_);
        logs_.emplace_back(logger, std::move(record));
        need_flush = logs_.size() > 20;
    }
    if (need_flush) {
        cv_.notify_one();
    }
}

auto worker::flush() -> void {
    assert(!stop_);
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic_bool flushed {false};
    {
        std::unique_lock lock(mutex_);
        waiters_.emplace_back(cv, flushed);
    }
    cv_.notify_one();

    std::unique_lock lock(mutex);
    cv_.wait(lock, [&] -> bool {
        return flushed;
    });
}

auto worker::loop_() -> void {
    while (!stop_) {
        logs ready;
        std::vector<waiter> waiters;
        {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(10), [this] {
                return !logs_.empty() || stop_;
            });
            logs_.swap(ready);
            waiters.swap(waiters_);
        }
        drain_(ready, waiters);
    }
    drain_(logs_, waiters_);
}

auto worker::drain_(logs &logs, std::vector<waiter> &waiters) -> void {
    std::map<logger const *, std::string> buffers;
    for (auto const &[logger, record] : logs) {
        if (!buffers.contains(&logger)) {
            logger.reference_();
        }
        buffers[&logger] += logger.formatter().format(record.timestamp, record.lv, record.topic, record.tid, record.location, record.message);
        buffers[&logger] += "\n";
        logger.unreference_();
    }
    for (auto const &[plogger, buffer] : buffers) {
        plogger->backend().write(buffer);
        plogger->unreference_();
    }
    for (auto &[cv, flushed] : waiters) {
        flushed = true;
        cv.notify_one();
    }
}

auto background_worker() -> worker & {
    static worker worker;
    return worker;
}

} /* namespace bc::log */
