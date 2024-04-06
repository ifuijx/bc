#pragma once

#ifndef __BC_LOG_BACKEND_H__
#define __BC_LOG_BACKEND_H__

#include <iostream>
#include <string>
#include <string_view>
#include <syncstream>
#include <utility>

#include <bc/utils/noncopyable.hpp>

namespace bc::log {

class backend : private utils::noncopyable {
public:
    backend() {}
    backend(std::string file) : file_(std::move(file)) {}

    auto write(std::string_view log) const -> void {
        if (file_.empty()) {
            std::osyncstream(std::cout) << log;
        }
    }

private:
    std::string file_;
};

} /* namespace bc::log */

#endif /* __BC_LOG_BACKEND_H__ */
