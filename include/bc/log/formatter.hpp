#pragma once

#ifndef __BC_LOG_FORMATTER_H__
#define __BC_LOG_FORMATTER_H__

#include <chrono>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <fmt/chrono.h>

#include <bc/utils/overload.hpp>

#include "record.hpp"

namespace bc::log {

auto level_string(level level) -> std::string_view;

class formatter {
    enum class meta_type {
        DATETIME,
        LEVEL,
        TOPIC,
        THREAD,
        LOCATION,
        MESSAGE,
    };

public:
    explicit formatter(std::string format);

    template <typename Duration>
    auto format(std::chrono::time_point<std::chrono::system_clock, Duration> timestamp, level level, std::string_view topic, pid_t tid, std::source_location location, std::string_view message) const -> std::string {
        std::string log;
        for (auto const &segment : segments_) {
            std::visit(utils::overload([&](std::string_view segment) {
                log += segment;
            }, [&](meta_type meta) {
                switch (meta) {
                    case meta_type::DATETIME: {
                        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
                        log += fmt::format("{:%F %H:%M:}{:%S}", timestamp, ms);
                        break;
                    }
                    case meta_type::LEVEL: {
                        log += level_string(level);
                        break;
                    };
                    case meta_type::TOPIC: {
                        log += topic;
                        break;
                    }
                    case meta_type::THREAD: {
                        log += fmt::format("{}", tid);
                        break;
                    }
                    case meta_type::LOCATION: {
                        log += fmt::format("{}:{}", location.file_name(), location.line());
                        break;
                    }
                    case meta_type::MESSAGE: {
                        log += message;
                        break;
                    }
                }
            }), segment);
        }
        while (log.back() == '\n') {
            log.pop_back();
        }
        return log;
    }

    auto repr() const -> std::string;

private:
    std::string format_;
    std::vector<std::variant<std::string_view, meta_type>> segments_;
};

} /* namespace bc::log */

#endif /* __BC_LOG_FORMATTER_H__ */
