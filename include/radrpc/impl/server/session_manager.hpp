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

#ifndef RADRPC_IMPL_SERVER_SESSION_MANAGER_HPP
#define RADRPC_IMPL_SERVER_SESSION_MANAGER_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <radrpc/common/server_config.hpp>
#include <radrpc/common/session_config.hpp>
#include <radrpc/common/session_context.hpp>
#include <radrpc/common/streams.hpp>

namespace radrpc {
namespace core {
namespace data {

class push;

} // namespace data
} // namespace core
} // namespace radrpc

namespace radrpc {
namespace impl {
namespace server {

using namespace radrpc::common;
using namespace radrpc::core;

template <class Derived> class server_session;
template <typename T> class session_accept;

/**
 * A manager to track all sessions and to operate on them.
 */
class session_manager : public std::enable_shared_from_this<session_manager>
{
    typedef std::unordered_map<
        uint64_t,
        std::weak_ptr<server_session<session_accept<streams::plain>>>>
        plain_sessions;
        
#ifdef RADRPC_SSL_SUPPORT
    typedef std::unordered_map<
        uint64_t,
        std::weak_ptr<server_session<session_accept<streams::ssl>>>>
        ssl_sessions;
#endif

    /// The reference count before any session is created.
    long m_sub_refs;

    /// The amount of the queued messages.
    std::atomic<long> m_msg_queued;

    /// Lock for operating on 'plain_sessions'
    std::mutex m_plain_mtx;

    /// Holds all plain sessions.
    plain_sessions m_plain_sessions;

#ifdef RADRPC_SSL_SUPPORT
    /// Lock for operating on 'ssl_sessions'
    std::mutex m_ssl_mtx;

    /// Holds all ssl sessions.
    ssl_sessions m_ssl_sessions;
#endif

    /**
     * Broadcasts data to specific sessions.
     * @tparam StlContainer The container type to use, usually a vector or set.
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients
     * @param session_ids The session ids to broadcast this message.
     */
    template <typename StlContainer>
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   StlContainer *session_ids);

    /**
     * Used for decrementing the message counter.
     * @param p
     */
    void on_msg_sent(const data::push *p);

  public:
    /// The server config to use.
    const server_config server_cfg;

    /// The bound handlers.
    std::unordered_map<uint32_t, std::function<void(session_context *)>>
        bound_funcs;

    /// The bound accept handler.
    std::function<bool(session_info &)> on_accept;

    /// The bound listen handler.
    std::function<bool(const std::string &)> on_listen;

    /// The bound disconnect handler.
    std::function<void(const session_info &)> on_disconnect;

    /**
     * @param p_server_cfg The server config to use.
     */
    explicit session_manager(server_config p_server_cfg);

    /**
     * Sets the reference count to subtract
     * later to get the actual session count.
     * This function can only be called once.
     */
    void set_subtract_refs();

    /**
     * Checks whether the maximum sessions is reached.
     * @return True if full, false if not
     */
    bool is_full();

    /**
     * Returns the amount of sessions.
     * by checking the shared reference count.
     * @return The amount of sessions.
     */
    long connections();

    /**
     * Adds a plain session to the list.
     * @param session The plain session to add.
     */
    void add_plain_session(
        const std::shared_ptr<server_session<session_accept<streams::plain>>>
            &session);

    /**
     * Removes a plain session from the list.
     * @tparam StreamType
     * @param id The id of the session to remove.
     */
    void remove_plain_session(uint64_t id);

#ifdef RADRPC_SSL_SUPPORT

    /**
     * Adds a ssl session to the list.
     * @param session The ssl session to add.
     */
    void add_ssl_session(
        const std::shared_ptr<server_session<session_accept<streams::ssl>>>
            &session);

    /**
     * Removes a ssl session from the list.
     * @tparam StreamType
     * @param id The id of the session to remove.
     */
    void remove_ssl_session(uint64_t id);

#endif

    /**
     * Broadcasts data to all sessions.
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients
     */
    void broadcast(uint32_t call_id, const std::vector<char> &send_bytes);

    /**
     * Broadcasts data to specific sessions.
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients
     * @param session_ids The session ids to broadcast this message.
     */
    template <typename StlContainer>
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   const StlContainer &session_ids);

    /**
     * Get all session ids.
     * @return All session ids.
     */
    std::vector<uint64_t> get_session_ids();
};

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_SESSION_MANAGER_HPP