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

auto main() -> int {
    log::default_logger().set_level(bc::log::level::DEBUG);

    network::server<network::protocol::TCP, network::domain::IPv4> server("127.0.0.1"sv, 12345);
    server.start([](network::socket<network::protocol::TCP> &sock) -> async::task<> {
        while (true) {
            array<char, 1024> buffer;
            auto read_res = co_await network::async_read(sock, buffer);
            if (read_res) {
                auto write_res = co_await network::async_write(sock, {buffer.data(), read_res.value()});
                if (!write_res) {
                    log::error("unexpected write error, message: {}", read_res.error().message());
                    break;
                }
            }
            else {
                log::error("unexpected read error, message: {}", read_res.error().message());
                break;
            }
        }
    });

    default_scheduler().run();
}
