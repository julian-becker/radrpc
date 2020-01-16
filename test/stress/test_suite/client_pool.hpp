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

#ifndef RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_POOL_HPP
#define RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_POOL_HPP

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <radrpc/client.hpp>

#include <test/core/test_data.hpp>
#include <test/core/thread_pool.hpp>
#include <test/stress/test_suite/client_set.hpp>
#include <test/stress/test_suite/rpc_command.hpp>
#include <test/stress/test_suite/server_set.hpp>

namespace test {
namespace stress {
namespace test_suite {

using namespace radrpc;
using namespace test::common;

class client_pool
{
    ///
    const client_set m_cl_set;

    ///
    const server_set m_srv_set;

    ///
    const client_config &m_cl_cfg;

    ///
    const client_timeout &m_cl_timeout;

    ///
    test_data m_test_data;

    ///
    std::atomic<bool> m_running;

    ///
    std::vector<std::unique_ptr<client::plain>> m_plain_clients;
#ifdef RADRPC_SSL_SUPPORT
    ///
    std::vector<std::unique_ptr<client::ssl>> m_ssl_clients;
#endif
    ///
    thread_pool m_pool;

    ///
    std::thread m_worker;

    ///
    std::string m_current_test_case;

  public:
  private:
    void listen_handler(receive_buffer &data);

  public:
    client_pool(const client_set &p_cl_set,
                const server_set &p_srv_set,
                const client_config &p_cl_cfg,
                const client_timeout &p_cl_timeout);

    ~client_pool();

    /**
     * @brief Set the test case object
     *
     * @param name
     */
    void set_test_case(const std::string &name);

    /**
     * @brief Set the wait server object
     *
     * @return true
     * @return false
     */
    bool set_wait_server();

    /**
     * @brief
     *
     */
    void connect_clients();

    /**
     * @brief
     *
     */
    void run();

    /**
     * @brief
     *
     */
    void wait();

    /**
     * @brief
     *
     */
    void stop();

    /**
     * @brief
     *
     * @tparam Clients
     * @param clients
     * @return true
     * @return false
     */
    template <class Clients> bool pulse(Clients &clients);

    /**
     * @brief
     *
     * @tparam Clients
     * @param clients
     * @param client_idx
     * @param action
     * @param data
     * @return true
     * @return false
     */
    template <class Clients>
    bool execute_action(Clients &clients,
                        int client_idx,
                        rpc_command action,
                        const std::vector<char> &data);
};

} // namespace test_suite
} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_TEST_SUITE_CLIENT_POOL_HPP