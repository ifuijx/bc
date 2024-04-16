#include <functional>
#include <sys/socket.h>
#include <array>
#include <list>
#include <fmt/core.h>
#include <bc/core.hpp>

using namespace std;
using namespace std::chrono_literals;
using namespace bc;
using namespace bc::async;

auto async_sleep_loop() -> task<> {
    fmt::print("async run\n");
    for (size_t i = 0; i < 5; ++i) {
        co_await async_sleep(1s);
        fmt::print("waked loop\n");
    }
    fmt::print("loop finished\n");
    for (size_t i = 0; i < 5; ++i) {
        co_await async_sleep(1s);
        fmt::print("waked loop 2\n");
    }
}

auto async_run() -> task<> {
    auto t = async_sleep_loop();
    co_await t;
    fmt::print("async run finished\n");
}

auto main() -> int {
    // log::default_logger().set_level(bc::log::level::DEBUG);

    auto t = async_run();

    default_scheduler().run();
}
