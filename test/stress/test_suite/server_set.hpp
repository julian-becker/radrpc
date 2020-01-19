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

#ifndef RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP
#define RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP

#include <cstdint>
#include <ostream>

#include <boost/asio/detail/socket_ops.hpp>

#include <radrpc/common/packing.hpp>

namespace test {
namespace stress {
namespace test_suite {

PACK(struct server_set {
    uint32_t accept_chance;
    uint32_t connect_chance;
    uint32_t response_chance;
    uint32_t close_chance;
    uint32_t min_delay_ms;
    uint32_t max_delay_ms;
    uint32_t broadcast_delay_ms;
    uint32_t min_send_bytes;
    uint32_t max_send_bytes;

    server_set();

    void to_network();

    void to_host();
});

inline server_set::server_set() :
    accept_chance(100),
    connect_chance(100),
    response_chance(100),
    close_chance(0),
    min_delay_ms(20),
    max_delay_ms(50),
    broadcast_delay_ms(1000),
    min_send_bytes(1),
    max_send_bytes(0xFF)
{
}

inline void server_set::to_network()
{
    accept_chance = htonl(accept_chance);
    connect_chance = htonl(connect_chance);
    response_chance = htonl(response_chance);
    close_chance = htonl(close_chance);
    min_delay_ms = htonl(min_delay_ms);
    max_delay_ms = htonl(max_delay_ms);
    broadcast_delay_ms = htonl(broadcast_delay_ms);
    min_send_bytes = htonl(min_send_bytes);
    max_send_bytes = htonl(max_send_bytes);
}

inline void server_set::to_host()
{
    accept_chance = ntohl(accept_chance);
    connect_chance = ntohl(connect_chance);
    response_chance = ntohl(response_chance);
    close_chance = ntohl(close_chance);
    min_delay_ms = ntohl(min_delay_ms);
    max_delay_ms = ntohl(max_delay_ms);
    broadcast_delay_ms = ntohl(broadcast_delay_ms);
    min_send_bytes = ntohl(min_send_bytes);
    max_send_bytes = ntohl(max_send_bytes);
}

inline std::ostream &operator<<(std::ostream &os, const server_set &o)
{
    os << "accept_chance: " << o.accept_chance << std::endl
       << "connect_chance: " << o.connect_chance << std::endl
       << "response_chance: " << o.response_chance << std::endl
       << "close_chance: " << o.close_chance << std::endl
       << "min_delay_ms: " << o.min_delay_ms << std::endl
       << "max_delay_ms: " << o.max_delay_ms << std::endl
       << "broadcast_delay_ms: " << o.broadcast_delay_ms << std::endl
       << "min_send_bytes: " << o.min_send_bytes << std::endl
       << "max_send_bytes: " << o.max_send_bytes << std::endl;
    return os;
}

} // namespace test_suite
} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP