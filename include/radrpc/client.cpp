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

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include <radrpc/client.hpp>
#include <radrpc/debug/log.hpp>
#include <radrpc/impl/client/connector.hpp>

namespace radrpc {

void client::base::on_plain_session_delete(connector_plain *p)
{
    delete p;
    expire_session();
    RADRPC_LOG("client::on_plain_session_delete");
}

#ifdef RADRPC_SSL_SUPPORT

void client::base::on_ssl_session_delete(connector_ssl *p)
{
    delete p;
    expire_session();
    RADRPC_LOG("client::on_ssl_session_delete");
}
#endif

void client::base::expire_session()
{
    {
        std::lock_guard<std::mutex> lock(m_expired_mtx);
        m_expired = true;
    }
    m_expired_cv.notify_all();
}

void client::base::init()
{
    m_client_cfg.send_attempts = std::max(m_client_cfg.send_attempts, 1u),
    m_timeout.handshake_timeout = m_client_timeout.handshake_timeout;
    m_timeout.idle_timeout = duration::max();
    m_timeout.keep_alive_pings = false;
}

void client::base::destruct()
{
    RADRPC_LOG("client::destruct");
    m_session_mtx.lock();
    m_exit = true;

#ifdef RADRPC_SSL_SUPPORT
    if (m_is_ssl)
    {
        if (auto session = m_ssl_session.lock())
            session->close();
    }
    else
#endif
    {
        if (auto session = m_plain_session.lock())
            session->close();
    }

    if (!wait_destruction())
    {
        RADRPC_LOG("client::destruct: Timeout, this should not happen\n[IO"
                   << m_io_ctx->stopped() << "]");
    }

    if (m_io_worker.joinable())
        m_io_worker.join();
    m_session_mtx.unlock();
}

bool client::base::wait_destruction()
{
    std::unique_lock<std::mutex> lock(m_expired_mtx);
    bool success =
        m_expired_cv.wait_for(lock,
                              std::chrono::seconds(config::deadlock_secs),
                              [&] { return m_expired; });
    return success;
}

bool client::base::check_construct()
{
    RADRPC_LOG("client::check_construct");
    {
        std::lock_guard<std::mutex> lock(m_expired_mtx);
        if (!m_expired)
            return false;
        m_expired = false;
    }
    if (m_io_worker.joinable())
        m_io_worker.join();
    return true;
}

bool client::base::construct_session()
{
#ifdef RADRPC_SSL_SUPPORT
    if (m_is_ssl)
        return construct_ssl_session();
#endif
    return construct_plain_session();
}

bool client::base::construct_plain_session()
{
    RADRPC_LOG("client::construct_plain_session");
    if (!check_construct())
        return true;

    std::shared_ptr<connector_plain> ptr(
        new connector_plain(*m_io_ctx.get(),
                               m_io_worker,
                               m_client_cfg,
                               m_client_timeout,
                               m_timeout,
                               m_req_handshake,
                               m_bound_funcs),
        std::bind(&base::on_plain_session_delete, this, std::placeholders::_1));
    m_plain_session = ptr;
    return ptr->run() == connection_state::established;
}

#ifdef RADRPC_SSL_SUPPORT

bool client::base::construct_ssl_session()
{
    RADRPC_LOG("client::construct_ssl_session");
    if (!check_construct())
        return true;
    std::shared_ptr<connector_ssl> ptr(
        new connector_ssl(*m_io_ctx.get(),
                             m_ssl_ctx,
                             m_io_worker,
                             m_client_cfg,
                             m_client_timeout,
                             m_timeout,
                             m_req_handshake,
                             m_bound_funcs),
        std::bind(&base::on_ssl_session_delete, this, std::placeholders::_1));
    m_ssl_session = ptr;
    return ptr->run() == connection_state::established;
}

#endif

client::base::base(client_config p_client_cfg,
                   const client_timeout &p_client_timeout) :
#ifdef RADRPC_SSL_SUPPORT
    m_is_ssl(false),
    m_ssl_ctx(boost::asio::ssl::context::sslv23),
#endif
    m_expired(true),
    m_io_ctx(new boost::asio::io_context()),
    m_client_cfg(std::move(p_client_cfg)),
    m_client_timeout(p_client_timeout),
    m_timeout({}),
    m_exit(false)
{
    init();
}

#ifdef RADRPC_SSL_SUPPORT

client::base::base(client_config p_client_cfg,
                   const client_timeout &p_client_timeout,
                   boost::asio::ssl::context &&p_ssl_ctx) :
    m_is_ssl(true),
    m_ssl_ctx(std::move(p_ssl_ctx)),
    m_expired(true),
    m_io_ctx(new boost::asio::io_context()),
    m_client_cfg(std::move(p_client_cfg)),
    m_client_timeout(p_client_timeout),
    m_timeout({}),
    m_exit(false)
{
    init();
    RADRPC_LOG("+client::base");
}

#endif

client::base::~base() { RADRPC_LOG("~client::base"); }

bool client::base::listen_broadcast(
    uint32_t broadcast_id,
    std::function<void(receive_buffer &)> handler)
{
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return false;
    std::unique_lock<std::mutex> lock(m_expired_mtx);
    if (!m_expired)
        RADRPC_THROW("client::listen_broadcast: Cannot assign listen "
                     "handler while "
                     "session is connected, disconnect at first.");
    auto func_itr = m_bound_funcs.find(broadcast_id);
    if (func_itr != m_bound_funcs.end())
        RADRPC_THROW("client::listen_broadcast: The given id '" +
                     std::to_string(broadcast_id) +
                     "' was already bound to a function.");
    m_bound_funcs[broadcast_id] = std::move(handler);
    return true;
}

bool client::base::connect()
{
    RADRPC_LOG("client::connect");
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return false;
    bool result = construct_session();
    return result;
}

bool client::base::connect(unsigned int attempts, duration delay)
{
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return false;
    for (unsigned int i = 0; i < attempts; ++i)
    {
        if (i != 0)
        {
            // If session construction failed, it should self-destruct.
            // However if this throws, it means something hangs in
            // 'connection_state run(std::thread &worker)'
            // and 'm_run_callback' is not triggering 'set_value()'
            if (!wait_destruction())
                RADRPC_THROW(
                    "client::connect: Timeout, this should not happen");
            std::this_thread::sleep_for(delay);
        }
        RADRPC_LOG("client::connect: Attempt [" << i + 1 << "/" << attempts
                                                << "]");
        if (construct_session())
            return true;
    }
    return false;
}

void client::base::disconnect()
{
    RADRPC_LOG("client::disconnect");
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return;

#ifdef RADRPC_SSL_SUPPORT
    if (m_is_ssl)
    {
        if (auto session = m_ssl_session.lock())
            session->close();
    }
    else
#endif
    {
        if (auto session = m_plain_session.lock())
            session->close();
    }

    if (!wait_destruction())
        RADRPC_THROW("client::disconnect: Timeout, this should not happen");
}

bool client::base::ping()
{
    RADRPC_LOG("client::ping");
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return false;
    if (!construct_session())
        return false;

#ifdef RADRPC_SSL_SUPPORT
    if (m_is_ssl)
    {
        if (auto session = m_ssl_session.lock())
            return session->ping();
    }
    else
#endif
    {
        if (auto session = m_plain_session.lock())
            return session->ping();
    }

    return false;
}

bool client::base::send(uint32_t call_id,
                        const char *data_ptr,
                        std::size_t data_size)
{
    for (unsigned int i = 0; i < m_client_cfg.send_attempts; ++i)
    {
        if (i != 0)
            std::this_thread::sleep_for(m_client_cfg.send_attempt_delay);
        bool expired = true;
        boost::system::error_code ec;
        {
            std::shared_lock<std::shared_timed_mutex> read_lock(m_session_mtx);
            if (m_exit)
                return false;
                // Send
#ifdef RADRPC_SSL_SUPPORT
            if (m_is_ssl)
            {
                if (auto session = m_ssl_session.lock())
                {
                    session->send(call_id, data_ptr, data_size, ec);
                    expired = false;
                }
            }
            else
#endif
            {
                if (auto session = m_plain_session.lock())
                {
                    session->send(call_id, data_ptr, data_size, ec);
                    expired = false;
                }
            }
        }
        // Disconnect on error
        if (ec)
            disconnect();
        // Connect if expired
        if (expired)
            connect();
        // If operation was successful, return
        if (!ec && !expired)
            return true;
    }
    return false;
}

bool client::base::send(uint32_t call_id, const std::vector<char> &send_bytes)
{
    return send(call_id, send_bytes.data(), send_bytes.size());
}

receive_buffer client::base::send_recv(uint32_t call_id,
                                       const char *data_ptr,
                                       std::size_t data_size)
{
    receive_buffer result;
    for (unsigned int i = 0; i < m_client_cfg.send_attempts; ++i)
    {
        if (i != 0)
            std::this_thread::sleep_for(m_client_cfg.send_attempt_delay);
        bool expired = true;
        boost::system::error_code ec;
        {
            std::shared_lock<std::shared_timed_mutex> read_lock(m_session_mtx);
            if (m_exit)
                return result;
                // Send
#ifdef RADRPC_SSL_SUPPORT
            if (m_is_ssl)
            {
                if (auto session = m_ssl_session.lock())
                {
                    session->send_recv(
                        call_id, data_ptr, data_size, result, ec);
                    expired = false;
                }
            }
            else
#endif
            {
                if (auto session = m_plain_session.lock())
                {
                    session->send_recv(
                        call_id, data_ptr, data_size, result, ec);
                    expired = false;
                }
            }
        }
        // Disconnect on error
        if (ec)
            disconnect();
        // Connect if expired
        if (expired)
            connect();
        // If operation was successful, break
        if (!ec && !expired)
            break;
    }
    return result;
}

receive_buffer client::base::send_recv(uint32_t call_id,
                                       const std::vector<char> &send_bytes)
{
    return send_recv(call_id, send_bytes.data(), send_bytes.size());
}

void client::base::set_handshake_request(handshake_request &req_handshake)
{
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return;
    m_req_handshake = req_handshake;
}

handshake_response client::base::get_handshake_response()
{
    std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
    if (m_exit)
        return handshake_response{};
#ifdef RADRPC_SSL_SUPPORT
    if (m_is_ssl)
    {
        if (auto session = m_ssl_session.lock())
            return session->res_handshake;
    }
    else
#endif
    {
        if (auto session = m_plain_session.lock())
            return session->res_handshake;
    }
    return handshake_response{};
}

void client::base::set_send_timeout(duration timeout)
{
    core::send_timeout(timeout);
}

void client::base::set_response_timeout(duration timeout)
{
    core::response_timeout(timeout);
}

client::plain::plain(client_config p_client_cfg,
                     const client_timeout &p_client_timeout) :
    base(p_client_cfg, p_client_timeout)
{
    RADRPC_LOG("+client::plain");
}

client::plain::~plain()
{
    RADRPC_LOG("~client::plain");
    destruct();
    RADRPC_LOG("~client::plain: Done");
}

#ifdef RADRPC_SSL_SUPPORT

client::ssl::ssl(client_config p_client_cfg,
                 const client_timeout &p_client_timeout,
                 boost::asio::ssl::context &&p_ssl_ctx) :
    base(p_client_cfg, p_client_timeout, std::move(p_ssl_ctx))
{
    RADRPC_LOG("+client::ssl");
}

client::ssl::~ssl()
{
    RADRPC_LOG("~client::ssl");
    destruct();
    RADRPC_LOG("~client::ssl: Done");
}

#endif

} // namespace radrpc