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

#ifndef RADRPC_TEST_STRESS_BROADCASTER_HPP
#define RADRPC_TEST_STRESS_BROADCASTER_HPP

#include <boost/filesystem.hpp>

#include <test/core/sleep.hpp>
#include <test/core/test_data.hpp>
#include <test/stress/test_suite/client_set.hpp>
#include <test/stress/test_suite/server_set.hpp>

#include "radrpc.hpp"

namespace test {
namespace stress {

using namespace radrpc;
using namespace test::common;
using namespace test::core;

class broadcaster
{
    /// Broadcast worker.
    std::thread m_worker;

    /// Worker run flag.
    std::atomic<bool> m_broadcast_run;

    /// The referenced server.
    radrpc::server &m_srv;

    /// The referenced test data.
    test_data &m_data;

    /// The id to broadcast
    const uint32_t m_call_id;

    /// The server configuration.
    test_suite::server_set m_set;

  public:
    broadcaster(radrpc::server &p_srv, test_data &p_data, rpc_command p_call_id) :
        m_broadcast_run(false),
        m_srv(p_srv),
        m_data(p_data),
        m_call_id((uint32_t)p_call_id),
        m_set({})
    {
    }

    ~broadcaster() { stop(); }

    void set_cfg(const test_suite::server_set &p_set)
    {
        m_set = p_set;
    }

    void run()
    {
        if (m_broadcast_run || m_set.broadcast_delay_ms == 0)
            return;
        m_broadcast_run = true;
        m_worker = std::thread([&] {
            while (m_broadcast_run)
            {
                m_srv.broadcast(m_call_id, m_data.get_random_data());
                sleep_ms(m_set.broadcast_delay_ms);
            }
        });
    }

    void stop()
    {
        m_broadcast_run = false;
        if (m_worker.joinable())
            m_worker.join();
    }
};

} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_BROADCASTER_HPP