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

#ifndef RADRPC_COMMON_SERVER_CONFIG_HPP
#define RADRPC_COMMON_SERVER_CONFIG_HPP

#include <string>
#include <cstdint>
#include <ostream>

#include <radrpc/common/stream_mode.hpp>

namespace radrpc {
namespace common {

/**
 * @brief
 *
 */
class server_config
{
  public:
    /// The address to host on, usually "0.0.0.0"
    std::string host_address;

    /// The port to open.
    uint16_t port;

    /// The amount of workers to run the io context.
    unsigned int workers;

    /// The maximum amount of sessions.
    unsigned int max_sessions;

    /// The maximum handshake size for clients.
    std::size_t max_handshake_bytes;

    /// The mode represented as flag to run the server.
    stream_mode mode;

    server_config() :
        host_address(""),
        port(0),
        workers(0),
        max_sessions(0),
        max_handshake_bytes(0),
        mode(stream_mode::plain)
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const server_config &o)
{
    os << "host_address: " << o.host_address << std::endl
       << "port: " << o.port << std::endl
       << "workers: " << o.workers << std::endl
       << "max_sessions: " << o.max_sessions << std::endl
       << "max_handshake_bytes: " << o.max_handshake_bytes << std::endl
       << "mode: " << (int)o.mode << std::endl;
    return os;
}

} // namespace common

using server_config = radrpc::common::server_config;

} // namespace radrpc

#endif // RADRPC_CORE_SERVER_CONFIG_HPP