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

#include <functional>
#include <memory>
#include <utility>

#include <boost/asio/strand.hpp>
#include <boost/beast/core/basic_stream.hpp>

#include <radrpc/config.hpp>
#include <radrpc/debug/log.hpp>
#include <radrpc/common/session_context.hpp>
#include <radrpc/core/timeout.hpp>
#include <radrpc/impl/server/detect_session.hpp>
#include <radrpc/impl/server/listener.hpp>

namespace radrpc {
namespace impl {
namespace server {

void listener::handle_accept()
{
    if (!m_acceptor.is_open())
        return;
    RADRPC_LOG("listener::handle_accept");
    m_acceptor.async_accept(boost::asio::make_strand(m_io_ctx),
                            boost::beast::bind_front_handler(
                                &listener::on_accept, shared_from_this()));
}

void listener::on_accept(boost::system::error_code ec, tcp::socket socket)
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
            RADRPC_LOG(
                "listener::on_accept: Rejected connection: " << ec.message());
            socket.close();
        }
        else
        {
            RADRPC_LOG("listener::on_accept: Create & run new server_session");
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
                auto remote_host = socket.remote_endpoint().address().to_string();
                boost::beast::flat_buffer buffer;
                buffer.max_size(manager->server_cfg.max_handshake_bytes);
                std::make_shared<session_accept<streams::plain>>(
                    boost::beast::tcp_stream(std::move(socket)),
                    std::move(buffer),
                    std::move(remote_host),
                    manager,
                    m_server_timeout,
                    m_session_cfg)
                    ->accept();
            }
        }
    }
    handle_accept();
}

listener::listener(boost::asio::io_context &p_io_ctx,
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

    m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
    {
        RADRPC_LOG("+listener:: Acceptor set option failed\n" << ec.message());
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

listener::~listener() { RADRPC_LOG("~listener"); }

void listener::run()
{
    if (!m_acceptor.is_open())
        return;
    handle_accept();
    RADRPC_LOG("listener::run");
}

} // namespace server
} // namespace impl
} // namespace radrpc