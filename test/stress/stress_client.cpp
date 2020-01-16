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

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <radrpc/client.hpp>
#include <test/core/defaults.hpp>
#include <test/core/sanitizer.hpp>
#include <test/core/sleep.hpp>
#include <test/stress/config_file.hpp>
#include <test/stress/test_suite/client_pool.hpp>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace test::common;
using namespace test::core;
using namespace test::stress;
using namespace std::chrono;




std::unique_ptr<config_file> g_cfg = nullptr;

void set_defaults(test_suite::client_set &cl_set,
                  test_suite::server_set &srv_set,
                  client_config &cl_cfg,
                  client_timeout &cl_timeout)
{
    cl_set.timeout_ms = 300;
    cl_set.disconnect_chance = 0;
    cl_set.restart_chance = 0;
    cl_set.random_send_timeout = true;

    srv_set.accept_chance = 100;
    srv_set.connect_chance = 100;
    srv_set.response_chance = 100;
    srv_set.close_chance = 0;
    srv_set.broadcast_delay_ms = 0;

    cl_cfg.max_read_bytes = 4294967295;
    cl_cfg.send_attempts = 0;
    cl_cfg.send_attempt_delay = steady_clock::duration::zero();

    cl_timeout.response_timeout = std::chrono::milliseconds(300);
}

void run(const test_suite::client_set &cl_set,
         const test_suite::server_set &srv_set,
         const client_config &cl_cfg,
         const client_timeout &cl_timeout)
{
    auto cp = std::make_unique<test_suite::client_pool>(
        cl_set, srv_set, cl_cfg, cl_timeout);
    cp->set_wait_server();
    cp->run();
    cp->wait();
    cp->stop();
}

void run_destruct(test_suite::client_set cl_set,
                  const test_suite::server_set &srv_set,
                  const client_config &cl_cfg,
                  const client_timeout &cl_timeout)
{
    const int div = 100;
    cl_set.runtime_secs =
        (uint32_t)std::min(((float)cl_set.runtime_secs) / div, 2.0f);
    for (std::size_t i = 0; i < div; i++)
    {
        auto cp = std::make_unique<test_suite::client_pool>(
            cl_set, srv_set, cl_cfg, cl_timeout);
        cp->set_wait_server();
        cp->run();
        cp->wait();
        cp->stop();
    }
}

void shutdown_server()
{
    sleep_ms(3000);
    auto cfg = default_client_config();
    cfg.port = 3378;
    auto timeout = default_client_timeout();
    client::plain cl(cfg, timeout);
    if (cl.connect(10, std::chrono::seconds(1)))
        cl.send((int)rpc_command::shutdown, std::vector<char>());
}

int main(int argc, char *argv[])
{
    g_cfg = std::unique_ptr<config_file>(new config_file("stress_test.cfg"));
    sanitizer_info();
    int result = Catch::Session().run(argc, argv);
    return result;
}




////////////////////////////////////////////////////////
// Test cases for different actions & situations
////////////////////////////////////////////////////////

TEST_CASE("basic")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    // Wait long enough for a response
    cl_set.timeout_ms = defaults::wait_response_ms;
    cl_set.random_send_timeout = false;

    cl_timeout.response_timeout =
        std::chrono::milliseconds(defaults::wait_response_ms);

    INFO("Run 'basic'\nclient_set:\n" << cl_set << "server_set:\n" << srv_set);

    // Manually connect clients with 'random_send_timeout = false'
    auto cp = std::make_unique<test_suite::client_pool>(
        cl_set, srv_set, cl_cfg, cl_timeout);
    cp->set_wait_server();
    cp->connect_clients();
    cp->run();
    cp->wait();
    cp->stop();
}

TEST_CASE("reject_listener - reject clients in listener")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    srv_set.connect_chance = 80;

    INFO("Run 'reject_listener'\nclient_set:\n"
         << cl_set << "server_set:\n"
         << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("reject_session - reject clients in session")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    srv_set.accept_chance = 80;

    INFO("Run 'reject_session'\nclient_set:\n"
         << cl_set << "server_set:\n"
         << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("restart - restarts the server")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    cl_set.restart_chance = 20;

    INFO("Run 'restart'\nclient_set:\n"
         << cl_set << "server_set:\n"
         << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("disconnect - disconnect clients")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    cl_set.timeout_ms = defaults::wait_response_ms;

    cl_set.disconnect_chance = 20;

    INFO("Run 'disconnect'\nclient_set:\n"
         << cl_set << "server_set:\n"
         << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("no_response - server sends no response")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    srv_set.response_chance = 80;

    INFO("Run 'no_response'\nclient_set:\n"
         << cl_set << "server_set:\n"
         << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("close - server closes connections")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);

    srv_set.close_chance = 20;

    INFO("Run 'close'\nclient_set:\n" << cl_set << "server_set:\n" << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

TEST_CASE("all")
{
    auto cl_set = g_cfg->get_client_set();
    auto srv_set = g_cfg->get_server_set();
    auto cl_cfg = g_cfg->get_client_config();
    auto cl_timeout = default_client_timeout();

    set_defaults(cl_set, srv_set, cl_cfg, cl_timeout);
    
    cl_set.restart_chance = 20;
    cl_set.disconnect_chance = 20;

    srv_set.connect_chance = 80;
    srv_set.accept_chance = 80;
    srv_set.response_chance = 80;
    srv_set.close_chance = 20;

    INFO("Run 'all'\nclient_set:\n" << cl_set << "server_set:\n" << srv_set);

    run(cl_set, srv_set, cl_cfg, cl_timeout);
}

////////////////////////////////////////////////////////
// Test cases with broadcast, since this may have
// a big impact on the session manager
// Todo tidy this up
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// Test cases with broadcast & client destruct
////////////////////////////////////////////////////////












TEST_CASE("shutdown server") { shutdown_server(); }