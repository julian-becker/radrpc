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

#ifndef RADRPC_SERVER_HPP
#define RADRPC_SERVER_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/asio/signal_set.hpp>
#ifdef RADRPC_SSL_SUPPORT
#include <boost/asio/ssl/context.hpp>
#endif

#include <radrpc/common/server_config.hpp>
#include <radrpc/common/server_timeout.hpp>
#include <radrpc/common/session_context.hpp>

namespace radrpc {
namespace impl {
namespace server {

class listener;
class session_manager;

} // namespace server
} // namespace impl
} // namespace radrpc

namespace radrpc {

#ifdef RADRPC_SSL_SUPPORT
namespace ssl = boost::asio::ssl;
#endif

/**
 * A type for communicating with clients.
 */
class server
{
#ifdef RADRPC_SSL_SUPPORT
    /// The ssl context.
    ssl::context m_ssl_ctx;
#endif
    /// Used for stopping the workers.
    std::condition_variable m_cv_stop;

    /// Lock to use for condition variable.
    std::mutex m_stop_mtx;

    /// Common lock.
    std::mutex m_mtx;

    /// Used to check whether IO and workers are running
    bool m_running;

    /// Used for check if server was asynchron started
    bool m_async_start;

    /// Determine the amount of workers finished execution.
    unsigned int m_workers_done;

    /// The server config.
    const server_config m_server_cfg;

    /// The server timeout.
    const server_timeout m_server_timeout;

    /// The default session config.
    const session_config m_session_cfg;

    /// The manager shared among sessions.
    std::shared_ptr<impl::server::session_manager> m_manager;

    /// The io context.
    boost::asio::io_context m_io_ctx;

    /// The listener to accept new conenctions.
    std::shared_ptr<impl::server::listener> m_listener;

    /// Used to assign the desired signals.
    boost::asio::signal_set m_signals;

    /// Workers which are running the io context.
    std::vector<std::thread> m_workers;

    /**
     * Run a specific amount of workers without blocking.
     */
    void run_async_workers(
        const std::function<void()> &io_stopped_handler = nullptr);

    /**
     * @param ec
     * @param signal_code
     */
    void on_signal(const boost::system::error_code &ec, int signal_code);

  public:
    /**
     * @param p_server_cfg The server config.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The default session config.
     */
    server(const server_config &p_server_cfg,
           const server_timeout &p_server_timeout,
           const session_config &p_session_cfg);

#ifdef RADRPC_SSL_SUPPORT
    /**
     * @param p_server_cfg The server config.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The default session config.
     * @param p_ssl_ctx The ssl context to own.
     */
    server(const server_config &p_server_cfg,
           const server_timeout &p_server_timeout,
           const session_config &p_session_cfg,
           ssl::context &&p_ssl_ctx);
#endif

    ~server();

    /**
     * Starting the server.
     * This function returns if the io context was stopped
     * for e.g. with 'stop()'.
     * [thread-safe]
     * @see radrpc::server::stop
     * @see radrpc::server::async_start
     */
    void start();

    /**
     * Starting the server asynchronously.
     * This function returns immediately
     * and can be halted with 'stop()'
     * [thread-safe]
     * @see radrpc::server::stop
     * @see radrpc::server::start
     */
    void async_start(const std::function<void()> &io_stopped_handler = nullptr);

    /**
     * Stops the server.
     * If executed in one of the bound handlers (in executor itself)
     * it will just stop the io context, otherwise it will stop the io,
     * wait for and join the workers.
     * [thread-safe]
     */
    void stop();

    /**
     * Broadcasts data to all sessions.
     * The message will be copied one time.
     * [thread-safe]
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients.
     */
    void broadcast(uint32_t call_id, const std::vector<char> &send_bytes);

    /**
     * Broadcasts data to specific sessions.
     * The message will be copied one time.
     * [thread-safe]
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients.
     * @param session_ids The session ids to broadcast this message.
     */
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   const std::vector<uint64_t> &session_ids);

    /**
     * Broadcasts data to specific sessions.
     * The message will be copied one time.
     * [thread-safe]
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients.
     * @param session_ids The session ids to broadcast this message.
     */
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   const std::set<uint64_t> &session_ids);

    /**
     * Broadcasts data to specific sessions.
     * The message will be copied one time.
     * [thread-safe]
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients.
     * @param session_ids The session ids to broadcast this message.
     */
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   const std::unordered_set<uint64_t> &session_ids);

    /**
     * Returns the amount of sessions
     * by checking the shared reference count.
     * [thread-safe]
     * @return The amount of sessions.
     */
    long connections();

    /**
     * Get all session ids.
     * [thread-safe]
     * @return All session ids.
     */
    std::vector<uint64_t> get_session_ids();

    /**
     * Binds a handler specified by an id
     * which the client may call with its call_id.
     * The handler allows to execute common server tasks.
     * [thread-safe]
     * @param bind_id The id to listen on.
     * @param handler The handler to call if a request was received.
     * @return True if successfully bound the handler, false if not.
     */
    bool bind(uint32_t bind_id, std::function<void(session_context *)> handler);

    /**
     * Binds a handler that will fire
     * on each incoming session creation request.
     * The handler allows to lookup the ip.
     * [thread-safe]
     * @param handler The handler to call on each creation request.
     * @return True if bound successfully, false if not
     */
    bool bind_listen(std::function<bool(const std::string &)> handler);

    /**
     * Binds a handler that will fire when the session was created.
     * The handler allows to configure & inspect the
     * properties for the newly created session.
     * [thread-safe]
     * @param handler The handler to call on each new session.
     * @return True if bound successfully, false if not
     */
    bool bind_accept(std::function<bool(session_info &)> handler);

    /**
     * Binds a handler that will fire when the session is disconnected.
     * The handler allows to lookup the session's properties.
     * [thread-safe]
     * @param handler The handler to call if disconnected.
     * @return True if bound successfully, false if not
     */
    bool bind_disconnect(std::function<void(const session_info &)> handler);
};

} // namespace radrpc

#endif // RADRPC_SERVER_HPP