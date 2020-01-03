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

#ifndef RADRPC_IMPL_SERVER_SERVER_SESSION_HPP
#define RADRPC_IMPL_SERVER_SERVER_SESSION_HPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

//#include <boost/asio/strand.hpp>
#include <boost/beast/core/detect_ssl.hpp>
//#include <boost/beast/core.hpp>
//#include <boost/beast/websocket.hpp>
//#include <boost/beast/http/read.hpp>

#include <radrpc/debug.hpp>
#include <radrpc/types.hpp>
#include <radrpc/detail/data.hpp>

namespace radrpc {

/**
 * Type to expose the server session to the bound handlers.
 */
class session_context
{
  protected:
    bool m_bound_close; ///< Used to check whether the bound handler requests to
                        ///< close
    detail::io_header
        m_header; ///< The io_header to send along with the bytes to the client.
    boost::beast::flat_buffer
        m_receive_buffer; ///< The buffer to receive incoming client requests.
    boost::asio::const_buffer
        m_receive_buffer_ref; ///< The buffer's reference to the actual data.
    session_config m_config;  ///< The config to use for this session.
    handshake_request m_req_handshake; ///< The handshake from the client.
    handshake_response
        m_res_handshake; ///< The handshake to send to the client.

    ~session_context() = default;

  public:
    uint64_t id;                   ///< The unique id of this session.
    const std::string remote_host; ///< The clients ip address.
    std::vector<char> response;    ///< The bytes to send for a response.

    /**
     *
     * @param p_id The session id of the context.
     * @param p_remote_host The ip address of the client.
     * @param p_session_cfg The config for this session.
     * @param p_server_timeout The server timeout.
     */
    session_context(uint64_t p_id,
                    std::string p_remote_host,
                    const session_config &p_session_cfg,
                    const server_timeout &p_server_timeout) :
        m_bound_close(false),
        m_header(0, 0),
        m_config(p_session_cfg),
        id(p_id),
        remote_host(std::move(p_remote_host))
    {
    }

    /**
     * Returns the pointer of the received data.
     * @return The pointer of the buffer.
     */
    const char *data() const
    {
        return reinterpret_cast<const char *>(m_receive_buffer_ref.data());
    }

    /**
     * Returns the size of the received bytes.
     * @return The size of the buffer.
     */
    std::size_t size() const { return m_receive_buffer_ref.size(); }

    /**
     * Manually clears the received bytes.
     */
    void clear_buffer() { m_receive_buffer.consume(m_receive_buffer.size()); }

    /**
     * Asks the session to close.
     */
    void close() { m_bound_close = true; }

    /**
     * Returns the used session config.
     * @return The session config.
     */
    const session_config &config() const { return m_config; }

    /**
     * Returns the received handshake from the client.
     * @return The request handshake.
     */
    const handshake_request &request_handshake() const
    {
        return m_req_handshake;
    }
};

} // namespace radrpc

namespace radrpc {
namespace impl {
namespace server {

template <typename T> class session_accept;

/**
 * Type to specifiy the ping states.
 */
enum class ping_state
{
    ping_next,
    ping_send,
    ping_close
};

/**
 * Represents a server session to communicate with the client.
 * @tparam Derived The accept facility.
 * @tparam SharedRef The shared reference, usually the session manager.
 */
template <class Derived, class SharedRef>
class server_session : private session_context
{
    bool m_write_error; ///< Info for read error.
    bool m_read_error;  ///< Info for write error.
    std::shared_ptr<SharedRef>
        m_manager; ///< The manager shared among sessions.
    std::deque<std::shared_ptr<detail::data_push>>
        m_queue; ///< A queue to hold the data to send.
    const server_timeout &m_server_timeout; ///< The used server timeout.

    friend class session_manager;
    template <typename T> friend class session_accept;

    Derived &derived() { return static_cast<Derived &>(*this); }

    /**
     * Listen to all writes from the client.
     * This function must be called within the executor context.
     */
    void read()
    {
        derived().m_stream.async_read(
            m_receive_buffer,
            boost::beast::bind_front_handler(&server_session::on_read,
                                             derived().shared_from_this()));
    }

    /**
     * Calls the bound handler with the specified id & start a next read.
     * This function must be called within the executor context.
     */
    void call_function()
    {
        // Check io_header & call bound function
        m_receive_buffer_ref =
            boost::beast::buffers_front(m_receive_buffer.data());
        if (m_receive_buffer_ref.size() >= sizeof(detail::io_header))
        {
            m_header = *reinterpret_cast<const detail::io_header *>(
                m_receive_buffer_ref.data());
            uint32_t call_id = m_header.call_id;
            auto func_itr = m_manager->bound_funcs.find(call_id);
            m_receive_buffer.consume(sizeof(detail::io_header));
            m_receive_buffer_ref =
                boost::beast::buffers_front(m_receive_buffer.data());
            if (func_itr != m_manager->bound_funcs.end())
            {
                if (func_itr->second)
                {
                    // Call bound function
                    func_itr->second(this);
                    // Check if bound function requests to close
                    if (m_bound_close)
                    {
                        RADRPC_LOG("server_session::call_function: Bound "
                                   "function close request");
                        derived().close_session();
                        return;
                    }
                    // Check if bound function has added bytes to send back to
                    // client
                    if (!response.empty())
                    {
                        auto push = new detail::data_push(m_header);
                        push->body.swap(response);
                        handle_send(std::shared_ptr<detail::data_push>(push));
                    }
                }
            }
            else
            {
                RADRPC_LOG("server_session::call_function: Call id "
                           << call_id << " was not bound to any function");
            }
        }
        else
        {
            RADRPC_LOG("server_session::call_function: Invalid buffer");
        }
    }

    /**
     * Send data to the client.
     * This function must be called within the executor context.
     * @param data
     */
    void handle_send(const std::shared_ptr<detail::data_push> &data)
    {
        if (derived().m_close || derived().m_close_received || m_write_error ||
            m_read_error)
            return;
        if (m_queue.size() >= config::queue_send_max)
            return;
        m_queue.push_back(data);
        if (m_queue.size() > 1)
            return;
        write();
    }

    /**
     * Write the message queued by 'handle_send()'.
     * This function must be called within the executor context.
     */
    void write()
    {
        derived().m_stream.async_write(
            std::vector<boost::asio::const_buffer>{
                boost::asio::buffer(
                    reinterpret_cast<char *>(&m_queue.front()->header),
                    sizeof(detail::io_header)),
                boost::asio::buffer(m_queue.front()->body),
            },
            boost::beast::bind_front_handler(&server_session::on_write,
                                             derived().shared_from_this()));
        // IF we are going to operate on the queue for example with 'clear()',
        // it is needed to also pass the reference of the data to the handler,
        // because it is only referenced in the queue.
        // Clearing it while async_write is processing means a 'heap use after
        // free' error. The alternative handler binding could look like this:
        // boost::beast::bind_front_handler(&server_session::on_write,
        //                                  derived().shared_from_this(),
        //                                  m_queue.front()->shared_from_this())
        // The handler needs also to be changed.
        // But since removing entries happens only in 'on_write()' it is fine
        // now.
    }

    /**
     * @param ec
     * @param bytes_transferred
     */
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec || derived().m_close || derived().m_close_received || m_read_error)
        {
            RADRPC_LOG("server_session::on_write: " << ec.message());
            m_write_error = true;
            m_queue.clear();
            derived().close_session();
            return;
        }
        m_queue.erase(m_queue.begin());
        if (!m_queue.empty())
            write();
    }

    /**
     * @param ec
     * @param bytes_transferred
     */
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec || derived().m_close_received)
        {
            RADRPC_LOG("server_session::on_read: " << ec.message());
            m_read_error = true;
            derived().close_session();
            return;
        }
        RADRPC_LOG("server_session::on_read: " << bytes_transferred << "bytes");
        if (!derived().m_close && !m_write_error)
            call_function();
        m_receive_buffer.consume(m_receive_buffer.size());
        response = std::vector<char>();
        // Read operations allowed, since 
        // it would read until an error occures.
        read();
    }

  public:
    session_info info; ///< The session info to expose in handlers.

    /**
     * @param p_remote_host The client's ip address.
     * @param p_manager The manager shared among sessions.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The config to use for this session.
     */
    explicit server_session(std::string p_remote_host,
                            const std::shared_ptr<SharedRef> &p_manager,
                            const server_timeout &p_server_timeout,
                            const session_config &p_session_cfg) :
        radrpc::session_context(
            reinterpret_cast<uint64_t>(&(derived().m_stream)),
            std::move(p_remote_host),
            p_session_cfg,
            p_server_timeout),
        m_write_error(false),
        m_read_error(false),
        m_manager(p_manager->shared_from_this()),
        m_server_timeout(p_server_timeout),
        info(session_info{id,
                          remote_host,
                          m_req_handshake,
                          m_res_handshake,
                          m_config})
    {
        RADRPC_LOG("+server_session");
    }

    ~server_session()
    {
        RADRPC_LOG("~server_session");
        if (m_manager->on_disconnect)
            m_manager->on_disconnect(std::ref(info));
    }

    /**
     * @param data
     */
    void send(std::shared_ptr<detail::data_push> data)
    {
        boost::asio::post(
            derived().m_stream.get_executor(),
            boost::beast::bind_front_handler(&server_session::handle_send,
                                             derived().shared_from_this(),
                                             data));
    }
};

/**
 * A manager to track all sessions and to operate on them.
 */
class session_manager : public std::enable_shared_from_this<session_manager>
{
    typedef std::unordered_map<uint64_t,
                               std::weak_ptr<server_session<
                                   session_accept<server_streams::plain_stream>,
                                   session_manager>>>
        plain_sessions;
#ifdef RADRPC_SSL_SUPPORT
    typedef std::unordered_map<
        uint64_t,
        std::weak_ptr<server_session<session_accept<server_streams::ssl_stream>,
                                     session_manager>>>
        ssl_sessions;
#endif
    long m_sub_refs; ///< The reference count before any session is created.
    std::atomic<long> m_msg_queued;  ///< The amount of the queued messages.
    std::mutex m_plain_mtx;          ///< Lock for operating on 'plain_sessions'
    plain_sessions m_plain_sessions; ///< Holds all plain sessions.
#ifdef RADRPC_SSL_SUPPORT
    std::mutex m_ssl_mtx;        ////< Lock for operating on 'ssl_sessions'
    ssl_sessions m_ssl_sessions; ///< Holds all ssl sessions.
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
                   StlContainer *session_ids)
    {
        if (m_msg_queued >= config::queue_send_max)
        {
            RADRPC_LOG("session_manager::broadcast: Queue is full");
            return;
        }
        if (connections() == 0)
            return;
        auto push = new detail::data_push(detail::io_header(call_id, 0));
        push->body = send_bytes;
        auto push_ptr = std::shared_ptr<detail::data_push>(
            push,
            std::bind(&session_manager::on_msg_sent,
                      shared_from_this(),
                      std::placeholders::_1));
        m_msg_queued++;

        // Copy sessions to avoid long locking.
        plain_sessions plain_sessions_tmp;
        {
            std::unique_lock<std::mutex> lock(m_plain_mtx);
            if (session_ids)
            {
                for (const auto &id : *session_ids)
                {
                    auto session_itr = m_plain_sessions.find(id);
                    if (session_itr != m_plain_sessions.end())
                        plain_sessions_tmp.emplace(session_itr->first,
                                                   session_itr->second);
                }
            }
            else
            {
                plain_sessions_tmp = m_plain_sessions;
            }
        }
        for (auto const &s : plain_sessions_tmp)
            if (auto session = s.second.lock())
                session->send(push_ptr);



#ifdef RADRPC_SSL_SUPPORT
        // Copy sessions to avoid long locking.
        ssl_sessions ssl_sessions_tmp;
        {
            std::unique_lock<std::mutex> lock(m_ssl_mtx);
            if (session_ids)
            {
                for (const auto &id : *session_ids)
                {
                    auto session_itr = m_ssl_sessions.find(id);
                    if (session_itr != m_ssl_sessions.end())
                        ssl_sessions_tmp.emplace(session_itr->first,
                                                 session_itr->second);
                }
            }
            else
            {
                ssl_sessions_tmp = m_ssl_sessions;
            }
        }
        for (auto const &s : ssl_sessions_tmp)
            if (auto session = s.second.lock())
                session->send(push_ptr);
#endif
    }

    /**
     * Used for decrementing the message counter.
     * @param p
     */
    void on_msg_sent(detail::data_push *p)
    {
        m_msg_queued--;
        RADRPC_LOG("session_manager::on_msg_sent: " << m_msg_queued);
        delete p;
    }

  public:
    const server_config server_cfg; ///< The server config to use.
    std::unordered_map<uint32_t,
                       std::function<void(session_context *)>>
        bound_funcs; ///< The bound handlers.
    std::function<bool(session_info &)>
        on_accept; ///< The bound accept handler.
    std::function<bool(const std::string &)>
        on_listen; ///< The bound listen handler.
    std::function<void(const session_info &)>
        on_disconnect; ///< The bound disconnect handler.

    /**
     * @param p_server_cfg The server config to use.
     */
    explicit session_manager(server_config p_server_cfg) :
        m_sub_refs(0),
        m_msg_queued(0),
        server_cfg(std::move(p_server_cfg)),
        on_accept(nullptr),
        on_listen(nullptr),
        on_disconnect(nullptr)
    {
    }

    /**
     * Sets the reference count to subtract
     * later to get the actual session count.
     * This function can only be called once.
     */
    void set_subtract_refs()
    {
        // This can be only called once
        RADRPC_ASSERT(m_sub_refs == 0);
        m_sub_refs = shared_from_this().use_count();
        RADRPC_LOG("session_manager::set_subtract_refs: " << m_sub_refs);
    }

    /**
     * Checks whether the maximum sessions is reached.
     * @return True if full, false if not
     */
#pragma warning(suppress: 4018)
    bool is_full() { return connections() >= server_cfg.max_sessions; }

    /**
     * Returns the amount of sessions.
     * by checking the shared reference count.
     * @return The amount of sessions.
     */
    long connections()
    {
        // Use max just in case...
        return std::max(shared_from_this().use_count() - m_sub_refs, 0l);
    }

    /**
     * Adds a plain session to the list.
     * @param session The plain session to add.
     */
    void
    add_session(const std::shared_ptr<
                server_session<session_accept<server_streams::plain_stream>,
                               session_manager>> &session)
    {
        std::unique_lock<std::mutex> lock(m_plain_mtx);
        m_plain_sessions[session->id] = std::weak_ptr<
            server_session<session_accept<server_streams::plain_stream>,
                           session_manager>>(session);
    }

    /**
     * Removes a plain session from the list.
     * @tparam StreamType
     * @param id The id of the session to remove.
     */
    template <typename StreamType>
    typename std::enable_if<
        std::is_same<StreamType, server_streams::plain_stream>::value,
        void>::type
    remove_session(uint64_t id)
    {
        std::unique_lock<std::mutex> lock(m_plain_mtx);
        auto session_itr = m_plain_sessions.find(id);
        if (session_itr != m_plain_sessions.end())
        {
            m_plain_sessions.erase(session_itr);
        }
    }

#ifdef RADRPC_SSL_SUPPORT

    /**
     * Adds a ssl session to the list.
     * @param session The ssl session to add.
     */
    void add_session(const std::shared_ptr<
                     server_session<session_accept<server_streams::ssl_stream>,
                                    session_manager>> &session)
    {
        std::unique_lock<std::mutex> lock(m_ssl_mtx);
        m_ssl_sessions[session->id] = std::weak_ptr<
            server_session<session_accept<server_streams::ssl_stream>,
                           session_manager>>(session);
    }

    /**
     * Removes a ssl session from the list.
     * @tparam StreamType
     * @param id The id of the session to remove.
     */
    template <typename StreamType>
    typename std::enable_if<
        std::is_same<StreamType, server_streams::ssl_stream>::value,
        void>::type
    remove_session(uint64_t id)
    {
        std::unique_lock<std::mutex> lock(m_ssl_mtx);
        auto session_itr = m_ssl_sessions.find(id);
        if (session_itr != m_ssl_sessions.end())
        {
            m_ssl_sessions.erase(session_itr);
        }
    }

#endif

    /**
     * Broadcasts data to all sessions.
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients
     */
    void broadcast(uint32_t call_id, const std::vector<char> &send_bytes)
    {
        broadcast(call_id, send_bytes, (std::vector<char> *)nullptr);
    }

    /**
     * Broadcasts data to specific sessions.
     * @param call_id The id to call on the clients.
     * @param send_bytes The bytes to send to the clients
     * @param session_ids The session ids to broadcast this message.
     */
    template <typename StlContainer>
    void broadcast(uint32_t call_id,
                   const std::vector<char> &send_bytes,
                   const StlContainer &session_ids)
    {
        broadcast(call_id, send_bytes, &session_ids);
    }

    /**
     * Get all session ids.
     * @return All session ids.
     */
    std::vector<uint64_t> get_session_ids()
    {
        std::vector<uint64_t> ids;
        std::unique_lock<std::mutex> plain_lock(m_plain_mtx);
#ifdef RADRPC_SSL_SUPPORT
        std::unique_lock<std::mutex> ssl_lock(m_ssl_mtx);
        ids.reserve(m_plain_sessions.size() + m_ssl_sessions.size());
        for (const auto &id : m_plain_sessions)
            ids.push_back(id.first);
        for (const auto &id : m_ssl_sessions)
            ids.push_back(id.first);
#else
        ids.reserve(m_plain_sessions.size());
        for (const auto &id : m_plain_sessions)
            ids.push_back(id.first);
#endif
        return ids;
    }
};

/**
 * Represents a type to wrap around server_session
 * to provide a accept facility.
 * @tparam StreamType The stream to use.
 */
template <class StreamType>
class session_accept
    : public server_session<session_accept<StreamType>, session_manager>,
      public std::enable_shared_from_this<session_accept<StreamType>>
{
    boost::beast::flat_buffer
        m_handshake_buffer;            ///< The buffer to use for handshakes.
    StreamType m_stream;               ///< The stream to use.
    boost::asio::steady_timer m_timer; ///< Timer for ping/pong activity.
    bool m_established;      ///< Used to check whether the connection was
                             ///< established.
    bool m_close;            ///< Used to check if close was already executed.
    bool m_close_received;   ///< Used to check if a close was received.
    ping_state m_ping_state; ///< The current state of the ping/pong procedure.

    friend class server_session<session_accept, session_manager>;

    server_session<session_accept<StreamType>, session_manager> &base()
    {
        return static_cast<
            server_session<session_accept<StreamType>, session_manager> &>(
            *this);
    }

    /**
     * Checks whether this class is using a ssl stream.
     * @return True if ssl stream is used, false if not.
     */
    template <typename F = StreamType>
    typename std::enable_if<
        std::is_same<F, server_streams::plain_stream>::value,
        bool>::type
    is_ssl()
    {
        return false;
    }

    /**
     * Starts a ssl handshake.
     */
    template <typename F = StreamType>
    typename std::enable_if<
        std::is_same<F, server_streams::plain_stream>::value,
        void>::type
    ssl_handshake()
    {
        on_ssl_handshake(boost::beast::error_code{}, 0);
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * Checks whether this class is using a ssl stream.
     * @return True if ssl stream is used, false if not.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, server_streams::ssl_stream>::value,
                            bool>::type
    is_ssl()
    {
        return true;
    }

    /**
     * Starts a ssl handshake.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, server_streams::ssl_stream>::value,
                            void>::type
    ssl_handshake()
    {
        boost::beast::get_lowest_layer(m_stream).expires_after(
            base().m_server_timeout.handshake_or_close_timeout);
        m_stream.next_layer().async_handshake(
            ssl::stream_base::server,
            m_handshake_buffer.data(),
            boost::beast::bind_front_handler(&session_accept::on_ssl_handshake,
                                             this->shared_from_this()));
    }
#endif

    /**
     * @param ec
     * @param bytes_used
     */
    void on_ssl_handshake(boost::beast::error_code ec, std::size_t bytes_used)
    {
        if (ec)
        {
            RADRPC_LOG("session_accept::on_ssl_handshake: " << ec.message());
            return;
        }
        // Consume the portion of the buffer used by the handshake
        m_handshake_buffer.consume(bytes_used);
        // Read client's request handshake
        boost::beast::get_lowest_layer(m_stream).expires_after(
            base().m_server_timeout.handshake_or_close_timeout);
        boost::beast::http::async_read(
            m_stream.next_layer(),
            m_handshake_buffer,
            base().m_req_handshake,
            boost::beast::bind_front_handler(&session_accept::on_read_handshake,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     * @param bytes_transferred
     */
    void on_read_handshake(boost::beast::error_code ec,
                           std::size_t bytes_transferred)
    {
        if (ec || m_handshake_buffer.size() !=
                      0) // Clients should not begin sending WebSocket frames
        {
            RADRPC_LOG("session_accept::on_read_handshake: " << ec.message());
            return;
        }
        m_handshake_buffer.consume(m_handshake_buffer.size());

        // Let handler inspect/configure session and allow a customized response
        // handshake
        if (base().m_manager->on_accept &&
            !base().m_manager->on_accept(std::ref(base().info)))
        {
            RADRPC_LOG(
                "session_accept::on_read_handshake: Rejected connection");
            return;
        }
        m_stream.read_message_max(base().m_config.max_transfer_bytes +
                                  sizeof(detail::io_header));

        // Set the response handshake
        m_stream.set_option(websocket::stream_base::decorator(
            [set_res{base().m_res_handshake}](handshake_response &res) {
                for (const auto &field : set_res)
                {
                    if (field.name() == boost::beast::http::field::unknown)
                        res.insert(field.name_string(), field.value());
                    else
                        res.set(field.name(), field.value());
                }
            }));

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        boost::beast::get_lowest_layer(m_stream).expires_never();

        // Set the websocket stream timeout system
        websocket::stream_base::timeout timeout{};
        timeout.handshake_timeout =
            base().m_server_timeout.handshake_or_close_timeout;
        timeout.idle_timeout = duration::max();
        timeout.keep_alive_pings = false;
        m_stream.set_option(timeout);

        // Accept with the read client handshake
        m_stream.async_accept(
            base().m_req_handshake,
            boost::beast::bind_front_handler(&session_accept::on_accept,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_accept(boost::system::error_code ec)
    {
        if (ec)
        {
            RADRPC_LOG("session_accept::on_accept: " << ec.message());
            return;
        }
        m_stream.control_callback(
            std::bind(&session_accept::on_control_callback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));
        RADRPC_LOG("server_session::on_accept: Connection established");
        m_established = true;

        // Use a custom version of ping activity instead of
        // the internal by tcp_stream,
        // since this throws an invalid executor at stress tests.
        // It may be a resource issue with the stream and its
        // associated executor doesn't live long enough.
        if (base().m_config.ping_delay != duration::zero())
        {
            m_timer.expires_after(base().m_config.ping_delay);
            m_timer.async_wait(boost::beast::bind_front_handler(
                &session_accept::on_timer, this->shared_from_this()));
        }

        base().m_manager->add_session(this->shared_from_this());
        base().read();
    }

    /**
     * Closes the session by 'async_close()'.
     * This works by sending a close frame, waiting
     * for a timeout (option was set) & close itself.
     */
    void close_session()
    {
        if (base().m_config.ping_delay != duration::zero())
        {
            m_ping_state = ping_state::ping_close;
            m_timer.expires_after(std::chrono::seconds(0));
        }
        else
        {
            shutdown();
        }
    }

    /**
     * @param ec
     */
    void on_timer(boost::beast::error_code ec)
    {
        if (ec && ec != boost::asio::error::operation_aborted)
            return;
        // Check if the timer really expired since the deadline may have moved
        if (m_timer.expiry() <= std::chrono::steady_clock::now())
        {
            if (m_stream.is_open() && m_ping_state == ping_state::ping_next)
            {
                m_ping_state = ping_state::ping_send;
                m_stream.async_ping(
                    {},
                    boost::beast::bind_front_handler(&session_accept::on_ping,
                                                     this->shared_from_this()));
            }
            else
            {
                shutdown();
                return;
            }
        }
        m_timer.expires_after(base().m_config.ping_delay);
        m_timer.async_wait(boost::beast::bind_front_handler(
            &session_accept::on_timer, this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_ping(boost::system::error_code ec)
    {
        if (ec)
            RADRPC_LOG("session_accept::on_ping: " << ec.message());
    }

    /**
     * @param kind
     * @param payload
     */
    void on_control_callback(websocket::frame_type kind,
                             boost::beast::string_view payload)
    {
        boost::ignore_unused(payload);
        if (kind == websocket::frame_type::close)
        {
            RADRPC_LOG("session_accept::on_control_callback: Received close");
            m_close_received = true;
            return;
        }
        m_ping_state = ping_state::ping_next;
        m_timer.expires_after(base().m_config.ping_delay);
    }

    void shutdown()
    {
        if (m_close || m_close_received)
            return;
        m_close = true;
        RADRPC_LOG("session_accept::shutdown");
        // This will shutdown the ssl stream & socket itself.
        // Timeout is also handled if set
        m_stream.async_close(
            websocket::close_code::normal,
            boost::beast::bind_front_handler(&session_accept::on_shutdown,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_shutdown(boost::system::error_code ec)
    {
        if (ec)
            RADRPC_LOG("session_accept::on_shutdown: " << ec.message());
    }

  public:
    /**
     * @param p_stream The stream to own.
     * @param p_handshake_buffer The buffer to own.
     * @param p_manager The manager shared among sessions.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The config for the session.
     */
    template <typename F = StreamType>
    explicit session_accept(
        boost::beast::tcp_stream &&p_stream,
        boost::beast::flat_buffer &&p_handshake_buffer,
        const std::shared_ptr<session_manager> &p_manager,
        const server_timeout &p_server_timeout,
        const session_config &p_session_cfg,
        typename std::enable_if<
            std::is_same<F, server_streams::plain_stream>::value> * = nullptr) :
        server_session<session_accept<StreamType>, session_manager>(
            p_stream.socket().remote_endpoint().address().to_string(),
            p_manager,
            p_server_timeout,
            p_session_cfg),
        m_handshake_buffer(std::move(p_handshake_buffer)),
        m_stream(std::move(p_stream)),
        m_timer(m_stream.get_executor(),
                (std::chrono::steady_clock::time_point::max)()),
        m_established(false),
        m_close(false),
        m_close_received(false),
        m_ping_state(ping_state::ping_next)
    {
        RADRPC_LOG("+session_accept: plain");
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * @param p_stream The stream to own.
     * @param p_handshake_buffer The buffer to own.
     * @param p_handshake_buffer The ssl context to use.
     * @param p_manager The manager shared among sessions.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The config for the session.
     */
    template <typename F = StreamType>
    explicit session_accept(
        boost::beast::tcp_stream &&p_stream,
        boost::beast::flat_buffer &&p_handshake_buffer,
        ssl::context &p_ssl_ctx,
        const std::shared_ptr<session_manager> &p_manager,
        const server_timeout &p_server_timeout,
        const session_config &p_session_cfg,
        typename std::enable_if<
            std::is_same<F, server_streams::ssl_stream>::value> * = nullptr) :
        server_session<session_accept<StreamType>, session_manager>(
            p_stream.socket().remote_endpoint().address().to_string(),
            p_manager,
            p_server_timeout,
            p_session_cfg),
        m_handshake_buffer(std::move(p_handshake_buffer)),
        m_stream(std::move(p_stream), p_ssl_ctx),
        m_timer(m_stream.get_executor(),
                (std::chrono::steady_clock::time_point::max)()),
        m_established(false),
        m_close(false),
        m_close_received(false),
        m_ping_state(ping_state::ping_next)
    {
        RADRPC_LOG("+session_accept: ssl");
    }
#endif

    ~session_accept()
    {
        if (m_established)
            base().m_manager->template remove_session<StreamType>(base().id);
        RADRPC_LOG("~session_accept: " << base().m_manager->connections());
    }

    /**
     * Start accept the connection.
     */
    void accept()
    {
        m_stream.binary(true);
        ssl_handshake();
    }
};

#ifdef RADRPC_SSL_SUPPORT
/**
 * A type to determine whether the
 * client is using plain or ssl stream
 */
class detect_session : public std::enable_shared_from_this<detect_session>
{
    boost::beast::tcp_stream m_stream;      ///< The stream to operate on.
    ssl::context &m_ssl_ctx;                ///< The used ssl context.
    const server_config &m_server_cfg;      ///< The server config.
    const server_timeout &m_server_timeout; ///< The server timeout.
    const session_config &m_session_cfg;    ///< The session config.
    std::shared_ptr<session_manager>
        &m_manager;                     ///< The manager shared among sessions.
    boost::beast::flat_buffer m_buffer; ///< The buffer to read the request.

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
                            std::shared_ptr<session_manager> &p_manager) :
        m_stream(std::move(m_socket)),
        m_ssl_ctx(p_ssl_ctx),
        m_server_cfg(p_server_cfg),
        m_server_timeout(p_server_timeout),
        m_session_cfg(p_session_cfg),
        m_manager(p_manager)
    {
    }

    /**
     * Start read the client's request
     * & check whether it is plain or ssl.
     */
    void run()
    {
        m_buffer.max_size(m_manager->server_cfg.max_handshake_bytes);
        m_stream.expires_after(m_server_timeout.handshake_or_close_timeout);
        boost::beast::async_detect_ssl(
            m_stream,
            m_buffer,
            boost::beast::bind_front_handler(&detect_session::on_detect,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     * @param result
     */
    void on_detect(boost::beast::error_code ec, boost::tribool result)
    {
        if (ec)
        {
            RADRPC_LOG("detect_session::on_detect: " << ec.message());
            return;
        }
        if (result && m_server_cfg.mode & server_mode::ssl)
        {
            std::make_shared<session_accept<server_streams::ssl_stream>>(
                std::move(m_stream),
                std::move(m_buffer),
                m_ssl_ctx,
                m_manager,
                m_server_timeout,
                m_session_cfg)
                ->accept();
        }
        else if (!result && m_server_cfg.mode & server_mode::plain)
        {
            std::make_shared<session_accept<server_streams::plain_stream>>(
                std::move(m_stream),
                std::move(m_buffer),
                m_manager,
                m_server_timeout,
                m_session_cfg)
                ->accept();
        }
    }
};
#endif

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_SERVER_SESSION_HPP