#pragma once

#ifndef __BC_UTILS_OVERLOAD_H__
#define __BC_UTILS_OVERLOAD_H__

#include <tuple>
#include <utility>

#include "detail/detail.hpp"

namespace bc::utils {

template <typename ...Fs>
struct overload {
    overload(Fs &&...fs) : fs(std::forward<Fs>(fs)...) {}

    auto operator()(auto &&value) {
        if constexpr (constexpr size_t i = detail::select<decltype(value), Fs...>(); i < sizeof...(Fs)) {
            return get<i>(fs)(std::forward<decltype(value)>(value));
        }
        else {
            throw;
        }
    }

    std::tuple<Fs &&...> fs;
};

} /* namespace bc::utils */

#endif /* __BC_UTILS_OVERLOAD_H__ */
