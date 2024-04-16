#include <functional>
#include <iostream>
#include <string>
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
    // log::default_logger().set_level(bc::log::level::DEBUG);

    network::client<network::protocol::TCP> client;
    auto t = client.async_connect("127.0.0.1", 12345, [](network::socket<network::protocol::TCP> &sock) -> task<> {
        string line;
        while (getline(cin, line)) {
            auto write_res = co_await network::async_write(sock, line);
            if (write_res) {
                array<char, 1024> buffer;
                auto read_res = co_await network::async_read(sock, buffer);
                if (read_res) {
                    buffer[*read_res] = 0;
                    cout << buffer.data() << endl;
                }
                else {
                    log::error("unexpected read error, message: {}", read_res.error().message());
                    break;
                }
            }
            else {
                log::error("unexpected write error, message: {}", write_res.error().message());
                break;
            }
        }
    });
    default_scheduler().run();
}
