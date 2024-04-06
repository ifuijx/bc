#pragma once

#ifndef __BC_NETWORK_ADDRESS_H__
#define __BC_NETWORK_ADDRESS_H__

#include <arpa/inet.h>
#include <sys/socket.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>
#include <system_error>
#include <utility>

#include <bc/log/log.hpp>

namespace bc::network {

enum class domain : decltype(AF_INET) {
    IPv4 = AF_INET,
};

template <domain Domain>
class address {
public:
    auto sockaddr() const -> sockaddr const * {
        throw "not implemented";
    }
    auto socklen() const -> std::size_t {
        throw "not implemented";
    }
    auto expr() const -> std::string_view {
        throw "not implemented";
    }
};

// TODO: support fmt

template <>
class address<domain::IPv4> {
    static auto parse_ipv4(std::string_view hostname) -> std::optional<uint32_t> {
        auto is_ipv4_char = [](char c) {
            return c >= '0' && c <= '9' || c == '.';
        };

        if (!std::ranges::all_of(hostname, is_ipv4_char)) {
            log::error("hostname format error");
            return std::nullopt;
        }

        auto segments = std::views::split(hostname, std::string_view(".", 1));
        uint32_t addr {0};
        uint8_t *parts = reinterpret_cast<uint8_t *>(&addr);
        {
            std::size_t i = 0;
            for (auto const segment : segments) {
                if (i == 4) {
                    log::error("hostname format error");
                    return std::nullopt;
                }
                auto [ptr, ec] = std::from_chars(segment.begin(), segment.end(), parts[3 - i]);
                if (ec != std::errc()) {
                    log::error("hostname format error");
                    return std::nullopt;
                }
                ++i;
            }
            if (i != 4) {
                log::error("hostname format error");
                return std::nullopt;
            }
        }

        return addr;
    }

public:
    address(std::string_view hostname, uint16_t port) {
        auto ip = parse_ipv4(hostname);
        if (!ip) {
            throw;
        }
        addr_.sin_family = std::to_underlying(domain::IPv4);
        static_assert(sizeof(in_port_t) == 2);
        addr_.sin_port = ::htons(port);
        addr_.sin_addr.s_addr = ::htonl(*ip);
    }

    auto sockaddr() const -> sockaddr const * { return reinterpret_cast<struct sockaddr const *>(&addr_); }
    auto socklen() const -> std::size_t { return sizeof(addr_); }
    auto expr() const -> std::string_view {
        throw "not implemented";
    }

private:
    sockaddr_in addr_ {0};
};

} /* namespace bc::network */

#endif /* __BC_NETWORK_ADDRESS_H__ */
