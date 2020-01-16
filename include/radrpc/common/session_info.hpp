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

#ifndef RADRPC_COMMON_SESSION_INFO_HPP
#define RADRPC_COMMON_SESSION_INFO_HPP

#include <boost/beast/websocket/stream_base.hpp>

#include <radrpc/common/session_config.hpp>

namespace radrpc {
namespace common {

namespace websocket = boost::beast::websocket;
typedef websocket::request_type handshake_request;
typedef websocket::response_type handshake_response;

/**
 * @brief Type containing session data.
 *
 */
struct session_info
{
    /// The unqiue id of the session.
    const uint64_t id;

    /// The client's ip address.
    const std::string &remote_host;

    /// The client's handshake.
    const handshake_request &request_handshake;

    /// The sent handshake to the client.
    handshake_response &response_handshake;

    /// The used configuration for this session.
    session_config &config;
};

} // namespace common

using session_info = radrpc::common::session_info;

} // namespace radrpc

#endif // RADRPC_COMMON_SESSION_INFO_HPP