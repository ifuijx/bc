#pragma once

#ifndef __BC_NETWORK_ADDRESS_H__
#define __BC_NETWORK_ADDRESS_H__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <fmt/core.h>
#include <fmt/format.h>

#include <bc/utils/error.hpp>
#include <bc/utils/expected.hpp>
#include <bc/utils/overload.hpp>
#include <bc/log/log.hpp>

namespace bc::network {

enum class domain {
    IPv4 = AF_INET,
    IPv6 = AF_INET6,
};

static auto parse_ipv4(std::string_view hostname) -> std::optional<uint32_t> {
    auto is_ipv4_char = [](char c) {
        return c >= '0' && c <= '9' || c == '.';
    };

    if (!std::ranges::all_of(hostname, is_ipv4_char)) {
        return std::nullopt;
    }

    auto segments = std::views::split(hostname, std::string_view(".", 1));
    uint32_t addr {0};
    uint8_t *parts = reinterpret_cast<uint8_t *>(&addr);
    {
        std::size_t i = 0;
        for (auto const segment : segments) {
            if (i == 4) {
                return std::nullopt;
            }
            auto [ptr, ec] = std::from_chars(segment.begin(), segment.end(), parts[3 - i]);
            if (ec != std::errc()) {
                return std::nullopt;
            }
            ++i;
        }
        if (i != 4) {
            return std::nullopt;
        }
    }

    return addr;
}

static auto parse_ipv6(std::string_view hostname) -> std::optional<std::array<uint16_t, 8>> {
    auto is_ipv6_char = [](char c) {
        return c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F' || c == ':';
    };

    if (!std::ranges::all_of(hostname, is_ipv6_char)) {
        log::error("hostname format error");
        return std::nullopt;
    }

    if (hostname.starts_with("::")) {
        hostname = hostname.substr(1);
    }
    else if (hostname.ends_with("::")) {
        hostname = hostname.substr(0, hostname.size() - 1);
    }

    auto segments = std::views::split(hostname, std::string_view(":", 1));
    std::array<uint16_t, 8> addr {0};
    std::array<uint16_t, 8> tail {0};
    {
        bool has_tail = false;
        auto it = segments.begin();
        auto addr_it = addr.begin();
        while (addr_it != addr.end() && it != segments.end()) {
            if ((*it).empty()) {
                has_tail = true;
                ++it;
                break;
            }
            if ((*it).size() > 4) {
                return std::nullopt;
            }
            auto [ptr, ec] = std::from_chars((*it).begin(), (*it).end(), *addr_it, 16);
            if (ec != std::errc()) {
                return std::nullopt;
            }
            ++it;
            ++addr_it;
        }

        if (has_tail) {
            auto tail_it = tail.begin();
            while (tail_it != tail.end() && it != segments.end()) {
                if ((*it).empty()) {
                    return std::nullopt;
                }
                if ((*it).size() > 4) {
                    return std::nullopt;
                }
                auto [ptr, ec] = std::from_chars((*it).begin(), (*it).end(), *tail_it, 16);
                if (ec != std::errc()) {
                    return std::nullopt;
                }
                ++it;
                ++tail_it;
            }

            auto visible_count = addr_it - addr.begin() + (tail_it - tail.begin());
            if (visible_count >= 8) {
                return std::nullopt;
            }

            addr_it += 8 - visible_count;

            std::copy(tail.begin(), tail_it, addr_it);
        }
    }
    std::ranges::transform(addr, addr.begin(), [](uint16_t n) {
        return ::htons(n);
    });
    return addr;
}

// TODO: support fmt

class address {
    friend fmt::formatter<bc::network::address>;

public:
    static auto from(std::string_view hostname, uint16_t port) -> utils::expected<address, std::error_code> {
        static_assert(sizeof(in_port_t) == 2);
        auto ipv4 = parse_ipv4(hostname);
        if (ipv4) {
            sockaddr_in sockaddr = {0};
            sockaddr.sin_family = std::to_underlying(domain::IPv4);
            sockaddr.sin_port = ::htons(port);
            sockaddr.sin_addr.s_addr = ::htonl(*ipv4);
            return sockaddr;
        }
        auto ipv6 = parse_ipv6(hostname);
        if (ipv6) {
            sockaddr_in6 sockaddr = {0};
            sockaddr.sin6_family = std::to_underlying(domain::IPv6);
            sockaddr.sin6_port = ::htons(port);
            std::ranges::copy(*ipv6, sockaddr.sin6_addr.s6_addr16);
            return sockaddr;
        }
        return utils::trans_error_code(utils::detail::invalid_address);
    }

public:
    address(sockaddr_in const &addr) : sockaddr_(addr) {}
    address(sockaddr_in6 const &addr) : sockaddr_(addr) {}
    address(std::string_view hostname, uint16_t port) : address(*from(hostname, port)) {}

    auto sockaddr() const -> sockaddr const * {
        return std::visit(utils::overload([](auto const &sockaddr) {
            return reinterpret_cast<struct sockaddr const *>(&sockaddr);
        }), sockaddr_);
    }

    auto socklen() const -> std::size_t {
        return std::visit(utils::overload([](auto const &sockaddr) {
            return sizeof(sockaddr);
        }), sockaddr_);
    }

    auto domain() const -> network::domain {
        return std::visit(utils::overload([](sockaddr_in const &sockaddr) {
            return network::domain::IPv4;
        }, [](sockaddr_in6 const &sockaddr) {
            return network::domain::IPv6;
        }), sockaddr_);
    }

private:
    template <typename context>
    auto expr(context &ctx) const {
        return std::visit(utils::overload([&](sockaddr_in const &sockaddr) {
            auto numbers = reinterpret_cast<uint8_t const *>(&sockaddr.sin_addr.s_addr);
            return fmt::format_to(ctx.out(), "{}.{}.{}.{}:{}", numbers[0], numbers[1], numbers[2], numbers[3], ::ntohs(sockaddr.sin_port));
        }, [&](sockaddr_in6 const &sockaddr) {
            auto &ns = sockaddr.sin6_addr.s6_addr16;
            std::array<uint16_t, 8> numbers {
                ns[0], ns[1], ns[2], ns[3],
                ns[4], ns[5], ns[6], ns[7],
            };
            std::ranges::transform(numbers, numbers.begin(), [](uint16_t n) {
                return ::ntohs(n);
            });
            return fmt::format_to(ctx.out(), "[{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}]:{}", numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], ::ntohs(sockaddr.sin6_port));
        }), sockaddr_);
    }

private:
    std::variant<sockaddr_in, sockaddr_in6> sockaddr_;
};

} /* namespace bc::network */

namespace fmt {

template <>
class fmt::formatter<bc::network::address> {
public:
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
    template <typename context>
    constexpr auto format(bc::network::address const &addr, context &ctx) const {
        return addr.expr(ctx);
    }
};

} /* namespace fmt */

#endif /* __BC_NETWORK_ADDRESS_H__ */
