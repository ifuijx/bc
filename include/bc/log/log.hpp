#pragma once

#ifndef __BC_LOG_H__
#define __BC_LOG_H__

#include <source_location>
#include <utility>
#include <fmt/format.h>

#include "backend.hpp"
#include "formatter.hpp"
#include "logger.hpp"
#include "record.hpp"
#include "worker.hpp"

namespace bc::log {

template <typename ...Ts>
struct debug {
    debug(logger &logger, fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) {
        logger.log(level::DEBUG, location, fmt, std::forward<Ts>(args)...);
    }

    debug(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current())
        : debug(default_logger(), fmt, std::forward<Ts>(args)..., location) {}

    // debug(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current())
    // {
    //     fmt::print(fmt, std::forward<Ts>(args)...);
    //     fmt::print("\n");
    // }
};

template <typename ...Ts>
debug(logger &, fmt::format_string<Ts...>, Ts &&...) -> debug<Ts...>;

template <typename ...Ts>
debug(fmt::format_string<Ts...>, Ts &&...) -> debug<Ts...>;

template <typename ...Ts>
struct info {
    info(logger &logger, fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) {
        logger.log(level::INFO, location, fmt, std::forward<Ts>(args)...);
    }

    info(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current())
        : info(default_logger(), fmt, std::forward<Ts>(args)..., location) {}
};

template <typename ...Ts>
info(logger &, fmt::format_string<Ts...>, Ts &&...) -> info<Ts...>;

template <typename ...Ts>
info(fmt::format_string<Ts...>, Ts &&...) -> info<Ts...>;

template <typename ...Ts>
struct warning {
    warning(logger &logger, fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) {
        logger.log(level::WARNING, location, fmt, std::forward<Ts>(args)...);
    }

    warning(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) 
        : warning(default_logger(), fmt, std::forward<Ts>(args)..., location) {}
};

template <typename ...Ts>
warning(logger &, fmt::format_string<Ts...>, Ts &&...) -> warning<Ts...>;

template <typename ...Ts>
warning(fmt::format_string<Ts...>, Ts &&...) -> warning<Ts...>;

template <typename ...Ts>
struct error {
    error(logger &logger, fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) {
        logger.log(level::ERROR, location, fmt, std::forward<Ts>(args)...);
    }

    error(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current())
        : error(default_logger(), fmt, std::forward<Ts>(args)..., location) {}
};

template <typename ...Ts>
error(logger &, fmt::format_string<Ts...>, Ts &&...) -> error<Ts...>;

template <typename ...Ts>
error(fmt::format_string<Ts...>, Ts &&...) -> error<Ts...>;

template <typename ...Ts>
struct fatal {
    fatal(logger &logger, fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current()) {
        logger.log(level::FATAL, location, fmt, std::forward<Ts>(args)...);
    }

    fatal(fmt::format_string<Ts...> fmt, Ts &&...args, std::source_location location = std::source_location::current())
        : fatal(default_logger(), fmt, std::forward<Ts>(args)..., location) {}
};

template <typename ...Ts>
fatal(logger &, fmt::format_string<Ts...>, Ts &&...) -> fatal<Ts...>;

template <typename ...Ts>
fatal(fmt::format_string<Ts...>, Ts &&...) -> fatal<Ts...>;

} /* namespace bc::log */

#endif /* __BC_LOG_H__ */
