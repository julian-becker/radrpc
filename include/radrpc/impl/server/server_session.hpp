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

#ifndef RADRPC_IMPL_SERVER_SERVER_SESSION_HPP
#define RADRPC_IMPL_SERVER_SERVER_SESSION_HPP

#include <cstdint>
#include <deque>

#include <radrpc/common/receive_buffer.hpp>
#include <radrpc/common/server_config.hpp>
#include <radrpc/common/server_timeout.hpp>
#include <radrpc/common/session_context.hpp>
#include <radrpc/core/endian.hpp>
#include <radrpc/core/data/push.hpp>
#include <radrpc/debug/log.hpp>
#include <radrpc/impl/server/session_accept.hpp>
#include <radrpc/impl/server/session_manager.hpp>

namespace radrpc {
namespace impl {
namespace server {

using namespace radrpc::common;
using namespace radrpc::core;
using namespace radrpc::core::endian;

/**
 * Represents a server session to communicate with the client.
 * @tparam Derived The accept facility.
 * @tparam SharedRef The shared reference, usually the session manager.
 */
template <class Derived> class server_session : private session_context
{
    /// Info for read error.
    bool m_write_error;

    /// Info for write error.
    bool m_read_error;

    /// The manager shared among sessions.
    std::shared_ptr<session_manager> m_manager;

    /// A queue to hold the data to send.
    std::deque<std::shared_ptr<const data::push>> m_queue;

    /// The used server timeout.
    const server_timeout &m_server_timeout;

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
        if (m_receive_buffer_ref.size() >= sizeof(io_header))
        {
            // Copy header & clear its portion
            m_header = *reinterpret_cast<const io_header *>(
                m_receive_buffer_ref.data());
            m_receive_buffer.consume(sizeof(io_header));

            // Convert to host byte order
            m_header.call_id = ntohl(m_header.call_id);
            m_header.result_id = ntohl_uint64(m_header.result_id);

            m_receive_buffer_ref =
                boost::beast::buffers_front(m_receive_buffer.data());

            if (m_header.call_id >= config::max_call_id)
            {
                RADRPC_LOG("server_session::call_function: Call id "
                           << m_header.call_id << " is out of bounds");
            }
            else if (m_manager->bound_funcs[m_header.call_id] == nullptr)
            {
                RADRPC_LOG("server_session::call_function: Call id "
                           << m_header.call_id
                           << " was not bound to any function");
            }
            else
            {
                // Call bound function
                m_manager->bound_funcs[m_header.call_id](this);
                // Check if bound function requests to close
                if (m_bound_close)
                {
                    RADRPC_LOG("server_session::call_function: Bound "
                                "function close request");
                    derived().close_session(true);
                    return;
                }
                // Check if bound function has added bytes to send back to
                // client
                if (!response.empty())
                {
                    // Convert to network byte order (big endian)
                    m_header.call_id = htonl(m_header.call_id);
                    m_header.result_id = htonl_uint64(m_header.result_id);

                    const auto push =
                        new data::push(m_header, std::move(response));
                    // Don't need to clear, since it will be reassigned after
                    // call in 'on_read'
                    handle_send(std::shared_ptr<const data::push>(push));
                }
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
    void handle_send(const std::shared_ptr<const data::push> &data)
    {
        if (derived().m_closing || derived().m_remote_sent_close ||
            m_write_error || m_read_error)
            return;
        if (m_queue.size() >= config::queue_send_max_bytes)
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
        auto packet = m_queue.front();
        derived().m_stream.async_write(
            std::vector<boost::asio::const_buffer>{
                boost::asio::buffer(reinterpret_cast<const char *>(&packet->header),
                                    sizeof(io_header)),
                boost::asio::buffer(packet->body),
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
        //                                  packet->shared_from_this())
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
        if (derived().m_remote_sent_close)
            return;
        if (ec || derived().m_closing || m_read_error)
        {
            RADRPC_LOG("server_session::on_write: " << ec.message());
            m_write_error = true;
            m_queue.clear();
            derived().close_session(false);
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
        if (derived().m_remote_sent_close)
            return;
        if (ec)
        {
            RADRPC_LOG("server_session::on_read: " << ec.message());
            m_read_error = true;
            derived().close_session(false);
            return;
        }
        RADRPC_LOG("server_session::on_read: " << bytes_transferred << "bytes");
        if (!derived().m_closing && !m_write_error)
            call_function();
        m_receive_buffer.consume(m_receive_buffer.size());
        response = std::vector<char>();
        // Read operations allowed, since
        // it would read until an error occures.
        read();
    }

  public:
    /// The session info to expose in handlers.
    session_info info;

    /**
     * @param p_remote_host The client's ip address.
     * @param p_manager The manager shared among sessions.
     * @param p_server_timeout The server timeout.
     * @param p_session_cfg The config to use for this session.
     */
    explicit server_session(const std::string &&p_remote_host,
                            const std::shared_ptr<session_manager> &p_manager,
                            const server_timeout &p_server_timeout,
                            const session_config &p_session_cfg) :
        radrpc::session_context(reinterpret_cast<uint64_t>(this),
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
    void send(std::shared_ptr<const data::push> data)
    {
        boost::asio::post(
            derived().m_stream.get_executor(),
            boost::beast::bind_front_handler(&server_session::handle_send,
                                             derived().shared_from_this(),
                                             data));
    }
};

} // namespace server
} // namespace impl
} // namespace radrpc

#endif // RADRPC_IMPL_SERVER_SERVER_SESSION_HPP