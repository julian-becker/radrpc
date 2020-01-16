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

#ifndef RADRPC_IMPL_SERVER_DETECT_SESSION_HPP
#define RADRPC_IMPL_SERVER_DETECT_SESSION_HPP
#ifdef RADRPC_SSL_SUPPORT

#include <memory>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include <radrpc/common/streams.hpp>
#include <radrpc/config.hpp>

namespace boost {
namespace asio {
namespace ssl {

class context;

} // namespace ssl
} // namespace asio
} // namespace boost

namespace radrpc {
namespace common {

class server_config;
class server_timeout;
class session_config;

} // namespace common
} // namespace radrpc

namespace radrpc {
namespace impl {
namespace server {

using namespace radrpc::common;

class session_manager;

/**
 * A type to determine whether the
 * client is using plain or ssl stream
 */
class detect_session : public std::enable_shared_from_this<detect_session>
{
    /// The stream to operate on.
    boost::beast::tcp_stream m_stream;

    /// The used ssl context.
    ssl::context &m_ssl_ctx;

    /// The server config.
    const server_config &m_server_cfg;

    /// The server timeout.
    const server_timeout &m_server_timeout;

    /// The session config.
    const session_config &m_session_cfg;

    /// The manager shared among sessions.
    std::shared_ptr<session_manager> &m_manager;

    /// The buffer to read the request.
    boost::beast::flat_buffer m_buffer;

  public:
    /**
     * @param m_socket The socket to own.
     * @param p_ssl_ctx The ssl context to use.
     * @param p_server_cfg The server config.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The session config.
     * @param p_manager The manager shared among sessions.
     */
    explicit detect_session(tcp::socket &&m_socket,
                            ssl::context &p_ssl_ctx,
                            const server_config &p_server_cfg,
                            const server_timeout &p_server_timeout,
                            const session_config &p_session_cfg,
                            std::shared_ptr<session_manager> &p_manager);

    /**
     * Start read the client's request
     * & check whether it is plain or ssl.
     */
    void run();

    /**
     * @param ec
     * @param result
     */
    void on_detect(boost::beast::error_code ec, bool result);
};

} // namespace server
} // namespace impl
} // namespace radrpc

#endif
#endif // RADRPC_IMPL_SERVER_DETECT_SSL_HPP