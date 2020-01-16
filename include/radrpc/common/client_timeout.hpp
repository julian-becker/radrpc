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

#ifndef RADRPC_COMMON_CLIENT_TIMEOUT_HPP
#define RADRPC_COMMON_CLIENT_TIMEOUT_HPP

#include <string>
#include <cstdint>
#include <ostream>

#include <radrpc/config.hpp>

namespace radrpc {
namespace common {

class client_timeout
{
  public:
    /// The timeout for handshake used in 'connect()'.
    duration handshake_timeout;

    /// The timeout for sending a message used in 'send()' & 'send_recv()'.
    duration send_timeout;

    /// The timeout for a response used in 'send_recv()' or 'ping()'
    duration response_timeout;

    explicit client_timeout() :
        handshake_timeout(config::handshake_timeout),
        send_timeout(config::send_timeout),
        response_timeout(config::response_timeout)
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const client_timeout &o)
{
    os << "handshake_timeout: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.handshake_timeout)
              .count()
       << std::endl
       << "send_timeout: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.send_timeout)
              .count()
       << std::endl
       << "response_timeout: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.response_timeout)
              .count()
       << std::endl;
    return os;
}

} // namespace common

using client_timeout = radrpc::common::client_timeout;

} // namespace radrpc

#endif // RADRPC_COMMON_CLIENT_TIMEOUT_HPP