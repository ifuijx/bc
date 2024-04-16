#pragma once

#ifndef __BC_UTILS_ERROR_H__
#define __BC_UTILS_ERROR_H__

#include <cstdlib>
#include <string>
#include <system_error>

namespace bc::utils {

namespace detail {

enum errc {
    operation_not_permitted = EPERM, // 1
    no_such_file_or_directory = ENOENT, // 2
    interrupted = EINTR, // 4
    io_error = EIO, // 5
    bad_file_descriptor = EBADF, // 9
    not_enough_memory = ENOMEM, // 12
    permission_denied = EACCES, // 13
    bad_address = EFAULT, // 14
    file_exists = EEXIST, // 17
    not_a_directory = ENOTDIR, // 20
    invalid_argument = EINVAL, // 22
    too_many_files_open = EMFILE, // 24
    file_too_large = EFBIG, // 27
    no_space_on_device = ENOSPC, // 28
    read_only_file_system = EROFS, // 30
    broken_pipe = EPIPE, // 32
    name_too_long = ENAMETOOLONG, // 36
    too_many_symbolic_link_levels = ELOOP, // 40
    not_a_socket = ENOTSOCK, // 88
    destination_address_required = EDESTADDRREQ, // 89
    no_protocol_option = ENOPROTOOPT, // 92
    operation_not_supported = EOPNOTSUPP, // 95
    address_in_use = EADDRINUSE, // 98
    address_not_available = EADDRNOTAVAIL, // 99
    quota_exceeded = EDQUOT, // 122
    epoll_error = 200,
    closed_by_peer = 201,
    invalid_address = 202,
};

class bc_error_category : public std::error_category {
public:
    auto name() const noexcept -> char const * override {
        return "bc";
    }

    auto message(int condition) const -> std::string override {
        switch (condition) {
            case operation_not_permitted:
                return "operation not permitted";
            case no_such_file_or_directory:
                return "no such file or directory";
            case interrupted:
                return "interrupted system call";
            case io_error:
                return "input/output error";
            case bad_file_descriptor:
                return "bad file descriptor";
            case not_enough_memory:
                return "cannot allocate memory";
            case permission_denied:
                return "permission denied";
            case bad_address:
                return "bad address";
            case file_exists:
                return "file exists";
            case not_a_directory:
                return "not a directory";
            case invalid_argument:
                return "invalid argument";
            case too_many_files_open:
                return "too many open files";
            case file_too_large:
                return "file too large";
            case no_space_on_device:
                return "no space left on device";
            case read_only_file_system:
                return "read-only file system";
            case broken_pipe:
                return "broken pipe";
            case name_too_long:
                return "name too long";
            case too_many_symbolic_link_levels:
                return "too many levels of symbolic links";
            case not_a_socket:
                return "socket operation on non-socket";
            case destination_address_required:
                return "destination address required";
            case no_protocol_option:
                return "protocol not available";
            case operation_not_supported:
                return "operation not supported";
            case address_in_use:
                return "address already in use";
            case address_not_available:
                return "cannot assign requested address";
            case quota_exceeded:
                return "quota exceeded";
            case epoll_error:
                return "epoll error";
            case closed_by_peer:
                return "closed by peer";
            case invalid_address:
                return "invalid address";
            default:
                abort();
        }
    }
};

inline auto bc_category() -> std::error_category const & {
    static bc_error_category bc_category;
    return bc_category;
}

} /* namespace bc::utils::detail */

inline auto trans_error_code(int error) -> std::error_code {
    return std::error_code(error, detail::bc_category());
}

} /* namespace bc::error */

#endif /* __BC_UTILS_ERROR_H__ */
