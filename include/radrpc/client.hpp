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

#ifndef RADRPC_CLIENT_HPP
#define RADRPC_CLIENT_HPP

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include <radrpc/debug.hpp>
#include <radrpc/types.hpp>
#include <radrpc/detail/operations.hpp>
#include <radrpc/impl/client/client_session.hpp>

namespace radrpc {

/**
 * The client modes to choose from.
 */
struct client_mode
{
    typedef impl::client::session_connect<client_streams::plain_stream> plain;
#ifdef RADRPC_SSL_SUPPORT
    typedef impl::client::session_connect<client_streams::ssl_stream> ssl;
#endif
};

/**
 * A type to communicate with the server
 * @tparam client_session_  The mode to use.
 */
template <class client_session_> class client
{
#ifdef RADRPC_SSL_SUPPORT
    ssl::context m_ssl_ctx; ///< The ssl context.
#endif
    std::condition_variable
        m_expired_cv;         ///< Used for waiting on session destruction
    std::mutex m_expired_mtx; ///< Lock to use for condition variable.
    bool m_expired;           ///< Used to check whether the session is expired.
    std::unordered_map<uint32_t,
                       std::function<void(receive_buffer &)>>
        m_bound_funcs;                     ///< The bound broadcast handlers.
    std::shared_timed_mutex m_session_mtx; ///< Lock to operate on session.
    boost::asio::io_context m_io_ctx;      ///< The io context.
    std::thread m_io_worker;         ///< The worker for running the io context.
    client_config m_client_cfg;      ///< The client config.
    client_timeout m_client_timeout; ///< The client timeout.
    websocket::stream_base::timeout
        m_timeout; ///< The timeout to use in stream.
    std::weak_ptr<client_session_>
        m_session; ///< The weak reference of the session.
    handshake_request
        m_req_handshake; ///< The request handshake to send to the server.
    handshake_response
        m_res_handshake; ///< The response handshake from the server.
    bool m_exit;         ///< Used to check if client is going to exit.

    /**
     * Waits for the session destruction.
     * @return True if destructed, false if it might be in deadlock.
     */
    bool wait_destruction()
    {
        std::unique_lock<std::mutex> lock(m_expired_mtx);
        bool success =
            m_expired_cv.wait_for(lock,
                                  std::chrono::seconds(config::deadlock_secs),
                                  [&] { return m_expired; });
        return success;
    }

    /**
     * Checks whether this class is using a ssl stream.
     * @return True if ssl stream is used, false if not.
     */
    template <typename F = client_session_>
    typename std::enable_if<std::is_same<F, client_mode::plain>::value,
                            bool>::type
    is_ssl()
    {
        return false;
    }

    /**
     * Constructs a session if expired.
     * @return 	True if session is not expired or connected successfully,
     * 			false if constructed session failed to run.
     */
    template <typename F = client_session_>
    typename std::enable_if<std::is_same<F, client_mode::plain>::value,
                            bool>::type
    construct_session()
    {
        RADRPC_LOG("client::construct_session");
        {
            std::unique_lock<std::mutex> lock(m_expired_mtx);
            if (!m_expired)
                return true;
            m_expired = false;
        }
        if (m_io_worker.joinable())
            m_io_worker.join();
        std::shared_ptr<client_session_> ptr(
            new client_session_(m_io_ctx,
                                m_io_worker,
                                m_client_cfg,
                                m_client_timeout,
                                m_timeout,
                                m_req_handshake,
                                m_bound_funcs),
            std::bind(&client::on_session_delete, this, std::placeholders::_1));
        m_session = ptr;
        return ptr->run() == connection_state::established;
    }

#ifdef RADRPC_SSL_SUPPORT

    /**
     * Checks whether this class is using a ssl stream.
     * @return True if ssl stream is used, false if not.
     */
    template <typename F = client_session_>
    typename std::enable_if<std::is_same<F, client_mode::ssl>::value,
                            bool>::type
    is_ssl()
    {
        return true;
    }

    /**
     * Constructs a session if expired.
     * @return 	True if session is not expired or connected successfully,
     * 			false if constructed session failed to run.
     */
    template <typename F = client_session_>
    typename std::enable_if<std::is_same<F, client_mode::ssl>::value,
                            bool>::type
    construct_session()
    {
        RADRPC_LOG("client::construct_session");
        {
            std::unique_lock<std::mutex> lock(m_expired_mtx);
            if (!m_expired)
                return true;
            m_expired = false;
        }
        if (m_io_worker.joinable())
            m_io_worker.join();
        std::shared_ptr<client_session_> ptr(
            new client_session_(m_io_ctx,
                                m_ssl_ctx,
                                m_io_worker,
                                m_client_cfg,
                                m_client_timeout,
                                m_timeout,
                                m_req_handshake,
                                m_bound_funcs),
            std::bind(&client::on_session_delete, this, std::placeholders::_1));
        m_session = ptr;
        return ptr->run() == connection_state::established;
    }

#endif

    /**
     * Used to notfiy the condition variable.
     * @param p
     */
    void on_session_delete(client_session_ *p)
    {
        delete p;
        {
            std::unique_lock<std::mutex> lock(m_expired_mtx);
            m_expired = true;
        }
        m_expired_cv.notify_all();
        RADRPC_LOG("client::on_session_delete");
    }

  public:
    /**
     * @param p_client_cfg The client config.
     * @param p_client_timeout The client timeout.
     */
    template <typename F = client_session_>
    client(client_config p_client_cfg,
           const client_timeout &p_client_timeout,
           typename std::enable_if<std::is_same<F, client_mode::plain>::value>
               * = nullptr) :
#ifdef RADRPC_SSL_SUPPORT
        m_ssl_ctx(ssl::context::sslv23),
#endif
        m_expired(true),
        m_client_cfg(std::move(p_client_cfg)),
        m_client_timeout(p_client_timeout),
        m_timeout({}),
        m_exit(false)
    {
        m_client_cfg.send_attempts = std::max(m_client_cfg.send_attempts, 1u),
        m_timeout.handshake_timeout = m_client_timeout.handshake_timeout;
        m_timeout.idle_timeout = duration::max();
        m_timeout.keep_alive_pings = false;
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * @param p_client_cfg The client config.
     * @param p_client_timeout The client timeout.
     * @param p_ssl_ctx The ssl context.
     */
    template <typename F = client_session_>
    client(client_config p_client_cfg,
           const client_timeout &p_client_timeout,
           ssl::context &&p_ssl_ctx,
           typename std::enable_if<std::is_same<F, client_mode::ssl>::value> * =
               nullptr) :
        m_ssl_ctx(std::move(p_ssl_ctx)),
        m_expired(true),
        m_client_cfg(std::move(p_client_cfg)),
        m_client_timeout(p_client_timeout),
        m_timeout({}),
        m_exit(false)
    {
        m_client_cfg.send_attempts = std::max(m_client_cfg.send_attempts, 1u),
        m_timeout.handshake_timeout = m_client_timeout.handshake_timeout;
        m_timeout.idle_timeout = duration::max();
        m_timeout.keep_alive_pings = false;
    }
#endif

    ~client()
    {
        RADRPC_LOG("~client");
        m_session_mtx.lock();
        m_exit = true;
        if (auto session = m_session.lock())
            session->close();

        if (!wait_destruction())
            RADRPC_LOG("~client: Timeout, this should not happen");

        if (m_io_worker.joinable())
            m_io_worker.join();
        m_session_mtx.unlock();
        RADRPC_LOG("~client: Done");
    }

    /**
     * Assign a handler to listen for
     * the server broadcasts with the specified id.
     * This function throws if used while a session is active.
     * [thread-safe]
     * @param broadcast_id The id to listen on.
     * @param handler The handler to call if a broadcast was received.
     * @return True if the handler was successfully inserted, false if not.
     */
    bool listen_broadcast(uint32_t broadcast_id,
                          std::function<void(receive_buffer &)> handler)
    {
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return false;
        std::unique_lock<std::mutex> lock(m_expired_mtx);
        if (!m_expired)
            RADRPC_THROW(
                "client::listen_broadcast: Cannot assign listen handler while "
                "session is connected, disconnect at first.");
        auto func_itr = m_bound_funcs.find(broadcast_id);
        if (func_itr != m_bound_funcs.end())
            RADRPC_THROW("client::listen_broadcast: The given id '" +
                         std::to_string(broadcast_id) +
                         "' was already bound to a function.");
        m_bound_funcs[broadcast_id] = std::move(handler);
        return true;
    }

    /**
     * Tries to connect to the server.
     * [thread-safe]
     * @return True if connected, false if not.
     */
    bool connect()
    {
        RADRPC_LOG("client::connect");
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return false;
        bool result = construct_session();
        return result;
    }

    /**
     * Tries to connect to the server.
     * [thread-safe]
     * @param attempts The attempts for connecting.
     * @param delay The delay between the attempts.
     * @return True if connected, false if not.
     */
    bool connect(unsigned int attempts, duration delay)
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

    /**
     * Disconnects from the server.
     * [thread-safe]
     */
    void disconnect()
    {
        RADRPC_LOG("client::disconnect");
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return;
        if (auto session = m_session.lock())
            session->close();
        if (!wait_destruction())
            RADRPC_THROW("client::disconnect: Timeout, this should not happen");
    }

    /**
     * Ping the server.
     * [thread-safe]
     * @return True if pong received, false if not.
     */
    bool ping()
    {
        RADRPC_LOG("client::ping");
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return false;
        if (!construct_session())
            return false;
        if (auto session = m_session.lock())
        {
            return session->ping();
        }
        return false;
    }

    /**
     * Send bytes with a call id.
     * The data will only be referenced.
     * After return the data will not be used anymore.
     * On failure it will reconnect once & will try to send
     * again based on 'client_config::send_attempts'.
     * [thread-safe]
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @return True if no error occured, false if not.
     */
    bool send(uint32_t call_id, const char *data_ptr, std::size_t data_size)
    {
        for (unsigned int i = 0; i < m_client_cfg.send_attempts; ++i)
        {
            if (i != 0)
                std::this_thread::sleep_for(m_client_cfg.send_attempt_delay);
            bool expired = true;
            boost::system::error_code ec;
            {
                std::shared_lock<std::shared_timed_mutex> read_lock(
                    m_session_mtx);
                if (m_exit)
                    return false;
                // Send
                if (auto session = m_session.lock())
                {
                    session->send(call_id, data_ptr, data_size, ec);
                    expired = false;
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

    /**
     * Send bytes with a call id.
     * The data will only be referenced.
     * After return the data will not be used anymore.
     * On failure it will reconnect once & will try to send
     * again based on 'client_config::send_attempts'.
     * [thread-safe]
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @return True if no error occured, false if not.
     */
    bool send(uint32_t call_id, const std::vector<char> &send_bytes)
    {
        return send(call_id, send_bytes.data(), send_bytes.size());
    }

    /**
     * Send bytes with a call id & wait for a response.
     * The data will only be referenced.
     * After return the data will not be used anymore.
     * On failure it will reconnect once & will try to send
     * again based on 'client_config::send_attempts'.
     * [thread-safe]
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @return True if no error occured & response received, false if not.
     */
    receive_buffer
    send_recv(uint32_t call_id, const char *data_ptr, std::size_t data_size)
    {
        receive_buffer result;
        for (unsigned int i = 0; i < m_client_cfg.send_attempts; ++i)
        {
            if (i != 0)
                std::this_thread::sleep_for(m_client_cfg.send_attempt_delay);
            bool expired = true;
            boost::system::error_code ec;
            {
                std::shared_lock<std::shared_timed_mutex> read_lock(
                    m_session_mtx);
                if (m_exit)
                    return result;
                // Send
                if (auto session = m_session.lock())
                {
                    session->send_recv(
                        call_id, data_ptr, data_size, result, ec);
                    expired = false;
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

    /**
     * Send bytes with a call id & wait for a response.
     * The data will only be referenced.
     * After return the data will not be used anymore.
     * On failure it will reconnect once & will try to send
     * again based on 'client_config::send_attempts'.
     * [thread-safe]
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @return True if no error occured & response received, false if not.
     */
    receive_buffer send_recv(uint32_t call_id,
                             const std::vector<char> &send_bytes)
    {
        return send_recv(call_id, send_bytes.data(), send_bytes.size());
    }

    /**
     * Sets the handshake which will be
     * sent to the server with 'connect()'.
     * [thread-safe]
     * @see radrpc::client::connect()
     */
    void set_handshake_request(handshake_request &req_handshake)
    {
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return;
        m_req_handshake = req_handshake;
    }

    /**
     * Gets the handshake received from the server
     * which can be retrieved after a successful 'connect()'.
     * [thread-safe]
     * @see radrpc::client::connect()
     * @return The received handshake from the server.
     */
    handshake_response get_handshake_response()
    {
        std::lock_guard<std::shared_timed_mutex> write_lock(m_session_mtx);
        if (m_exit)
            return handshake_response{};
        if (auto session = m_session.lock())
            return session->res_handshake;
        return handshake_response{};
    }

    /**
     * Sets the send timeout for the next 'send()' or 'send_recv()'.
     * Afterwards it will fallback to the default value specified
     * by the passed timeout in constructor.
     * This timeout scope is thread-local.
     * [thread-safe]
     * @param timeout The timeout.
     * @see radrpc::client::send
     * @see radrpc::client::send_recv
     * @see radrpc::detail::send_timeout
     */
    void set_send_timeout(duration timeout) { detail::send_timeout(timeout); }

    /**
     * Sets the response timeout for the next 'ping()' or 'send_recv()'.
     * Afterwards it will fallback to the default value specified
     * by the passed timeout in constructor.
     * This timeout scope is thread-local.
     * [thread-safe]
     * @param timeout The timeout.
     * @see radrpc::client::ping
     * @see radrpc::client::send_recv
     * @see radrpc::detail::response_timeout
     */
    void set_response_timeout(duration timeout)
    {
        detail::response_timeout(timeout);
    }
};

} // namespace radrpc

#endif // RADRPC_CLIENT_HPP
