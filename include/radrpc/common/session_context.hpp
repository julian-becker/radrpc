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

#ifndef RADRPC_COMMON_SESSION_CONTEXT_HPP
#define RADRPC_COMMON_SESSION_CONTEXT_HPP

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket/stream_base.hpp>

#include <radrpc/config.hpp>
#include <radrpc/common/server_timeout.hpp>
#include <radrpc/common/session_config.hpp>
#include <radrpc/common/session_info.hpp>
#include <radrpc/common/io_header.hpp>

namespace radrpc {
namespace common {

/**
 * Type to expose the server session to the bound handlers.
 */
class session_context
{
  protected:
    /// Used to check whether the bound handler requests to close.
    bool m_bound_close;

    /// The io_header to send along with the bytes to the client.
    io_header m_header;

    /// The buffer to receive incoming client requests.
    boost::beast::flat_buffer m_receive_buffer;

    /// The buffer's reference to the actual data.
    boost::asio::const_buffer m_receive_buffer_ref;

    /// The config to use for this session.
    session_config m_config;

    /// The handshake from the client.
    handshake_request m_req_handshake;

    /// The handshake to send to the client.
    handshake_response m_res_handshake;

    ~session_context(){};

  public:
    /// The unique id of this session.
    uint64_t id;

    /// The clients ip address.
    const std::string remote_host;

    /// The bytes to send for a response.
    std::vector<char> response;

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

} // namespace common

using session_context = radrpc::common::session_context;

} // namespace radrpc

#endif // RADRPC_COMMON_SESSION_CONTEXT_HPP