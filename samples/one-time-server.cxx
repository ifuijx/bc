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

auto async_one_time_server() -> task<> {
    list<network::socket<network::protocol::TCP>> clients;
    list<task<>> tasks;
    network::address address("127.0.0.1"sv, 12345);
    network::socket<network::protocol::TCP> sock;
    sock.listen(address, 10);
    auto client = co_await network::async_accept(sock);
    if (client) {
        while (true) {
            log::debug("accepted");
            array<char, 1024> buffer;
            auto n = co_await network::async_read(*client, buffer);
            log::debug("read");
            if (n) {
                co_await network::async_write(*client, {buffer.data(), n.value()});
                log::debug("wrote");
            }
            else {
                break;
            }
        }
    }
    else {
        log::error("failed to accept, message: {}", client.error().message());
    }
}

auto main() -> int {
    // log::default_logger().set_level(bc::log::level::DEBUG);

    auto t = async_one_time_server();

    default_scheduler().run();
}
