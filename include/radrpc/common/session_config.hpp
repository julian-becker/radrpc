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

#ifndef RADRPC_COMMON_SESSION_CONFIG_HPP
#define RADRPC_COMMON_SESSION_CONFIG_HPP

#include <ostream>

#include <radrpc/config.hpp>

namespace radrpc {
namespace common {

/**
 * @brief Type for applying configuration on sessions.
 *
 */
class session_config
{
  public:
    /// Maximum message size of each received message.
    std::size_t max_transfer_bytes;

    /// The delay to ping the client to check activity.
    duration ping_delay;

    session_config() : max_transfer_bytes(0), ping_delay(duration::zero()) {}
};

inline std::ostream &operator<<(std::ostream &os, const session_config &o)
{
    os << "max_transfer_bytes: " << o.max_transfer_bytes << std::endl
       << "ping_delay: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.ping_delay)
              .count()
       << std::endl;
    return os;
}

} // namespace common

using session_config = radrpc::common::session_config;

} // namespace radrpc

#endif // RADRPC_COMMON_SESSION_CONFIG_HPP