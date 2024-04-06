#pragma once

#ifndef __BC_UTILS_NONCOPYABLE_H__
#define __BC_UTILS_NONCOPYABLE_H__

namespace bc::utils {

class noncopyable {
protected:
    noncopyable() = default;

private:
    noncopyable(noncopyable const &) = delete;
    noncopyable & operator=(noncopyable const &) = delete;

protected:
    ~noncopyable() {}
};

} /* namespace bc::utils */

#endif /* __BC_UTILS_NONCOPYABLE_H__ */
