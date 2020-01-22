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

#ifndef RADRPC_IMPL_CLIENT_SESSION_HPP
#define RADRPC_IMPL_CLIENT_SESSION_HPP

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

#include <boost/beast/core/tcp_stream.hpp>
#ifdef RADRPC_SSL_SUPPORT
#include <boost/beast/ssl/ssl_stream.hpp>
#endif
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <radrpc/debug/log.hpp>
#include <radrpc/common/client_config.hpp>
#include <radrpc/common/client_timeout.hpp>
#include <radrpc/common/connection_state.hpp>
#include <radrpc/common/receive_buffer.hpp>
#include <radrpc/common/streams.hpp>
#include <radrpc/core/endian.hpp>
#include <radrpc/core/timeout.hpp>
#include <radrpc/core/weak_post.hpp>
#include <radrpc/core/data/cache.hpp>
#include <radrpc/core/data/queue.hpp>

namespace radrpc {
namespace impl {
namespace client {

using namespace radrpc::common;
using namespace radrpc::core;
using namespace radrpc::core::endian;

template <typename T> class connector;

/**
 * Represents a client session.
 */
template <class Derived> class session
{
    /// The buffer to receive bytes from server.
    boost::beast::flat_buffer m_read_buffer;

    /// The referenced client configuration.
    client_config *m_client_cfg;

    /// The referenced client timeout.
    client_timeout *m_client_timeout;

    /// A cache to hold the server messages.
    data::cache m_cache;

    /// A queue to hold the data to send.
    data::queue m_queue;

    /// Lock for ping.
    std::mutex m_ping_mtx;

    /// Check whether a ping is still processed.
    bool m_ping;

    /// Check whether the pong was received.
    bool m_pong;

    /// Used to notify the waiting ping function.
    std::condition_variable m_ping_cv;

    /// Info for read error.
    bool m_read_error;

    /// Info for write error.
    bool m_write_error;

    /// Used to check if close was already executed.
    bool m_close;

    /// Used to check if a close was received.
    bool m_close_received;

    template <typename T> friend class connector;

    Derived &derived() { return static_cast<Derived &>(*this); }

    /**
     * Send a ping to server.
     * This function must be called within the executor context.
     */
    void handle_ping()
    {
        if (m_ping || m_close || m_close_received || m_write_error ||
            m_read_error)
            return;
        m_ping = true;
        derived().m_stream.async_ping({},
                                      std::bind(&session::on_ping,
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
            RADRPC_LOG("client::session::on_ping: " << ec.message());
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
        // Convert to network byte order (big endian)
        if (!m_queue.queue_data(
                htonl(call_id), htonl_uint64(result_id), callback, data_ptr, data_size))
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
        auto packet = m_queue.front();
        derived().m_stream.async_write(
            std::vector<boost::asio::const_buffer>{
                boost::asio::buffer(reinterpret_cast<const char *>(&packet->header),
                                    sizeof(io_header)),
                boost::asio::buffer(packet->body_ptr, packet->body_size)},
            boost::beast::bind_front_handler(&session::on_write,
                                             derived().shared_from_this()));
        // If we are going to operate on the queue for example with 'clear()',
        // it is needed to also pass the reference of the data to the handler,
        // because it is only referenced in the queue.
        // Clearing it while async_write is processing means a 'heap use after
        // free' error. The alternative handler binding could look like this:
        // boost::beast::bind_front_handler(&session::on_write,
        //                                  derived().shared_from_this(),
        //                                  packet->shared_from_this())
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
            RADRPC_LOG("client::session::on_write: " << ec.message());
            m_queue.clear();
            m_write_error = true;
            derived().close_session();
            return;
        }
        RADRPC_LOG("client::session::on_write: " << bytes_transferred
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
                                      std::bind(&session::on_read,
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
            RADRPC_LOG("client::session::on_read: " << ec.message());
            m_cache.clear();
            m_read_error = true;
            derived().close_session();
            return;
        }

        auto buffer_front = boost::beast::buffers_front(m_read_buffer.data());
        if (buffer_front.size() >= sizeof(io_header))
        {
            // Copy header & clear its portion
            auto header = *reinterpret_cast<io_header *>(buffer_front.data());
            m_read_buffer.consume(sizeof(io_header));

            // Convert to host byte order
            header.call_id = ntohl(header.call_id);
            header.result_id = ntohl_uint64(header.result_id);

            // Swap if not broadcast packet
            if (header.result_id != 0)
            {
                m_cache.swap_notify(header.result_id, m_read_buffer);
            }
            // Check bounds before calling broadcast
            else if (header.call_id >= config::max_call_id)
            {
                RADRPC_LOG("client::session::on_read: Call id "
                           << header.call_id << " is out of bounds");
            }
            // Check if function was bound
            else if (derived().m_bound_funcs[header.call_id] == nullptr)
            {
                RADRPC_LOG("client::session::on_read: Call id "
                           << header.call_id
                           << " was not bound to any function");
            }
            else
            {
                // Call broadcast handler
                derived().m_bound_funcs[header.call_id](
                    *(reinterpret_cast<receive_buffer *>(&m_read_buffer)));
            }
        }
        else
        {
            RADRPC_LOG("client::session::on_read: Invalid buffer");
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
            RADRPC_LOG("client::session::on_control_callback: Pong received");
            {
                std::unique_lock<std::mutex> lock(m_ping_mtx);
                m_pong = true;
            }
            m_ping_cv.notify_all();
        }
        // Close will be handled by the websocket itself
        else if (kind == websocket::frame_type::close)
        {
            RADRPC_LOG("client::session::on_control_callback: Close received");
            m_close_received = true;
        }
    }

  public:
    /**
     * @param p_client_cfg
     * @param p_client_timeout
     */
    explicit session(client_config &p_client_cfg,
                     client_timeout &p_client_timeout) :
        m_client_cfg(&p_client_cfg),
        m_client_timeout(&p_client_timeout),
        m_cache(config::queue_recv_max_entries),
        m_queue(config::queue_send_max_entries),
        m_ping(false),
        m_pong(false),
        m_read_error(false),
        m_write_error(false),
        m_close(false),
        m_close_received(false)
    {
        RADRPC_LOG("+client::session");
    }

    ~session() { RADRPC_LOG("~client::session"); }

    /**
     * Ping the server & wait for a pong.
     * @return True if pong received, false if not.
     */
    bool ping()
    {
        RADRPC_LOG("client::session::ping");
        core::weak_post<Derived>(derived().shared_from_this(),
                                 derived().m_stream.get_executor(),
                                 std::bind(&session::handle_ping, this));


        auto response_timeout = core::response_timeout();
        if (response_timeout == duration::zero())
            response_timeout = m_client_timeout->response_timeout;
        std::unique_lock<std::mutex> lock(m_ping_mtx);
        m_pong = false;
        bool pong =
            m_ping_cv.wait_for(lock, response_timeout, [&] { return m_pong; });
        core::response_timeout(duration::zero());
        if (!pong)
            RADRPC_LOG("client::session::ping: Timeout");
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


        RADRPC_LOG("client::session::send: Write " << data_size + sizeof(io_header)
                                           << "bytes");
        core::weak_post<Derived>(derived().shared_from_this(),
                                 derived().m_stream.get_executor(),
                                 std::bind(&session::handle_send,
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
        auto send_timeout = core::send_timeout();
        if (send_timeout == duration::zero())
            send_timeout = m_client_timeout->send_timeout;
        std::future_status status = written.wait_for(send_timeout);
        core::send_timeout(duration::zero());
        if (status != std::future_status::ready)
        {
            RADRPC_LOG("client::session::send: Timeout on send");
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


        RADRPC_LOG("client::session::send_recv: Write " << data_size + sizeof(io_header)
                                                << "bytes [RID:" << result_id
                                                << "]");
        core::weak_post<Derived>(derived().shared_from_this(),
                                 derived().m_stream.get_executor(),
                                 std::bind(&session::handle_send,
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
        auto send_timeout = core::send_timeout();
        if (send_timeout == duration::zero())
            send_timeout = m_client_timeout->send_timeout;
        std::future_status status = written.wait_for(send_timeout);
        core::send_timeout(duration::zero());
        if (status != std::future_status::ready)
        {
            RADRPC_LOG("client::session::send_recv: Timeout on send");
            ec = boost::asio::error::timed_out;
            return;
        }


        // Wait for response
        auto response_timeout = core::response_timeout();
        if (response_timeout == duration::zero())
            response_timeout = m_client_timeout->response_timeout;
        if (!m_cache.wait(result_id, response_timeout, recv_bytes))
        {
            RADRPC_LOG("client::session::send_recv: Timeout on receive");
            ec = boost::asio::error::timed_out;
        }
        core::response_timeout(duration::zero());
    }
};

} // namespace client
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_CLIENT_SESSION_HPP