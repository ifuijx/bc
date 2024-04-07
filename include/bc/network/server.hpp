#pragma once

#ifndef __BC_NETWORK_SERVER_H__
#define __BC_NETWORK_SERVER_H__

#include <cstdint>
#include <functional>
#include <list>

#include <bc/utils/noncopyable.hpp>
#include <bc/async/task.hpp>

#include "socket.hpp"

namespace bc::network {

template <protocol Protocol, domain Domain>
class server : private utils::noncopyable {
    constexpr static std::size_t s_backlog = 100;

    struct client {
        socket<Protocol> sock;
        async::task<> task;
    };

public:
    server(std::string_view hostname, uint16_t port) : address_(hostname, port) {}

    template <typename F>
    auto start(F &&f) -> void {
        handler_ = std::forward<F>(f);
        task_ = std::move(run_());
    }

private:
    auto run_() -> async::task<> {
        socket<Protocol> sock;
        sock.listen(address_, s_backlog);
        while (true) {
            auto res = co_await async_accept(sock);
            if (res) {
                add_client_(*std::move(res));
            }
            else {
                log::error("unexpected error, message: {}", res.error().message());
                break;
            }
            auto it = clients_.begin();
            while (it != clients_.end()) {
                if (it->task.done()) {
                    clients_.erase(it++);
                    log::info("remove a client\n");
                }
                else {
                    log::info("has clients: {}\n", clients_.size());
                    ++it;
                }
            }
        }
    }

    auto add_client_(socket<Protocol> &&client_sock) -> void {
        clients_.emplace_back(std::move(client_sock));
        clients_.back().task = std::move(async_session_(clients_.back().sock));
    }

    auto async_session_(socket<Protocol> &client_sock) -> async::task<> {
        auto session = handler_(client_sock);
        while (!session.done()) {
            co_await session;
        }
    }

private:
    address<Domain> address_;
    std::function<auto (socket<Protocol> &) -> async::task<>> handler_;
    async::task<> task_;
    std::list<client> clients_;
};

} /* namespace bc::network */

#endif /* __BC_NETWORK_SERVER_H__ */
