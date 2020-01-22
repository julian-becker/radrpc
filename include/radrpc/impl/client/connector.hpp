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

#ifndef RADRPC_IMPL_CLIENT_CONNECTOR_HPP
#define RADRPC_IMPL_CLIENT_CONNECTOR_HPP

#include <radrpc/debug/instance_track.hpp>
#include <radrpc/impl/client/session.hpp>

namespace radrpc {
namespace impl {
namespace client {

/**
 * Represents a type to wrap around session
 * to provide a connect / disconnect facility.
 * @tparam StreamType The stream to use.
 * @see radrpc::client_mode
 * @see radrpc::impl::client::session
 */
template <class StreamType>
class connector : public session<connector<StreamType>>,
                  public std::enable_shared_from_this<connector<StreamType>>
{
    /// The referenced io context.
    boost::asio::io_context *m_io_ctx;

    /// The referenced thread to execute blocking context.
    std::thread *m_thread;

    /// The referenced timeout.
    websocket::stream_base::timeout *m_timeout;

    /// The referenced request handshake.
    handshake_request *m_req_handshake;

    /// Bound handlers to call on broadcast by id.
    /// sizeof config::max_call_id
    std::function<void(receive_buffer &)> *m_bound_funcs;

    /// No Strand ("io_context" single threaded)
    tcp::resolver m_resolver;

    /// No Strand ("io_context" single threaded)
    StreamType m_stream;

    /// A state about the connection.
    connection_state m_state;

    /// Used to wait on establishing the connection.
    std::promise<bool> m_run_callback;

    /// Used to check if close initiated external.
    bool m_close_initiated;

    /// Used to check if closed.
    bool m_closed;

    friend class session<connector<StreamType>>;

    session<connector<StreamType>> &base()
    {
        return static_cast<session<connector<StreamType>> &>(*this);
    }

    /**
     * @param ec
     * @param results
     */
    void on_resolve(boost::system::error_code ec,
                    const tcp::resolver::results_type &results)
    {
        if (ec)
        {
            RADRPC_LOG("client::connector::on_resolve: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        m_state = connection_state::connect;
        boost::beast::get_lowest_layer(m_stream).expires_after(
            m_timeout->handshake_timeout);
        RADRPC_LOG("client::connector::on_resolve: Connect...");
        boost::beast::get_lowest_layer(m_stream).async_connect(
            results,
            boost::beast::bind_front_handler(&connector::on_connect,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_connect(boost::beast::error_code ec,
                    const tcp::resolver::results_type::endpoint_type &)
    {
        if (ec)
        {
            RADRPC_LOG("client::connector::on_connect: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        boost::beast::get_lowest_layer(m_stream).expires_after(
            base().m_client_timeout->handshake_timeout);
        ssl_handshake();
    }

    /**
     * Skip SSL handshake on 'plain_stream'
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, void>::type
    ssl_handshake()
    {
        RADRPC_LOG("client::connector::ssl_handshake: No ssl");
        boost::system::error_code ec;
        on_ssl_handshake(ec);
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * Make a SSL handshake with the server.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::ssl>::value, void>::type
    ssl_handshake()
    {
        RADRPC_LOG("client::connector::ssl_handshake: SSL handshake...");
        m_stream.next_layer().async_handshake(
            ssl::stream_base::client,
            boost::beast::bind_front_handler(&connector::on_ssl_handshake,
                                             this->shared_from_this()));
    }
#endif

    /**
     * @param ec
     */
    void on_ssl_handshake(boost::system::error_code ec)
    {
        if (ec)
        {
            RADRPC_LOG("client::connector::on_ssl_handshake: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        boost::beast::get_lowest_layer(m_stream).expires_never();
        m_state = connection_state::handshake;
        m_stream.set_option(*m_timeout);

        // Assign the fields of the request handshake to the normal handshake
        m_stream.set_option(websocket::stream_base::decorator(
            [set_req{*m_req_handshake}](handshake_request &req) {
                for (const auto &field : set_req)
                {
                    if (field.name() == boost::beast::http::field::unknown)
                        req.insert(field.name_string(), field.value());
                    else
                        req.set(field.name(), field.value());
                }
            }));

        RADRPC_LOG("client::connector::on_ssl_handshake: Handshake...");
        m_stream.async_handshake(
            res_handshake,
            base().m_client_cfg->host_address,
            "/",
            boost::beast::bind_front_handler(&connector::on_handshake,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_handshake(boost::system::error_code ec)
    {
        if (ec)
        {
            RADRPC_LOG("client::connector::on_handshake: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        base().read();
        m_state = connection_state::established;
        m_run_callback.set_value(true);
        RADRPC_LOG("client::connector::on_handshake: Connection established");
    }

    /**
     * Closes the session by 'async_close()'.
     * This works by sending a close frame, waiting
     * for a timeout (option was set) & close itself.
     */
    void close_session()
    {
        if (base().m_close || base().m_close_received)
            return;
        base().m_close = true;
        do_close();
    }

    /**
     * Closes the session.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::plain>::value, void>::type
    do_close()
    {
        // Timeout doesn't seem to work correctly with plain streams
        // and results in stucked session.
        // Maybe there's something internal that doesn't
        // cancel the operation via a timer or doing an invalid operation
        // here despite the carefully handling of the close procedure
        // (no other async operations, just read).
        // Todo: This may needs further investigation
        boost::beast::error_code ec;
        // Cancel pending async operations
        m_stream.next_layer().cancel();
        // Disable sending / receiving
        m_stream.next_layer().socket().shutdown(
            boost::asio::ip::tcp::socket::shutdown_both, ec);
        // Close socket
        m_stream.next_layer().socket().close(ec);
        m_closed = true;
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * Closes the session by 'async_close()'.
     * This works by sending a close frame, waiting
     * for a timeout (option was set) & close itself.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, streams::ssl>::value, void>::type
    do_close()
    {
        // Timeout is handled by 'm_timeout'
        m_stream.async_close(
            websocket::close_code::normal,
            boost::beast::bind_front_handler(&connector::on_close,
                                             this->shared_from_this()));
    }
#endif

    /**
     * @param ec
     */
    void on_close(boost::beast::error_code ec)
    {
        if (ec)
            RADRPC_LOG("client::connector::on_close: " << ec.message());
        m_closed = true;
    }

  public:
    /// The response handshake from the server.
    handshake_response res_handshake;

    /**
     * @param p_io_ctx The io_context to use.
     * @param p_thread The thread to run on the io_context.
     * @param p_client_cfg The client configuration.
     * @param p_client_timeout The client timeout to set the actuall timeout.
     * @param p_timeout The timeout to use on stream.
     * @param p_req_handshake The request handhshake to send to the server
     * @param p_bound_funcs The bound handlers for receive broadcasts.
     */
    template <typename F = StreamType>
    explicit connector(
        boost::asio::io_context &p_io_ctx,
        std::thread &p_thread,
        client_config &p_client_cfg,
        client_timeout &p_client_timeout,
        websocket::stream_base::timeout &p_timeout,
        handshake_request &p_req_handshake,
        std::function<void(receive_buffer &)>
            p_bound_funcs[config::max_call_id],
        typename std::enable_if<std::is_same<F, streams::plain>::value> * =
            nullptr) :
        session<connector>(p_client_cfg, p_client_timeout),
        m_io_ctx(&p_io_ctx),
        m_thread(&p_thread),
        m_timeout(&p_timeout),
        m_req_handshake(&p_req_handshake),
        m_bound_funcs(p_bound_funcs),
        m_resolver(p_io_ctx),
        m_stream(p_io_ctx),
        m_state(connection_state::none),
        m_close_initiated(false),
        m_closed(false)
    {
        RADRPC_LOG("+client::connector: plain");
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * @param p_io_ctx The io_context to use.
     * @param p_ssl_ctx The initialized ssl context to use.
     * @param p_thread The thread to run on the io_context.
     * @param p_client_cfg The client configuration.
     * @param p_client_timeout The client timeout to set the actuall timeout.
     * @param p_timeout The timeout to use on stream.
     * @param p_req_handshake The request handhshake to send to the server
     * @param p_bound_funcs The bound handlers for receive broadcasts.
     */
    template <typename F = StreamType>
    explicit connector(
        boost::asio::io_context &p_io_ctx,
        ssl::context &p_ssl_ctx,
        std::thread &p_thread,
        client_config &p_client_cfg,
        client_timeout &p_client_timeout,
        websocket::stream_base::timeout &p_timeout,
        handshake_request &p_req_handshake,
        std::function<void(receive_buffer &)>
            p_bound_funcs[config::max_call_id],
        typename std::enable_if<std::is_same<F, streams::ssl>::value> * =
            nullptr) :
        session<connector>(p_client_cfg, p_client_timeout),
        m_io_ctx(&p_io_ctx),
        m_thread(&p_thread),
        m_timeout(&p_timeout),
        m_req_handshake(&p_req_handshake),
        m_bound_funcs(p_bound_funcs),
        m_resolver(p_io_ctx),
        m_stream(p_io_ctx, p_ssl_ctx),
        m_state(connection_state::none),
        m_close_initiated(false),
        m_closed(false)
    {
        RADRPC_LOG("+client::connector: ssl");
    }
#endif

    ~connector() { RADRPC_LOG("~client::connector"); }

    /**
     * Connects to the server.
     * @return The connection state.
     */
    connection_state run()
    {
        if (m_close_initiated || m_state != connection_state::none)
            return connection_state::none;

        m_stream.control_callback(
            std::bind(&session<connector<StreamType>>::on_control_callback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));
        m_stream.binary(true);
        m_stream.read_message_max(base().m_client_cfg->max_read_bytes);

        m_run_callback = std::promise<bool>();
        auto res = m_run_callback.get_future();

        m_resolver.async_resolve(
            base().m_client_cfg->host_address,
            std::to_string(base().m_client_cfg->port),
            boost::beast::bind_front_handler(&connector::on_resolve,
                                             this->shared_from_this()));

        *m_thread = std::thread([&] {
            if (m_io_ctx->stopped())
                m_io_ctx->restart();
            try
            {
                boost::system::error_code ec;
                m_io_ctx->run(ec);
                if (ec)
                    RADRPC_LOG("client::connector::run: " << ec);
            }
            catch (std::exception &ex)
            {
                RADRPC_LOG("client::connector::run: " << ec
                                                      << "\nEX: " << ex.what());
                RADRPC_DEBUG_LOOP();
            }
            catch (...)
            {
                RADRPC_DEBUG_LOOP();
            }
        });

        // This could get stuck if "websocket::stream_base::timeout" is set to
        // default
        res.wait();
        return m_state;
    }

    /**
     * Disconnects from the server.
     */
    void close()
    {
        if (m_close_initiated)
            return;
        m_close_initiated = true;
        if (m_io_ctx->stopped())
        {
            core::weak_post<connector>(
                this->shared_from_this(),
                m_stream.get_executor(),
                std::bind(&connector::close_session, this));
            RADRPC_LOG("client::connector::close: Run IO to close session");
            m_io_ctx->restart();
            m_io_ctx->run();
            RADRPC_LOG("session::close: IO done");
        }
        else
        {
            RADRPC_LOG("client::connector::close");
            if (!core::wait_weak_post<connector>(
                    this->shared_from_this(),
                    m_stream.get_executor(),
                    std::bind(&connector::close_session, this),
                    std::chrono::seconds(config::io_timeout_secs)))
            {
                RADRPC_LOG(
                    "client::connector::close: Timeout, IO probably stopped");
            }
        }
    }
};

} // namespace client
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_CLIENT_CONNECTOR_HPP