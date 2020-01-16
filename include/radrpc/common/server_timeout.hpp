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

#ifndef RADRPC_COMMON_SERVER_TIMEOUT_HPP
#define RADRPC_COMMON_SERVER_TIMEOUT_HPP

#include <string>
#include <cstdint>
#include <ostream>

#include <radrpc/config.hpp>

namespace radrpc {
namespace common {

/**
 * @brief
 *
 */
class server_timeout
{
  public:
    /// The timeout used for handshake or close.
    duration handshake_or_close_timeout;

    explicit server_timeout() :
        handshake_or_close_timeout(config::handshake_timeout)
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const server_timeout &o)
{
    os << "handshake_or_close_timeout: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.handshake_or_close_timeout)
              .count()
       << std::endl;
    return os;
}

} // namespace common

using server_timeout = radrpc::common::server_timeout;

} // namespace radrpc

#endif // RADRPC_COMMON_SERVER_TIMEOUT_HPP