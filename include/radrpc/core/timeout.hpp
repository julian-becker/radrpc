/*
 * MIT License

 * Copyright (c) 2020 reapler

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
   all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#ifndef RADRPC_CORE_TIMEOUT_HPP
#define RADRPC_CORE_TIMEOUT_HPP

#include <radrpc/config.hpp>

namespace radrpc {
namespace core {

/**
 * Set or get the timeout for sending data to the server for the calling thread.
 * @param timeout The timeout to set
 * @see radrpc::impl::client::session
 * @return The timeout if parameter 'timeout' was not set, otherwise zero.
 */
inline std::chrono::steady_clock::duration send_timeout(
    std::chrono::steady_clock::duration timeout =
        std::chrono::steady_clock::duration::max())
{
    thread_local duration tls_send_timeout = duration::zero();
    if (timeout == duration::max())
        return tls_send_timeout;
    tls_send_timeout = timeout;
    return duration::zero();
}

/**
 * Set or get the timeout for receiving data from the server for the calling
 * thread.
 * @param timeout The timeout to set
 * @see radrpc::impl::client::session
 * @return The timeout if parameter 'timeout' was not set, otherwise zero.
 */
inline std::chrono::steady_clock::duration response_timeout(
    std::chrono::steady_clock::duration timeout =
        std::chrono::steady_clock::duration::max())
{
    thread_local duration tls_response_timeout = duration::zero();
    if (timeout == duration::max())
        return tls_response_timeout;
    tls_response_timeout = timeout;
    return duration::zero();
}

} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_TIMEOUT_HPP