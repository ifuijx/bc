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

template <network::protocol proto>
auto async_session(network::socket<proto> &sock) -> task<> {
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
}

struct client {
    network::socket<network::protocol::TCP> sock;
    task<> t;
};

auto add_client(list<client> &clients, network::socket<network::protocol::TCP> &&sock) {
    clients.emplace_back(std::move(sock));
    clients.back().t = std::move(async_session(clients.back().sock));
}

auto async_server() -> task<> {
    list<client> clients;
    network::address<network::domain::IPv4> address("127.0.0.1"sv, 12345);
    auto sock = network::socket<network::protocol::TCP>{};
    sock.listen(address, 10);
    while (true) {
        auto client = co_await network::async_accept(sock);
        if (client) {
            if (client) {
                add_client(clients, *std::move(client));
            }
        }
        else {
            log::error("unexpected error, message: {}", client.error().message());
            break;
        }
        auto it = clients.begin();
        while (it != clients.end()) {
            if (it->t.done()) {
                clients.erase(it++);
                fmt::print("remove a client\n");
            }
            else {
                fmt::print("has clients: {}\n", clients.size());
                ++it;
            }
        }
    }
}

auto main() -> int {
    // log::default_logger().set_level(bc::log::level::DEBUG);

    auto t = async_server();

    default_scheduler().run();
}
