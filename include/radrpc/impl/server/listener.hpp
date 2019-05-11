/*
 * MIT License

 * Copyright (c) 2019 reapler

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

#include <functional>
#include <memory>
#include <utility>

#include <boost/asio/strand.hpp>

#include <radrpc/config.hpp>
#include <radrpc/debug.hpp>
#include <radrpc/types.hpp>
#include <radrpc/detail/operations.hpp>
#include <radrpc/impl/server/server_session.hpp>

namespace radrpc {
namespace impl {
namespace server {

/**
 * Accepts incoming requests & create sessions.
 */
class listener : public std::enable_shared_from_this<listener>
{
#ifdef RADRPC_SSL_SUPPORT
    ssl::context *m_ssl_ctx;
#endif
    boost::asio::io_context &m_io_ctx; ///< The referenced io context.
    tcp::acceptor m_acceptor; ///< Accept for listening to new connections.
    const server_config &m_server_cfg;      ///< The referenced server config.
    const server_timeout &m_server_timeout; ///< The referenced server timeout.
    const session_config &m_session_cfg;    ///< The referenced session config.

    /**
     * Accept new connections.
     * This function must be called within the executor context.
     */
    void handle_accept()
    {
        if (!m_acceptor.is_open())
            return;
        RADRPC_LOG("listener::handle_accept");
        m_acceptor.async_accept(boost::asio::make_strand(m_io_ctx),
                                boost::beast::bind_front_handler(
                                    &listener::on_accept, shared_from_this()));
    }

    /**
     *
     * @param ec
     * @param socket
     */
    void on_accept(boost::system::error_code ec, tcp::socket socket)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;
        if (ec)
        {
            RADRPC_LOG("listener::on_accept: " << ec.message());
        }
        else
        {
            if ((manager->on_listen &&
                 !manager->on_listen(
                     socket.remote_endpoint(ec).address().to_string())) ||
                ec || manager->is_full())
            {
                RADRPC_LOG("listener::on_accept: Rejected connection: "
                           << ec.message());
                socket.close();
            }
            else
            {
                RADRPC_LOG(
                    "listener::on_accept: Create & run new server_session");
#ifdef RADRPC_SSL_SUPPORT
                if (m_ssl_ctx)
                {
                    // Detect session and create plain or ssl session
                    std::make_shared<detect_session>(std::move(socket),
                                                     *m_ssl_ctx,
                                                     m_server_cfg,
                                                     m_server_timeout,
                                                     m_session_cfg,
                                                     manager)
                        ->run();
                }
                else
#endif
                {
                    // Create plain session only
                    boost::beast::flat_buffer buffer;
                    buffer.max_size(manager->server_cfg.max_handshake_bytes);
                    std::make_shared<
                        session_accept<server_streams::plain_stream>>(
                        boost::beast::tcp_stream(std::move(socket)),
                        std::move(buffer),
                        manager,
                        m_server_timeout,
                        m_session_cfg)
                        ->accept();
                }
            }
        }
        handle_accept();
    }

  public:
    std::shared_ptr<session_manager>
        manager; ///< The manager to share among sessions.

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
             const session_config &p_session_cfg) :
#ifdef RADRPC_SSL_SUPPORT
        m_ssl_ctx(p_ssl_ctx),
#endif
        m_io_ctx(p_io_ctx),
        m_acceptor(boost::asio::make_strand(p_io_ctx)),
        m_server_cfg(p_server_cfg),
        m_server_timeout(p_server_timeout),
        m_session_cfg(p_session_cfg),
        manager(p_manager->shared_from_this())
    {
#ifdef RADRPC_SSL_SUPPORT
        if (m_ssl_ctx != nullptr)
            RADRPC_LOG("+listener: SSL enabled");
        else
#endif
            RADRPC_LOG("+listener: SSL disabled");
        boost::system::error_code ec;

        // Set current amount of references so they can be
        // subtracted to get the amount of connections
        manager->set_subtract_refs();

        m_acceptor.open(p_endpoint.protocol(), ec);
        if (ec)
        {
            RADRPC_LOG("+listener:: Acceptor open failed\n" << ec.message());
            return;
        }

        m_acceptor.set_option(boost::asio::socket_base::reuse_address(true),
                              ec);
        if (ec)
        {
            RADRPC_LOG("+listener:: Acceptor set option failed\n"
                       << ec.message());
            return;
        }

        m_acceptor.bind(p_endpoint, ec);
        if (ec)
        {
            RADRPC_LOG("+listener:: Acceptor bind failed\n" << ec.message());
            return;
        }

        m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            RADRPC_LOG("+listener:: Acceptor listen failed\n" << ec.message());
            return;
        }
    }

    ~listener() { RADRPC_LOG("~listener"); }

    /**
     * Start the listener & accept new connections.
     */
    void run()
    {
        if (!m_acceptor.is_open())
            return;
        handle_accept();
        RADRPC_LOG("listener::run");
    }
};

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_LISTENER_HPP