#pragma once

#ifndef __BC_UTILS_DETAIL_H__
#define __BC_UTILS_DETAIL_H__

#include <cstddef>

namespace bc::utils::detail {

template <typename T, typename F, typename ...Fs>
consteval auto select() -> std::size_t {
    if constexpr (requires (F f, T v) { f(v); }) {
        return 0;
    }
    else if constexpr (sizeof...(Fs) > 0) {
        return 1 + select<T, Fs...>();
    }
    else {
        return 1;
    }
}

} /* namespace bc::utils::detail */

#endif /* __BC_UTILS_DETAIL_H__*/
