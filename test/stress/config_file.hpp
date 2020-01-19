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

#ifndef RADRPC_TEST_STRESS_CONFIG_FILE_HPP
#define RADRPC_TEST_STRESS_CONFIG_FILE_HPP

#include <boost/filesystem.hpp>

#include <test/stress/test_suite/client_set.hpp>
#include <test/stress/test_suite/server_set.hpp>

#include <radrpc/radrpc.hpp>

namespace test {
namespace stress {

using namespace radrpc;
using namespace test::common;
using namespace test::core;

/**
 * @brief
 *
 */
class config_file
{
    ///
    client_config m_client_cfg;

    ///
    server_config m_server_cfg;

    ///
    session_config m_session_cfg;

    ///
    test_suite::client_set m_client_set;

    ///
    test_suite::server_set m_server_set;

    /**
     * @brief
     *
     * @param file
     */
    void load_config(const boost::filesystem::path &file);

    /**
     * @brief
     *
     * @param file
     */
    void store_config(const boost::filesystem::path file);

  public:
    config_file(const boost::filesystem::path file);

    /**
     * @brief Get the client config object
     *
     * @return client_config
     */
    client_config get_client_config();

    /**
     * @brief Get the server config object
     *
     * @return server_config
     */
    server_config get_server_config();

    /**
     * @brief Get the session config object
     *
     * @return session_config
     */
    session_config get_session_config();

    /**
     * @brief Get the client set object
     *
     * @return test_suite::client_set
     */
    test_suite::client_set get_client_set();

    /**
     * @brief Get the server set object
     *
     * @return test_suite::server_set
     */
    test_suite::server_set get_server_set();
};

} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_CONFIG_FILE_HPP