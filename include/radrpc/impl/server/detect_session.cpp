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

#ifdef RADRPC_SSL_SUPPORT
#include <boost/beast/core/detect_ssl.hpp>

#include <radrpc/common/server_config.hpp>
#include <radrpc/common/server_timeout.hpp>
#include <radrpc/debug/log.hpp>
#include <radrpc/impl/server/detect_session.hpp>
#include <radrpc/impl/server/server_session.hpp>
#include <radrpc/impl/server/session_accept.hpp>
#include <radrpc/impl/server/session_manager.hpp>

namespace radrpc {
namespace impl {
namespace server {

detect_session::detect_session(tcp::socket &&m_socket,
                               ssl::context &p_ssl_ctx,
                               const server_config &p_server_cfg,
                               const server_timeout &p_server_timeout,
                               const session_config &p_session_cfg,
                               std::shared_ptr<session_manager> &p_manager) :
    m_stream(std::move(m_socket)),
    m_ssl_ctx(p_ssl_ctx),
    m_server_cfg(p_server_cfg),
    m_server_timeout(p_server_timeout),
    m_session_cfg(p_session_cfg),
    m_manager(p_manager)
{
}

void detect_session::run()
{
    m_buffer.max_size(m_manager->server_cfg.max_handshake_bytes);
    m_stream.expires_after(m_server_timeout.handshake_or_close_timeout);
    boost::beast::async_detect_ssl(
        m_stream,
        m_buffer,
        boost::beast::bind_front_handler(&detect_session::on_detect,
                                         this->shared_from_this()));
}

void detect_session::on_detect(boost::beast::error_code ec,
                               bool result)
{
    if (ec)
    {
        RADRPC_LOG("detect_session::on_detect: " << ec.message());
        return;
    }
    auto remote_host =
        m_stream.socket().remote_endpoint().address().to_string();
    if (result && m_server_cfg.mode & stream_mode::ssl)
    {
        std::make_shared<session_accept<streams::ssl>>(std::move(m_stream),
                                                       std::move(m_buffer),
                                                       m_ssl_ctx,
                                                       std::move(remote_host),
                                                       m_manager,
                                                       m_server_timeout,
                                                       m_session_cfg)
            ->accept();
    }
    else if (!result && m_server_cfg.mode & stream_mode::plain)
    {
        std::make_shared<session_accept<streams::plain>>(std::move(m_stream),
                                                         std::move(m_buffer),
                                                         std::move(remote_host),
                                                         m_manager,
                                                         m_server_timeout,
                                                         m_session_cfg)
            ->accept();
    }
}

} // namespace server
} // namespace impl
} // namespace radrpc

#endif