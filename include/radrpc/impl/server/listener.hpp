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

#ifndef RADRPC_IMPL_SERVER_LISTENER_HPP
#define RADRPC_IMPL_SERVER_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>

#include <radrpc/common/server_config.hpp>
#include <radrpc/common/server_timeout.hpp>
#include <radrpc/impl/server/server_session.hpp>

namespace boost {
namespace asio {
namespace ssl {

class context;

} // namespace ssl
} // namespace asio
} // namespace boost

namespace radrpc {
namespace impl {
namespace server {

/**
 * Accepts incoming requests & create sessions.
 */
class listener : public std::enable_shared_from_this<listener>
{
#ifdef RADRPC_SSL_SUPPORT
    /// The referenced ssl context.
    ssl::context *m_ssl_ctx;
#endif
    /// The referenced io context.
    boost::asio::io_context &m_io_ctx; 

    /// Accept for listening to new connections.
    tcp::acceptor m_acceptor; 

    /// The referenced server config.
    const server_config &m_server_cfg;      

    /// The referenced server timeout.
    const server_timeout &m_server_timeout; 

    /// The referenced session config.
    const session_config &m_session_cfg;    

    /**
     * Accept new connections.
     * This function must be called within the executor context.
     */
    void handle_accept();

    /**
     *
     * @param ec
     * @param socket
     */
    void on_accept(boost::system::error_code ec, tcp::socket socket);

  public:
    std::shared_ptr<session_manager>
        manager; /// The manager to share among sessions.

    /**
     * @param p_io_ctx The io context to use.
     * @param p_ssl_ctx The ssl context to use.
     * @param p_endpoint The endpoint to open the listener.
     * @param p_manager The manager to track the sessions.
     * @param p_server_cfg The server config.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The config for each session.
     */
    listener(boost::asio::io_context &p_io_ctx,
#ifdef RADRPC_SSL_SUPPORT
             ssl::context *p_ssl_ctx,
#endif
             tcp::endpoint p_endpoint,
             const std::shared_ptr<session_manager> &p_manager,
             const server_config &p_server_cfg,
             const server_timeout &p_server_timeout,
             const session_config &p_session_cfg);

    ~listener();

    /**
     * Start the listener & accept new connections.
     */
    void run();
};

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_LISTENER_HPP