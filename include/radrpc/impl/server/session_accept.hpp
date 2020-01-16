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

#ifndef RADRPC_IMPL_SERVER_SESSION_ACCEPT_HPP
#define RADRPC_IMPL_SERVER_SESSION_ACCEPT_HPP

#include <memory>

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#ifdef RADRPC_SSL_SUPPORT
#include <boost/beast/ssl/ssl_stream.hpp>
#endif
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <radrpc/config.hpp>
#include <radrpc/debug/log.hpp>
#include <radrpc/debug/instance_track.hpp>
#include <radrpc/common/connection_state.hpp>
#include <radrpc/common/io_header.hpp>
#include <radrpc/common/streams.hpp>

namespace radrpc {
namespace common {

class server_timeout;
class session_config;

} // namespace common
} // namespace radrpc

namespace radrpc {
namespace impl {
namespace server {

using namespace radrpc::common;

class session_manager;
template <class Derived> class server_session;

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
 * Represents a type to wrap around server_session
 * to provide a accept facility.
 * @tparam StreamType The stream to use.
 */
template <class StreamType>
class session_accept
    : public server_session<session_accept<StreamType>>,
      public std::enable_shared_from_this<session_accept<StreamType>>
{
    /// The buffer to use for handshakes.
    boost::beast::flat_buffer m_handshake_buffer;

    /// The stream to use.
    StreamType m_stream;

    /// Timer for ping/pong activity.
    boost::asio::steady_timer m_timer;

    /// Used to check whether stream is ssl.
    const bool m_is_ssl;

    /// Used to check whether the connection was established.
    bool m_established;

    /// Used to check if async_close was already executed.
    bool m_closing;

    /// Used to check if async_close was successful.
    bool m_closing_success;

    /// Used to check if remote host sent a close frame.
    bool m_remote_sent_close;

    /// Used to check whether the socket is fully closed.
    bool m_socket_closed;

    /// Used with handler 'on_timer' to timeout async operations.
    connection_state m_state;

    /// The current state of the ping/pong procedure.
    ping_state m_ping_state;


    friend class server_session<session_accept>;

    server_session<session_accept<StreamType>> &base()
    {
        return static_cast<server_session<session_accept<StreamType>> &>(*this);
    }

    /**
     * Checks whether this class is using a ssl stream.
     * @return True if ssl stream is used, false if not.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, bool>::type
    is_ssl()
    {
        return false;
    }

    /**
     * Starts a ssl handshake.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, void>::type
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
    typename std::enable_if<std::is_same<F, streams::ssl>::value, bool>::type
    is_ssl()
    {
        return true;
    }

    /**
     * Starts a ssl handshake.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::ssl>::value, void>::type
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

        // Let handler inspect & configure the session and allow a customized
        // response handshake
        if (base().m_manager->on_accept &&
            !base().m_manager->on_accept(std::ref(base().info)))
        {
            RADRPC_LOG(
                "session_accept::on_read_handshake: Rejected connection");
            return;
        }
        m_stream.read_message_max(base().m_config.max_transfer_bytes +
                                  sizeof(io_header));

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
        set_timeout(connection_state::accept,
                    base().m_server_timeout.handshake_or_close_timeout);
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

        if (base().m_config.ping_delay != duration::zero())
        {
            set_timeout(connection_state::established,
                        base().m_config.ping_delay);
        }
        else
        {
            set_timeout(connection_state::established,
                        std::chrono::milliseconds(0));
        }

        add_session();
        base().read();
    }

    /**
     * @brief
     *
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, void>::type
    add_session()
    {
        base().m_manager->add_plain_session(this->shared_from_this());
    }

#ifdef RADRPC_SSL_SUPPORT

    /**
     * @brief
     *
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::ssl>::value, void>::type
    add_session()
    {
        base().m_manager->add_ssl_session(this->shared_from_this());
    }

#endif

    /**
     * @brief
     *
     * @param wait
     */
    void close_session(bool wait)
    {
        if (m_closing || m_remote_sent_close)
            return;
        m_closing = true;
        // Cancel all async operations on socket
        cancel_operations();
        // It seems it is fine to just cancel operations
        // and let the stream starve. The corresponding
        // closing functions will also be called according
        // to the debugger for both streams.
        // However it can also be initiated by 'async_close'
        // but this caused hanging sessions on plain stream unable
        // to finishing the operation of 'async_close' even with a timer
        // to cancel all operations.
        // This was discovered in recent added stress tests with abruptly
        // destroyed clients.
    }

    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, void>::type
    cancel_operations()
    {
        m_stream.next_layer().cancel();
    }

#ifdef RADRPC_SSL_SUPPORT

    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::ssl>::value, void>::type
    cancel_operations()
    {
        m_stream.next_layer().next_layer().cancel();
    }

#endif

    /**
     * @brief Set timeout for the async next operation.
     *
     * @param state
     * @param expiration
     */
    void set_timeout(connection_state state, duration expiration)
    {
        m_state = state;
        m_timer.expires_after(expiration);
        m_timer.async_wait(boost::beast::bind_front_handler(
            &session_accept::on_timer, this->shared_from_this()));
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
            switch (m_state)
            {
                case connection_state::accept:
                {
                    close_session(false);
                    break;
                }
                case connection_state::established:
                {
                    if (m_stream.is_open() &&
                        m_ping_state == ping_state::ping_next && !m_closing &&
                        !m_remote_sent_close && !base().m_read_error &&
                        !base().m_write_error)
                    {
                        m_ping_state = ping_state::ping_send;
                        m_stream.async_ping({},
                                            boost::beast::bind_front_handler(
                                                &session_accept::on_ping,
                                                this->shared_from_this()));
                        set_timeout(connection_state::established,
                                    base().m_config.ping_delay);
                    }
                    break;
                }
                default:
                    break;
            }
        }
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
            m_remote_sent_close = true;
            return;
        }
        m_ping_state = ping_state::ping_next;
        m_timer.expires_after(base().m_config.ping_delay);
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
        const std::string &&p_remote_host,
        const std::shared_ptr<session_manager> &p_manager,
        const server_timeout &p_server_timeout,
        const session_config &p_session_cfg,
        typename std::enable_if<std::is_same<F, streams::plain>::value> * =
            nullptr) :
        server_session<session_accept<StreamType>>(std::move(p_remote_host),
                                                   p_manager,
                                                   p_server_timeout,
                                                   p_session_cfg),
        m_handshake_buffer(std::move(p_handshake_buffer)),
        m_stream(std::move(p_stream)),
        m_timer(m_stream.get_executor(),
                (std::chrono::steady_clock::time_point::max)()),
        m_is_ssl(false),
        m_established(false),
        m_closing(false),
        m_closing_success(false),
        m_socket_closed(false),
        m_state(connection_state::none),
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
        const std::string &&p_remote_host,
        const std::shared_ptr<session_manager> &p_manager,
        const server_timeout &p_server_timeout,
        const session_config &p_session_cfg,
        typename std::enable_if<std::is_same<F, streams::ssl>::value> * =
            nullptr) :
        server_session<session_accept<StreamType>>(std::move(p_remote_host),
                                                   p_manager,
                                                   p_server_timeout,
                                                   p_session_cfg),
        m_handshake_buffer(std::move(p_handshake_buffer)),
        m_stream(std::move(p_stream), p_ssl_ctx),
        m_timer(m_stream.get_executor(),
                (std::chrono::steady_clock::time_point::max)()),
        m_is_ssl(true),
        m_established(false),
        m_closing(false),
        m_closing_success(false),
        m_socket_closed(false),
        m_state(connection_state::none),
        m_ping_state(ping_state::ping_next)
    {
        RADRPC_LOG("+session_accept: ssl");
    }
#endif

    ~session_accept()
    {
        if (m_established)
        {
#ifdef RADRPC_SSL_SUPPORT
            if (m_is_ssl)
            {
                base().m_manager->remove_ssl_session(base().id);
                RADRPC_LOG(
                    "~session_accept: ssl   " << base().m_manager->connections());
            }
            else
#endif
            {
                base().m_manager->remove_plain_session(base().id);
                RADRPC_LOG(
                    "~session_accept: plain " << base().m_manager->connections());
            }
        }
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

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_SESSION_ACCEPT_HPP