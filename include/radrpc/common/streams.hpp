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

#ifndef RADRPC_COMMON_STREAMS_HPP
#define RADRPC_COMMON_STREAMS_HPP

#ifdef RADRPC_SSL_SUPPORT
#include <boost/asio/ssl/context.hpp>
#endif
#include <boost/beast/http/write.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <boost/beast/websocket/stream_fwd.hpp>

#include <radrpc/beast_fwd.hpp>

namespace radrpc {

namespace websocket = boost::beast::websocket;
#ifdef RADRPC_SSL_SUPPORT
namespace ssl = boost::asio::ssl;
#endif
typedef websocket::request_type handshake_request;
typedef websocket::response_type handshake_response;
using tcp = boost::asio::ip::tcp;

namespace common {

struct streams
{
    typedef websocket::stream<boost::beast::tcp_stream> plain;
#ifdef RADRPC_SSL_SUPPORT
    typedef websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>>
        ssl;
#endif
};

} // namespace common

using streams = radrpc::common::streams;

} // namespace radrpc

#endif // RADRPC_COMMON_STREAMS_HPP