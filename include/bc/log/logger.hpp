#pragma once

#ifndef __BC_LOG_LOGGER_H__
#define __BC_LOG_LOGGER_H__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <source_location>
#include <fmt/format.h>

#include <bc/utils/noncopyable.hpp>

#include "backend.hpp"
#include "formatter.hpp"
#include "record.hpp"
#include "worker.hpp"

namespace bc::log {

class logger : private utils::noncopyable {
    friend class worker;

public:
    static auto default_format() -> std::string_view {
        using namespace std::string_view_literals;
        return "[%{datetime}] [%{level}] [%{thread}] [%{location}] %{message}"sv;
    }

public:
    explicit logger(std::string format = std::string {default_format()}, std::string topic = "")
        : topic_(std::move(topic)), formatter_(std::move(format)) {}
    ~logger() {
        if (ref_) {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [&] {
                return ref_ == 0;
            });
        }
    }

    template <typename ...Ts>
    auto log(level level, std::source_location location, fmt::format_string<Ts...> fmt, Ts &&...args) -> void {
        if (level < level_) {
            return;
        }
        ++ref_;
        background_worker().append(*this, record {
            .timestamp = std::chrono::system_clock::now(),
            .level = level,
            .topic = topic_,
            .tid = gettid(),
            .location = location,
            .message = fmt::format(fmt, std::forward<Ts>(args)...),
        });
    }

    auto flush() -> void { background_worker().flush(); }
    auto set_level(level level) -> void { level_ = level; }
    auto formatter() const -> formatter const & { return formatter_; }
    auto backend() const -> backend const & { return backend_; }

private:
    auto reference_() const -> void { ++ref_; }
    auto unreference_() const -> void {
        if (--ref_ == 0) {
            cv_.notify_one();
        }
    }

private:
    std::string topic_;
    level level_ {level::WARNING};
    log::formatter formatter_;
    log::backend backend_;

    mutable std::atomic_uint64_t ref_;
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
};

auto default_logger() -> logger &;

} /* namespace bc::log */

#endif /* __BC_LOG_LOGGER_H__ */
