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

#ifndef RADRPC_TYPES_HPP
#define RADRPC_TYPES_HPP

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#ifdef RADRPC_SSL_SUPPORT
#include <boost/beast/ssl.hpp>
#endif

#include "radrpc/config.hpp"

namespace radrpc {
namespace websocket = boost::beast::websocket;
#ifdef RADRPC_SSL_SUPPORT
namespace ssl = boost::asio::ssl;
#endif
typedef websocket::request_type handshake_request;
typedef websocket::response_type handshake_response;
using tcp = boost::asio::ip::tcp;

enum class connection_state
{
    none,
    accept,
    resolve,
    connect,
    ssl_handshake,
    read_handshake,
    handshake,
    established,
};

struct client_streams
{
    typedef websocket::stream<boost::beast::tcp_stream> plain_stream;
#ifdef RADRPC_SSL_SUPPORT
    typedef websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>>
        ssl_stream;
#endif
};

struct server_streams
{
    typedef websocket::stream<boost::beast::tcp_stream> plain_stream;
#ifdef RADRPC_SSL_SUPPORT
    typedef websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>>
        ssl_stream;
#endif
};

class client_config
{
  public:
    std::string host_address;    ///< The address to connect to.
    uint16_t port;               ///< The port to open.
    std::size_t max_read_bytes;  ///< Maximum message size to receive.
    unsigned int send_attempts;  ///< The amount of attempts to send a message.
    duration send_attempt_delay; ///< The delay after a failed attempt.

    client_config() :
        host_address(""),
        port(0),
        max_read_bytes(0),
        send_attempts(0),
        send_attempt_delay({})
    {
    }
};

class client_timeout
{
  public:
    duration
        handshake_timeout; ///< The timeout for handshake used in 'connect()'.
    duration send_timeout; ///< The timeout for sending a message used in
                           ///< 'send()' & 'send_recv()'.
    duration response_timeout; ///< The timeout for a response used in
                               ///< 'send_recv()' or 'ping()'

    explicit client_timeout() :
        handshake_timeout(duration::max()),
        send_timeout(duration::max()),
        response_timeout(duration::max())
    {
    }
};

enum class server_mode : unsigned char
{
    plain = 1 << 0,
    ssl = 1 << 1,
};

inline server_mode operator|(server_mode lhs, server_mode rhs)
{
    return static_cast<server_mode>(
        static_cast<std::underlying_type<server_mode>::type>(lhs) |
        static_cast<std::underlying_type<server_mode>::type>(rhs));
}

inline bool operator&(server_mode lhs, server_mode rhs)
{
    return static_cast<bool>(
        static_cast<std::underlying_type<server_mode>::type>(lhs) &
        static_cast<std::underlying_type<server_mode>::type>(rhs));
}

class server_config
{
  public:
    std::string host_address;  ///< The address to host on, usually "0.0.0.0"
    uint16_t port;             ///< The port to open.
    unsigned int workers;      ///< The amount of workers to run the io context.
    unsigned int max_sessions; ///< The maximum amount of sessions.
    std::size_t
        max_handshake_bytes; ///< The maximum handshake size for clients.
    server_mode mode; ///< The mode represented as flag to run the server.

    server_config() :
        host_address(""),
        port(0),
        workers(0),
        max_sessions(0),
        max_handshake_bytes(0),
        mode(server_mode::plain)
    {
    }
};

class server_timeout
{
  public:
    duration handshake_or_close_timeout; ///< The timeout used for handshake or
                                         ///< close.

    explicit server_timeout() : handshake_or_close_timeout(duration::max()) {}
};

class session_config
{
  public:
    std::size_t
        max_transfer_bytes; ///< Maximum message size of each received message.
    duration ping_delay;    ///< The delay to ping the client to check activity.

    session_config() : max_transfer_bytes(0), ping_delay(duration::zero()) {}
};

struct session_info
{
    const uint64_t id;              ///< The unqiue id of the session.
    const std::string &remote_host; ///< The client's ip address.
    const handshake_request &request_handshake; ///< The client's handshake.
    handshake_response
        &response_handshake; ///< The sent handshake to the client.
    session_config &config;  ///< The used configuration for this session.
};

class receive_buffer : public boost::beast::flat_buffer
{
  public:
    receive_buffer() = default;

    boost::beast::flat_buffer &base()
    {
        return static_cast<boost::beast::flat_buffer &>(*this);
    }

    operator bool() { return !empty(); }

    /**
     * Gets the pointer of the buffer.
     */
    const char *data()
    {
        return reinterpret_cast<const char *>(
            boost::beast::buffers_front(base().data()).data());
    }

    /**
     * Gets the size of the buffer.
     */
    std::size_t size()
    {
        return boost::beast::buffers_front(base().data()).size();
    }

    /**
     * Clears the buffer.
     */
    void clear() { base().consume(base().size()); }

    /**
     * Check whether the buffer is empty.
     */
    bool empty()
    {
        return boost::beast::buffers_front(base().data()).size() == 0;
    }
};

} // namespace radrpc

#endif // RADRPC_TYPES_HPP