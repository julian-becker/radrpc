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

#ifndef RADRPC_CONFIG_HPP
#define RADRPC_CONFIG_HPP
#include <chrono>

namespace boost {
namespace asio {
namespace ssl {
} // namespace ssl
} // namespace asio
} // namespace boost
namespace boost {
namespace beast {
namespace http {
} // namespace http
namespace websocket {
} // namespace websocket
} // namespace beast
} // namespace boost

namespace radrpc {

using duration = std::chrono::steady_clock::duration;
using time_point = std::chrono::steady_clock::time_point;

namespace config {

constexpr int64_t deadlock_secs = 30;
constexpr int64_t io_timeout_secs = 5;

constexpr std::size_t handshake_max_bytes = 1024;
constexpr std::size_t queue_recv_max_bytes = 500;
constexpr std::size_t queue_send_max_bytes = 500;

constexpr duration close_timeout = std::chrono::seconds(2);
constexpr duration handshake_timeout = std::chrono::seconds(2);
constexpr duration send_timeout = std::chrono::seconds(2);
constexpr duration response_timeout = std::chrono::seconds(2);

} // namespace config
} // namespace radrpc

#endif // RADRPC_CONFIG_HPP