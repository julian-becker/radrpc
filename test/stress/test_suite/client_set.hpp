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

#ifndef RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_SET_HPP
#define RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_SET_HPP

#include <cstdint>
#include <ostream>

#include <test/core/defaults.hpp>
#include <test/stress/test_suite/rpc_command.hpp>

namespace test {
namespace stress {
namespace test_suite {

using namespace test::common;
using namespace test::core;

struct client_set
{
    uint32_t clients_per_mode;
    uint32_t max_threads;
    uint32_t min_queue_delay_ms;
    uint32_t max_queue_delay_ms;
    uint32_t disconnect_chance;
    uint32_t timeout_ms;
    uint32_t restart_chance;
    uint32_t runtime_secs;
    uint32_t rpc_command_flags;
    bool random_send_timeout;

    client_set() :
        clients_per_mode(2),
        max_threads(4),
        min_queue_delay_ms(3),
        max_queue_delay_ms(10),
        disconnect_chance(0),
        timeout_ms(300),
        restart_chance(0),
        runtime_secs(600),
        rpc_command_flags(uint32_t(
            rpc_command::echo | rpc_command::send | rpc_command::send_recv |
            rpc_command::send_broadcast | rpc_command::ping)),
        random_send_timeout(true)
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const client_set &o)
{
    os << "clients_per_mode: " << o.clients_per_mode << std::endl
       << "max_threads: " << o.max_threads << std::endl
       << "min_queue_delay_ms: " << o.min_queue_delay_ms << std::endl
       << "max_queue_delay_ms: " << o.max_queue_delay_ms << std::endl
       << "disconnect_chance: " << o.disconnect_chance << std::endl
       << "timeout_ms: " << o.timeout_ms << std::endl
       << "restart_chance: " << o.restart_chance << std::endl
       << "runtime_secs: " << o.runtime_secs << std::endl
       << "rpc_command_flags: " << o.rpc_command_flags << std::endl
       << "random_send_timeout: " << o.random_send_timeout << std::endl;
    return os;
}

} // namespace test_suite
} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_SET_HPP