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

#ifndef RADRPC_COMMON_CLIENT_CONFIG_HPP
#define RADRPC_COMMON_CLIENT_CONFIG_HPP

#include <string>
#include <cstdint>
#include <ostream>

#include <radrpc/config.hpp>

namespace radrpc {
namespace common {

class client_config
{
  public:
    /// The address to connect to.
    std::string host_address;

    /// The port to open.
    uint16_t port;

    /// Maximum message size to receive.
    std::size_t max_read_bytes;

    /// The amount of attempts to send a message.
    unsigned int send_attempts;

    /// The delay after a failed attempt.
    duration send_attempt_delay;

    client_config() :
        host_address(""),
        port(0),
        max_read_bytes(0),
        send_attempts(0),
        send_attempt_delay({})
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const client_config &o)
{
    os << "host_address: " << o.host_address << std::endl
       << "port: " << o.port << std::endl
       << "max_read_bytes: " << o.max_read_bytes << std::endl
       << "send_attempts: " << o.send_attempts << std::endl
       << "send_attempt_delay: "
       << std::chrono::duration_cast<std::chrono::milliseconds>( //
              o.send_attempt_delay)
              .count()
       << std::endl;
    return os;
}

} // namespace common

using client_config = radrpc::common::client_config;

} // namespace radrpc

#endif // RADRPC_COMMON_CLIENT_CONFIG_HPP