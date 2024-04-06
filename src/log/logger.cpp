#include <utility>

#include <bc/log/logger.hpp>
#include <bc/log/worker.hpp>

namespace bc::log {

auto default_logger() -> logger & {
    static logger logger;
    return logger;
}

} /* namespace bc::log */
