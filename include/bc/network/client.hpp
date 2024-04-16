#pragma once

#ifndef __BC_NETWORK_CLIENT_H__
#define __BC_NETWORK_CLIENT_H__

#include <functional>
#include <string_view>

#include <bc/utils/noncopyable.hpp>
#include <bc/async/task.hpp>

#include "socket.hpp"

namespace bc::network {

template <protocol Protocol>
class client : private utils::noncopyable {
public:
    template <typename F>
    auto async_connect(std::string_view hostname, uint16_t port, F &&f) -> async::task<> {
        co_await sock_.async_connect({hostname, port});
        handler_ = std::forward<F>(f);
        task_ = handler_(sock_);
    }

private:
    socket<Protocol> sock_;
    std::function<auto (socket<Protocol> &sock) -> async::task<>> handler_;
    async::task<> task_;
};

} /* namespace bc::network */

#endif /* __BC_NETWORK_CLIENT_H__ */
