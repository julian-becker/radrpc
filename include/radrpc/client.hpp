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

#ifndef RADRPC_CLIENT_HPP
#define RADRPC_CLIENT_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <unordered_map>

#include <radrpc/config.hpp>
#include <radrpc/common/client_config.hpp>
#include <radrpc/common/client_timeout.hpp>
#include <radrpc/common/receive_buffer.hpp>
#include <radrpc/common/streams.hpp>
#include <radrpc/core/timeout.hpp>

namespace radrpc {
namespace impl {
namespace client {

template <typename T> class connector;

} // namespace client
} // namespace impl
} // namespace radrpc

namespace radrpc {

/**
 * @brief A type to communicate with the server
 *
 */
class client
{
  public:
    class plain;
#ifdef RADRPC_SSL_SUPPORT
    class ssl;
#endif

  private:
    class base
    {
      public:
        typedef radrpc::impl::client::connector<common::streams::plain>
            connector_plain;
#ifdef RADRPC_SSL_SUPPORT
        typedef radrpc::impl::client::connector<common::streams::ssl>
            connector_ssl;
#endif
      private:
#ifdef RADRPC_SSL_SUPPORT
        /// Used to check whether stream is ssl.
        const bool m_is_ssl;

        /// The ssl context.
        boost::asio::ssl::context m_ssl_ctx;
#endif
        /// Used for waiting on session destruction
        std::condition_variable m_expired_cv;

        /// Lock to use for condition variable.
        std::mutex m_expired_mtx;

        /// Used to check whether the session is expired.
        bool m_expired;

        /// The bound broadcast handlers.
        std::unordered_map<uint32_t, std::function<void(receive_buffer &)>>
            m_bound_funcs;

        /// Lock to operate on session.
        std::shared_timed_mutex m_session_mtx;

        /// The io context.
        std::unique_ptr<boost::asio::io_context> m_io_ctx;

        /// The worker for running the io context.
        std::thread m_io_worker;

        /// The client config.
        client_config m_client_cfg;

        /// The client timeout.
        client_timeout m_client_timeout;

        /// The timeout to use in stream.
        websocket::stream_base::timeout m_timeout;

        /// The weak reference of the plain session.
        std::weak_ptr<connector_plain> m_plain_session;

#ifdef RADRPC_SSL_SUPPORT
        /// The weak reference of the ssl session.
        std::weak_ptr<connector_ssl> m_ssl_session;
#endif
        /// The request handshake to send to the server.
        handshake_request m_req_handshake;

        /// The response handshake from the server.
        handshake_response m_res_handshake;

        /// Used to check if client is going to exit.
        bool m_exit;

        /**
         * Used to notfiy the condition variable.
         * @param p
         */
        void on_plain_session_delete(connector_plain *p);

#ifdef RADRPC_SSL_SUPPORT
        /**
         * Used to notfiy the condition variable.
         * @param p
         */
        void on_ssl_session_delete(connector_ssl *p);
#endif

        /**
         * @brief
         *
         */
        void expire_session();

        /**
         * @brief
         *
         */
        void init();

        /**
         * @brief
         *
         */
        void destruct();

        /**
         * Waits for the session destruction.
         * @return True if destructed, false if it might be in deadlock.
         */
        bool wait_destruction();

        /**
         * @brief Checks whether a new client connection should be constructed.
         *
         * @return true If new connection is needed.
         * @return false If a connection is already available.
         */
        bool check_construct();

        /**
         * Constructs a session if expired.
         * @return 	True if session is not expired or connected
         * successfully, false if constructed session failed to run.
         */
        bool construct_session();

        /**
         * Constructs a plain session if expired.
         * @return 	True if session is not expired or connected
         * successfully, false if constructed session failed to run.
         */
        bool construct_plain_session();

#ifdef RADRPC_SSL_SUPPORT
        /**
         * Constructs a ssl session if expired.
         * @return 	True if session is not expired or connected
         * successfully, false if constructed session failed to run.
         */
        bool construct_ssl_session();
#endif

      public:
        friend class radrpc::client::plain;

        base(client_config p_client_cfg,
             const client_timeout &p_client_timeout);

#ifdef RADRPC_SSL_SUPPORT

        friend class radrpc::client::ssl;

        base(client_config p_client_cfg,
             const client_timeout &p_client_timeout,
             boost::asio::ssl::context &&p_ssl_ctx);

#endif

        ~base();

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
                              std::function<void(receive_buffer &)> handler);

        /**
         * Tries to connect to the server.
         * [thread-safe]
         * @return True if connected, false if not.
         */
        bool connect();

        /**
         * Tries to connect to the server.
         * [thread-safe]
         * @param attempts The attempts for connecting.
         * @param delay The delay between the attempts.
         * @return True if connected, false if not.
         */
        bool connect(unsigned int attempts, duration delay);

        /**
         * Disconnects from the server.
         * [thread-safe]
         */
        void disconnect();

        /**
         * Ping the server.
         * [thread-safe]
         * @return True if pong received, false if not.
         */
        bool ping();

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
        bool send(uint32_t call_id,
                  const char *data_ptr,
                  std::size_t data_size);

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
        bool send(uint32_t call_id, const std::vector<char> &send_bytes);

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
                                 const char *data_ptr,
                                 std::size_t data_size);

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
                                 const std::vector<char> &send_bytes);

        /**
         * Sets the handshake which will be
         * sent to the server with 'connect()'.
         * [thread-safe]
         * @see radrpc::client::connect()
         */
        void set_handshake_request(handshake_request &req_handshake);

        /**
         * Gets the handshake received from the server
         * which can be retrieved after a successful 'connect()'.
         * [thread-safe]
         * @see radrpc::client::connect()
         * @return The received handshake from the server.
         */
        handshake_response get_handshake_response();

        /**
         * Sets the send timeout for the next 'send()' or 'send_recv()'.
         * Afterwards it will fallback to the default value specified
         * by the passed timeout in constructor.
         * This timeout scope is thread-local.
         * [thread-safe]
         * @param timeout The timeout.
         * @see radrpc::client::send
         * @see radrpc::client::send_recv
         * @see radrpc::core::send_timeout
         */
        void set_send_timeout(duration timeout);

        /**
         * Sets the response timeout for the next 'ping()' or 'send_recv()'.
         * Afterwards it will fallback to the default value specified
         * by the passed timeout in constructor.
         * This timeout scope is thread-local.
         * [thread-safe]
         * @param timeout The timeout.
         * @see radrpc::client::ping
         * @see radrpc::client::send_recv
         * @see radrpc::core::response_timeout
         */
        void set_response_timeout(duration timeout);
    };

  public:
    class plain : public base
    {
      public:
        plain(client_config p_client_cfg,
              const client_timeout &p_client_timeout);
        ~plain();
    };

#ifdef RADRPC_SSL_SUPPORT
    class ssl : public base
    {
      public:
        ssl(client_config p_client_cfg,
            const client_timeout &p_client_timeout,
            boost::asio::ssl::context &&p_ssl_ctx);
        ~ssl();
    };
#endif
};

} // namespace radrpc

#endif // RADRPC_CLIENT_HPP