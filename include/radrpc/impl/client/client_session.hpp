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

#ifndef RADRPC_IMPL_CLIENT_CLIENT_SESSION_HPP
#define RADRPC_IMPL_CLIENT_CLIENT_SESSION_HPP

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <unordered_map>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <radrpc/debug.hpp>
#include <radrpc/types.hpp>
#include <radrpc/detail/data.hpp>
#include <radrpc/detail/operations.hpp>

namespace radrpc {
namespace impl {
namespace client {

template <typename T> class session_connect;

/**
 * Represents a client session.
 */
template <class Derived> class client_session
{
    boost::beast::flat_buffer
        m_read_buffer;           ///< The buffer to receive bytes from server.
    client_config *m_client_cfg; ///< The referenced client configuration.
    client_timeout *m_client_timeout; ///< The referenced client timeout.
    detail::data_cache m_cache;       ///< A cache to hold the server messages.
    detail::data_queue m_queue;       ///< A queue to hold the data to send.
    std::mutex m_ping_mtx;            ///< Lock for ping.
    bool m_ping; ///< Check whether a ping is still processed.
    bool m_pong; ///< Check whether the pong was received.
    std::condition_variable
        m_ping_cv;         ///< Used to notify the waiting ping function.
    bool m_read_error;     ///< Info for read error.
    bool m_write_error;    ///< Info for write error.
    bool m_close;          ///< Used to check if close was already executed.
    bool m_close_received; ///< Used to check if a close was received.

    template <typename T> friend class session_connect;

    Derived &derived() { return static_cast<Derived &>(*this); }

    /**
     * Send a ping to server.
     * This function must be called within the executor context.
     */
    void handle_ping()
    {
        if (m_ping)
            return;
        m_ping = true;
        derived().m_stream.async_ping({},
                                      std::bind(&client_session::on_ping,
                                                derived().shared_from_this(),
                                                std::placeholders::_1));
    }

    /**
     *
     */
    void on_ping(boost::beast::error_code ec)
    {
        m_ping = false;
        if (ec)
        {
            RADRPC_LOG("client_session::on_ping: " << ec.message());
            return;
        }
    }

    /**
     * Send data to the server
     * @param call_id The id to call serverside.
     * @param result_id The id to let the server assign the response.
     * @param callback The callback to wait til the message was written.
     * @param send_bytes The bytes to send.
     * This function must be called within the executor context.
     */
    void handle_send(uint32_t call_id,
                     uint64_t result_id,
                     std::weak_ptr<std::promise<bool>> &callback,
                     const char *data_ptr,
                     std::size_t data_size)
    {
        if (m_close || m_close_received || m_write_error || m_read_error)
            return;
        if (!m_queue.queue(call_id, result_id, callback, data_ptr, data_size))
            return;
        if (m_queue.is_writing())
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
                boost::asio::buffer(m_queue.front()->body_ptr,
                                    m_queue.front()->body_size)},
            boost::beast::bind_front_handler(&client_session::on_write,
                                             derived().shared_from_this()));
        // IF we are going to operate on the queue for example with 'clear()',
        // it is needed to also pass the reference of the data to the handler,
        // because it is only referenced in the queue.
        // Clearing it while async_write is processing means a 'heap use after
        // free' error. The alternative handler binding could look like this:
        // boost::beast::bind_front_handler(&client_session::on_write,
        //                                  derived().shared_from_this(),
        //                                  m_queue.front()->shared_from_this())
        // The handler needs also to be changed.
        // But since removing entries happens only in 'on_write()' it is fine
        // now.
    }

    /**
     *
     */
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if (ec || m_close || m_close_received || m_read_error)
        {
            RADRPC_LOG("client_session::on_write: " << ec.message());
            m_queue.clear();
            m_write_error = true;
            derived().close_session();
            return;
        }
        RADRPC_LOG("client_session::on_write: " << bytes_transferred
                                                << "bytes written");
        // Continue writing til it is empty
        if (m_queue.write_next())
        {
            write();
        }
    }

    /**
     * Listen to all writes from the server.
     * This function must be called within the executor context.
     */
    void read()
    {
        derived().m_stream.async_read(m_read_buffer,
                                      std::bind(&client_session::on_read,
                                                derived().shared_from_this(),
                                                std::placeholders::_1,
                                                std::placeholders::_2));
    }

    /**
     * Check the received bytes & notify
     * or call the broadcast handler.
     */
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if (ec || m_close_received)
        {
            RADRPC_LOG("client_session::on_read: " << ec.message());
            m_cache.clear();
            m_read_error = true;
            derived().close_session();
            return;
        }

        auto buffer_front = boost::beast::buffers_front(m_read_buffer.data());
        if (buffer_front.size() >= sizeof(detail::io_header))
        {
            auto header = reinterpret_cast<const detail::io_header *>(
                buffer_front.data());
            auto func_itr = derived().m_bound_funcs->find(header->call_id);
            m_read_buffer.consume(sizeof(detail::io_header));
            if (func_itr != derived().m_bound_funcs->end())
            {
                // Call broadcast handler
                if (func_itr->second)
                    func_itr->second(
                        *(reinterpret_cast<receive_buffer *>(&m_read_buffer)));
            }
            else
            {
                m_cache.swap_notify(header->result_id, m_read_buffer);
            }
        }
        else
        {
            RADRPC_LOG("client_session::on_read: Invalid buffer");
        }

        m_cache.remove_obsolete();
        m_read_buffer.consume(m_read_buffer.size());
        read();
    }

    /**
     * Used to listen to pongs & received close messages.
     */
    void on_control_callback(websocket::frame_type kind,
                             boost::beast::string_view payload)
    {
        boost::ignore_unused(payload);
        if (kind == websocket::frame_type::pong)
        {
            RADRPC_LOG("client_session::on_control_callback: Pong received");
            {
                std::unique_lock<std::mutex> lock(m_ping_mtx);
                m_pong = true;
            }
            m_ping_cv.notify_all();
        }
        // Close will be handled by the websocket itself
        else if (kind == websocket::frame_type::close)
        {
            RADRPC_LOG("client_session::on_control_callback: Close received");
            m_close_received = true;
        }
    }

  public:
    /**
     * @param p_client_cfg
     * @param p_client_timeout
     */
    explicit client_session(client_config &p_client_cfg,
                            client_timeout &p_client_timeout) :
        m_client_cfg(&p_client_cfg),
        m_client_timeout(&p_client_timeout),
        m_cache(config::queue_recv_max),
        m_queue(config::queue_send_max),
        m_ping(false),
        m_pong(false),
        m_read_error(false),
        m_write_error(false),
        m_close(false),
        m_close_received(false)
    {
        RADRPC_LOG("+client_session");
    }

    ~client_session() { RADRPC_LOG("~client_session"); }

    /**
     * Ping the server & wait for a pong.
     * @return True if pong received, false if not.
     */
    bool ping()
    {
        RADRPC_LOG("client_session::ping");
        detail::weak_post<Derived>(
            derived().shared_from_this(),
            derived().m_stream.get_executor(),
            std::bind(&client_session::handle_ping, this));


        auto response_timeout = detail::response_timeout();
        if (response_timeout == duration::zero())
            response_timeout = m_client_timeout->response_timeout;
        std::unique_lock<std::mutex> lock(m_ping_mtx);
        m_pong = false;
        bool pong =
            m_ping_cv.wait_for(lock, response_timeout, [&] { return m_pong; });
        detail::response_timeout(duration::zero());
        if (!pong)
            RADRPC_LOG("client_session::ping: Timeout");
        return pong;
    }

    /**
     * Send bytes to the server with a call id.
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @param ec The referenced error code on failure.
     */
    void send(uint32_t call_id,
              const char *data_ptr,
              std::size_t data_size,
              boost::system::error_code &ec)
    {
        auto write_callback = std::make_shared<std::promise<bool>>();
        std::weak_ptr<std::promise<bool>> weak_write_callback = write_callback;
        auto written = write_callback->get_future();


        RADRPC_LOG("client_session::send: Write "
                   << data_size + sizeof(detail::io_header) << "bytes");
        detail::weak_post<Derived>(derived().shared_from_this(),
                                   derived().m_stream.get_executor(),
                                   std::bind(&client_session::handle_send,
                                             this,
                                             call_id,
                                             0,
                                             weak_write_callback,
                                             data_ptr,
                                             data_size));


        // This ensures the referenced data itself will not be used anymore if
        // it was written. However if the message was processed with
        // 'async_write' & is not completed it still references the used
        // external bytes, so set 'timed_out' and let 'client' disconnect & wait
        // for session destruction. This means if the session is destructed the
        // handler passed to 'async_write' is also completed, since it was bound
        // with 'shared_from_this'
        auto send_timeout = detail::send_timeout();
        if (send_timeout == duration::zero())
            send_timeout = m_client_timeout->send_timeout;
        std::future_status status = written.wait_for(send_timeout);
        detail::send_timeout(duration::zero());
        if (status != std::future_status::ready)
        {
            RADRPC_LOG("client_session::send: Timeout on send");
            ec = boost::asio::error::timed_out;
        }
    }

    /**
     * Send bytes to the server with a call id & wait for a response.
     * @param call_id The id to call on serverside.
     * @param send_bytes The bytes to send.
     * @param recv_bytes The received bytes from the server.
     * @param ec The referenced error code on failure.
     */
    void send_recv(uint32_t call_id,
                   const char *data_ptr,
                   std::size_t data_size,
                   receive_buffer &recv_bytes,
                   boost::system::error_code &ec)
    {
        auto result_id = m_cache.queue(m_client_timeout->response_timeout * 2);
        if (result_id == 0)
        {
            ec = boost::asio::error::operation_aborted;
            return;
        }
        auto write_callback = std::make_shared<std::promise<bool>>();
        std::weak_ptr<std::promise<bool>> weak_write_callback = write_callback;
        auto written = write_callback->get_future();


        RADRPC_LOG("client_session::send_recv: Write "
                   << data_size + sizeof(detail::io_header)
                   << "bytes [RID:" << result_id << "]");
        detail::weak_post<Derived>(derived().shared_from_this(),
                                   derived().m_stream.get_executor(),
                                   std::bind(&client_session::handle_send,
                                             this,
                                             call_id,
                                             result_id,
                                             weak_write_callback,
                                             data_ptr,
                                             data_size));


        // This ensures the referenced data itself will not be used anymore if
        // it was written. However if the message was processed with
        // 'async_write' & is not completed it still references the used
        // external bytes, so set 'timed_out' and let 'client' disconnect & wait
        // for session destruction. This means if the session is destructed the
        // handler passed to 'async_write' is also completed, since it was used
        // with 'shared_from_this'
        auto send_timeout = detail::send_timeout();
        if (send_timeout == duration::zero())
            send_timeout = m_client_timeout->send_timeout;
        std::future_status status = written.wait_for(send_timeout);
        detail::send_timeout(duration::zero());
        if (status != std::future_status::ready)
        {
            RADRPC_LOG("client_session::send_recv: Timeout on send");
            ec = boost::asio::error::timed_out;
            return;
        }


        // Wait for response
        auto response_timeout = detail::response_timeout();
        if (response_timeout == duration::zero())
            response_timeout = m_client_timeout->response_timeout;
        if (!m_cache.wait(result_id, response_timeout, recv_bytes))
        {
            RADRPC_LOG("client_session::send_recv: Timeout on receive");
            ec = boost::asio::error::timed_out;
        }
        detail::response_timeout(duration::zero());
    }
};

/**
 * Represents a type to wrap around client_session
 * to provide a connect / disconnect facility.
 * @tparam StreamType The stream to use.
 * @see radrpc::client_mode
 * @see radrpc::impl::client::client_session
 */
template <class StreamType>
class session_connect
    : public client_session<session_connect<StreamType>>,
      public std::enable_shared_from_this<session_connect<StreamType>>
{
    boost::asio::io_context *m_io_ctx; ///< The referenced io context.
    std::thread
        *m_thread; ///< The referenced thread to execute blocking context.
    websocket::stream_base::timeout *m_timeout; ///< The referenced timeout.
    handshake_request *m_req_handshake; ///< The referenced request handshake.
    std::unordered_map<uint32_t,
                       std::function<void(receive_buffer &)>>
        *m_bound_funcs;       ///< Bound handlers to call on broadcast by id.
    tcp::resolver m_resolver; ///< No Strand ("io_context" single threaded)
    StreamType m_stream;      ///< No Strand ("io_context" single threaded)
    connection_state m_state; ///< A state about the connection.
    std::promise<bool>
        m_run_callback;     ///< Used to wait on establishing the connection.
    bool m_close_initiated; ///< Used to check if close initiated external.

    friend class client_session<session_connect<StreamType>>;

    client_session<session_connect<StreamType>> &base()
    {
        return static_cast<client_session<session_connect<StreamType>> &>(
            *this);
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
            RADRPC_LOG("session_connect::on_resolve: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        m_state = connection_state::connect;
        boost::beast::get_lowest_layer(m_stream).expires_after(
            m_timeout->handshake_timeout);
        RADRPC_LOG("session_connect::on_resolve: Connect...");
        boost::beast::get_lowest_layer(m_stream).async_connect(
            results,
            boost::beast::bind_front_handler(&session_connect::on_connect,
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
            RADRPC_LOG("session_connect::on_connect: " << ec.message());
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
    typename std::enable_if<
        std::is_same<F, client_streams::plain_stream>::value,
        void>::type
    ssl_handshake()
    {
        RADRPC_LOG("client_connect::ssl_handshake: No ssl");
        boost::system::error_code ec;
        on_ssl_handshake(ec);
    }

#ifdef RADRPC_SSL_SUPPORT
    /**
     * Make a SSL handshake with the server.
     */
    template <typename F = StreamType>
    typename std::enable_if<std::is_same<F, client_streams::ssl_stream>::value,
                            void>::type
    ssl_handshake()
    {
        RADRPC_LOG("client_connect::ssl_handshake: SSL handshake...");
        m_stream.next_layer().async_handshake(
            ssl::stream_base::client,
            boost::beast::bind_front_handler(&session_connect::on_ssl_handshake,
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
            RADRPC_LOG("session_connect::on_ssl_handshake: " << ec.message());
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

        RADRPC_LOG("session_connect::on_ssl_handshake: Handshake...");
        m_stream.async_handshake(
            res_handshake,
            base().m_client_cfg->host_address,
            "/",
            boost::beast::bind_front_handler(&session_connect::on_handshake,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_handshake(boost::system::error_code ec)
    {
        if (ec)
        {
            RADRPC_LOG("session_connect::on_handshake: " << ec.message());
            m_run_callback.set_value(false);
            return;
        }
        base().read();
        m_state = connection_state::established;
        m_run_callback.set_value(true);
        RADRPC_LOG("session_connect::on_handshake: Connection established");
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
        // Timeout is handled by 'm_timeout'
        m_stream.async_close(
            websocket::close_code::normal,
            boost::beast::bind_front_handler(&session_connect::on_close,
                                             this->shared_from_this()));
    }

    /**
     * @param ec
     */
    void on_close(boost::beast::error_code ec)
    {
        if (ec)
            RADRPC_LOG("session_connect::on_close: " << ec.message());
    }

  public:
    handshake_response
        res_handshake; ///< The response handshake from the server.

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
    explicit session_connect(
        boost::asio::io_context &p_io_ctx,
        std::thread &p_thread,
        client_config &p_client_cfg,
        client_timeout &p_client_timeout,
        websocket::stream_base::timeout &p_timeout,
        handshake_request &p_req_handshake,
        std::unordered_map<uint32_t, std::function<void(receive_buffer &)>>
            &p_bound_funcs,
        typename std::enable_if<
            std::is_same<F, client_streams::plain_stream>::value> * = nullptr) :
        client_session<session_connect>(p_client_cfg, p_client_timeout),
        m_io_ctx(&p_io_ctx),
        m_thread(&p_thread),
        m_timeout(&p_timeout),
        m_req_handshake(&p_req_handshake),
        m_bound_funcs(&p_bound_funcs),
        m_resolver(p_io_ctx),
        m_stream(p_io_ctx),
        m_state(connection_state::none),
        m_close_initiated(false)
    {
        RADRPC_LOG("+session_connect: plain");
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
    explicit session_connect(
        boost::asio::io_context &p_io_ctx,
        ssl::context &p_ssl_ctx,
        std::thread &p_thread,
        client_config &p_client_cfg,
        client_timeout &p_client_timeout,
        websocket::stream_base::timeout &p_timeout,
        handshake_request &p_req_handshake,
        std::unordered_map<uint32_t, std::function<void(receive_buffer &)>>
            &p_bound_funcs,
        typename std::enable_if<
            std::is_same<F, client_streams::ssl_stream>::value> * = nullptr) :
        client_session<session_connect>(p_client_cfg, p_client_timeout),
        m_io_ctx(&p_io_ctx),
        m_thread(&p_thread),
        m_timeout(&p_timeout),
        m_req_handshake(&p_req_handshake),
        m_bound_funcs(&p_bound_funcs),
        m_resolver(p_io_ctx),
        m_stream(p_io_ctx, p_ssl_ctx),
        m_state(connection_state::none),
        m_close_initiated(false)
    {
        RADRPC_LOG("+session_connect: ssl");
    }
#endif

    ~session_connect() { RADRPC_LOG("~session_connect"); }

    /**
     * Connects to the server.
     * @return The connection state.
     */
    connection_state run()
    {
        if (m_close_initiated || m_state != connection_state::none)
            return connection_state::none;

        m_stream.control_callback(std::bind(
            &client_session<session_connect<StreamType>>::on_control_callback,
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
            boost::beast::bind_front_handler(&session_connect::on_resolve,
                                             this->shared_from_this()));

        *m_thread = std::thread([&] {
            if (m_io_ctx->stopped())
                m_io_ctx->restart();
            m_io_ctx->run();
            RADRPC_LOG("session_connect::run: IO done");
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
            detail::weak_post<session_connect>(
                this->shared_from_this(),
                m_stream.get_executor(),
                std::bind(&session_connect::close_session, this));
            RADRPC_LOG("client_session::close: Run IO to close session");
            m_io_ctx->restart();
            m_io_ctx->run();
            RADRPC_LOG("client_session::close: IO done");
        }
        else
        {
            RADRPC_LOG("client_session::close");
            if (!detail::wait_weak_post<session_connect>(
                    this->shared_from_this(),
                    m_stream.get_executor(),
                    std::bind(&session_connect::close_session, this),
                    std::chrono::seconds(config::io_timeout_secs)))
            {
                RADRPC_LOG(
                    "client_session::close: Timeout, IO probably stopped");
            }
        }
    }
};

} // namespace client
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_CLIENT_CLIENT_SESSION_HPP