#pragma once

#include <asm-generic/errno.h>
#include <optional>
#ifndef __BC_NETWORK_SOCKET_H__
#define __BC_NETWORK_SOCKET_H__

#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <coroutine>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <bc/utils/expected.hpp>
#include <bc/utils/error.hpp>
#include <bc/utils/noncopyable.hpp>
#include <bc/async/task.hpp>
#include <bc/async/scheduler.hpp>

#include "address.hpp"

namespace bc::network {

enum class protocol {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
};

enum class role {
    UNDETERMINED,
    SERVER,
    PEER,
};

template <protocol proto>
class socket : private bc::utils::noncopyable {
public:
    static auto wrap(int fd, domain domain, role role) -> socket {
        socket socket;
        socket.fd_ = fd;
        socket.domain_ = domain;
        socket.role_ = role;
        return socket;
    }

public:
    socket() = default;
    socket(socket &&other) : fd_(other.fd_), domain_(other.domain_), role_(other.role_) {
        other.clear_();
    }

    auto operator=(socket &&other) -> socket & {
        if (this != &other) {
            assert(fd_ == 0);
            assert(role_ == role::UNDETERMINED);

            fd_ = other.fd_;
            domain_ = other.domain_;
            role_ = other.role_;

            other.clear_();
        }
        return *this;
    }

    ~socket() {
        if (fd_) {
            log::debug("socket {} destroy", fd_);
            async::default_scheduler().unsubscribe(fd_);
            if (::close(fd_) == -1) {
                log::error("failed to close socket fd, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
            }
        }
    }

    auto bind(address const &addr) -> void {
        use_domain_(addr.domain());
        int reuse = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
            log::error("failed to set reuse option, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
        if (::bind(fd_, addr.sockaddr(), addr.socklen()) == -1) {
            log::error("failed to bind socket to {}, fd: {}, errno: {}, message: {}", addr, fd_, errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
    }

    auto listen(std::size_t backlog) -> void {
        assert(role_ == role::UNDETERMINED);
        if (::listen(fd_, backlog) == -1) {
            log::error("failed to listen socket, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
        role_ = role::SERVER;
    }

    auto listen(address const &addr, std::size_t backlog) -> void {
        bind(addr);
        listen(backlog);
    }

    template <domain Domain>
    auto connect(address const &addr) -> void {
        if (fd_ == 0) {
            use_domain_(Domain);
        }
        else {
            assert(domain_ == Domain);
            assert(role_ == role::UNDETERMINED);
        }
        if (::connect(fd_, addr.sockaddr(), addr.socklen()) == -1) {
            log::error("failed to connect socket to {}, fd: {}, errno: {}, message: {}", addr, fd_, errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
        role_ = role::PEER;
    }

    auto read(std::span<char> buffer) -> utils::expected<std::size_t, std::error_code> {
        auto res = ::read(fd_, buffer.data(), buffer.size());
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                log::info("read returns negligible error, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
                return 0;
            }
            log::error("failed to read, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
            return utils::trans_error_code(errno);
        }
        return res;
    }

    auto write(std::string_view data) -> utils::expected<std::size_t, std::error_code> {
        auto res = ::write(fd_, data.data(), data.size());
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                log::info("write returns negligible error, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
                return 0;
            }
            log::error("failed to write, fd: {}, errno: {}, message: {}", fd_, errno, ::strerror(errno));
            return utils::trans_error_code(errno);
        }
        return res;
    }

    auto descriptor() const -> int { return fd_; }
    auto domain() const -> network::domain { return domain_; }

private:
    auto clear_() -> void {
        fd_ = 0;
        role_ = role::UNDETERMINED;
    }

    auto use_domain_(network::domain domain) -> void {
        assert(fd_ == 0);
        fd_ = ::socket(std::to_underlying(domain), std::to_underlying(proto) | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd_ == -1) {
            log::error("failed to create socket, errno: {}, message: {}", errno, ::strerror(errno));
            throw utils::trans_error_code(errno);
        }
        domain_ = domain;
    }

private:
    int fd_ {0};
    network::domain domain_;
    role role_ {role::UNDETERMINED};
};

namespace detail {

template <protocol proto>
class async_accept_awaiter {
public:
    async_accept_awaiter(socket<proto> &sock) : sock_(sock) {}

    auto await_ready() -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept {
        async::default_scheduler().post_coro(sock_.descriptor(), async::READ, revent_, [&, next=handle] {
            log::debug("async accept proxy was called, fd: {}, revent: {}", sock_.descriptor(), revent_);
            int fd = ::accept4(sock_.descriptor(), nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK);
            if (fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    // TODO: ECONNABORTED, ENOBUFS, EPERM, EPROTO, ENOSR, ESOCKTNOSUPPORT, EPROTONOSUPPORT, ETIMEOUT, ERESETARTSYS
                    log::info("accept returns negligible error, fd: {}, error: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
                    return false;
                }
                log::error("failed to accept, fd: {}, errno: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
                res_ = utils::trans_error_code(errno);
            }
            else {
                res_ = socket<proto>::wrap(fd, sock_.domain(), role::PEER);
            }
            next.resume();
            return true;
        });
        return true;
    }

    auto await_resume() noexcept -> utils::expected<socket<proto>, std::error_code> {
        log::debug("async accept awaiter resume, fd: {}", sock_.descriptor());
        return std::move(res_);
    }

private:
    socket<proto> &sock_;
    async::event revent_ {async::NONE};
    utils::expected<socket<proto>, std::error_code> res_;
};

template <protocol proto>
class async_read_awaiter {
public:
    async_read_awaiter(socket<proto> &sock, std::span<char> buffer) : sock_(sock), buffer_(buffer) {}

    auto await_ready() -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool {
        async::default_scheduler().post_coro(
            sock_.descriptor(),
            async::READ | async::ERROR | async::HANGUP | async::RDHANGUP,
            revent_,
            handle
        );
        return true;
    }

    auto await_resume() noexcept -> utils::expected<size_t, std::error_code> {
        assert(revent_);
        log::debug("async read awaiter resume, fd: {}, revent: {}", sock_.descriptor(), revent_);
        if (revent_ & async::ERROR) {
            return utils::trans_error_code(utils::detail::epoll_error);
        }
        else if (revent_ & (async::HANGUP | async::RDHANGUP)) {
            return utils::trans_error_code(utils::detail::closed_by_peer);
        }
        auto res = ::read(sock_.descriptor(), buffer_.data(), buffer_.size());
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                log::info("read returns negligible error, fd: {}, errno: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
                return 0;
            }
            log::error("failed to read, fd: {}, errno: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
            return utils::trans_error_code(errno);
        }
        return res;
    }

private:
    socket<proto> &sock_;
    std::span<char> buffer_;
    async::event revent_ {async::NONE};
};

template <protocol proto>
class async_write_awaiter {
public:
    async_write_awaiter(socket<proto> &sock, std::span<char> buffer) : sock_(sock), buffer_(buffer) {}

    auto await_ready() -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept {
        async::default_scheduler().post_coro(sock_.descriptor(),
            async::WRITE | async::ERROR | async::HANGUP,
            revent_,
            handle
        );
        return true;
    }

    auto await_resume() noexcept -> utils::expected<size_t, std::error_code> {
        log::debug("async write awaiter resume, fd: {}, revent: {}", sock_.descriptor(), revent_);
        if (revent_ & async::ERROR) {
            return utils::trans_error_code(utils::detail::epoll_error);
        }
        else if (revent_ & (async::HANGUP | async::RDHANGUP)) {
            return utils::trans_error_code(utils::detail::closed_by_peer);
        }
        auto res = ::write(sock_.descriptor(), buffer_.data(), buffer_.size());
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                log::info("write returns negligible error, fd: {}, errno: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
                return 0;
            }
            log::error("failed to write, fd: {}, errno: {}, message: {}", sock_.descriptor(), errno, ::strerror(errno));
            return utils::trans_error_code(errno);
        }
        return res;
    }

private:
    socket<proto> &sock_;
    std::span<char> buffer_;
    async::event revent_ {async::NONE};
};

} /* namespace bc::network::detail */

template <protocol proto>
auto async_accept(socket<proto> &sock) -> detail::async_accept_awaiter<proto> {
    return {sock};
}

template <protocol proto>
inline auto async_read(socket<proto> &sock, std::span<char> buffer) -> detail::async_read_awaiter<proto> {
    return {sock, buffer};
}

template <protocol proto>
inline auto async_write(socket<proto> &sock, std::span<char> const buffer) -> detail::async_write_awaiter<proto> {
    return {sock, buffer};
}

} /* namespace bc::network */

#endif /* __BC_NETWORK_SOCKET_H__ */
