#pragma once

#ifndef __BC_LOG_RECORD_H__
#define __BC_LOG_RECORD_H__

#include <sys/types.h>

#include <chrono>
#include <compare>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace bc::log {

enum class level {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4,
};

inline auto operator <=>(level l, level r) -> std::strong_ordering {
    return std::to_underlying(l) <=> std::to_underlying(r);
}

struct record {
    std::chrono::system_clock::time_point timestamp;
    level level;
    std::string_view topic;
    pid_t tid;
    std::source_location location;
    std::string message;
};

} /* namespace bc::log */

#endif /* __BC_LOG_RECORD_H__ */
