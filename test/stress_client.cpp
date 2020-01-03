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
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#define RADRPC_TEST_BINARY
#include <test/dep/unit_utils.hpp>
#define CATCH_CONFIG_RUNNER
#include <test/dep/catch.hpp>




///< if 'server_settings.response_chance' is 100, it will wait long enough for a
///< response
const uint32_t WAIT_RESPONSE_MS = 1000 * 4;
///< used for debug purposes
std::atomic<int> current_run = ATOMIC_VAR_INIT(0);

client_settings default_client_set()
{
    client_settings set{};
    // The amount of clients is just fine for executing
    // many actions on the same client, so it will rather
    // trigger something bad.
    set.clients_per_mode = 2;
    set.max_threads = 4;
    set.min_queue_delay_ms = 3;
    set.max_queue_delay_ms = 10;
    set.disconnect_chance = 0;
    set.timeout_ms = WAIT_RESPONSE_MS;
    set.restart_chance = 0;
    set.send_attempts = 3;
    set.attempts_delay_ms = 200;
    set.random_send_timeout = true;
    return set;
}

server_settings default_server_set()
{
    server_settings m_srv_set{};
    m_srv_set.accept_chance = 100;
    m_srv_set.connect_chance = 100;
    m_srv_set.response_chance = 100;
    m_srv_set.close_chance = 0;
    m_srv_set.min_delay_ms = 20;
    m_srv_set.max_delay_ms = 50;
    m_srv_set.broadcast_delay_ms = 1000;
    m_srv_set.test_entries = 1500;
    return m_srv_set;
}

client_config default_client_config()
{
    client_config cfg;
    cfg.host_address = "127.0.0.1"; //"192.168.2.104"
    cfg.port = 3377;
    cfg.max_read_bytes = 0xFFFFFFFF;
    cfg.send_attempts = 0;
    cfg.send_attempt_delay = duration::zero();
    return cfg;
}

client_timeout default_client_timeout()
{
    radrpc::client_timeout cfg;
    cfg.handshake_timeout = std::chrono::milliseconds(500);
    cfg.send_timeout = std::chrono::milliseconds(500);
    cfg.response_timeout = std::chrono::milliseconds(500);
    return cfg;
}

void shutdown_server()
{
    sleep_ms(3000);
    auto cfg = default_client_config();
    cfg.port = 3378;
    auto timeout = default_client_timeout();
    client<client_mode::plain> cl(cfg, timeout);
    if (cl.connect(10, std::chrono::seconds(1)))
        cl.send(STRESS_RPC_SHUTDOWN, std::vector<char>());
}




class test_suite
{
    const client_settings m_cl_set;
    const server_settings m_srv_set;
    const client_config &m_cl_cfg;
    const client_timeout &m_cl_timeout;
    test_data m_test_data;
    std::atomic<bool> m_running;
    std::vector<std::unique_ptr<client<client_mode::plain>>> m_plain_clients;
#ifdef RADRPC_SSL_SUPPORT
    std::vector<std::unique_ptr<client<client_mode::ssl>>> m_ssl_clients;
#endif
    ThreadPool m_pool;
    std::thread m_worker;

    void listen_handler(receive_buffer &data)
    {
        auto ec = m_test_data.data_entry_valid(data.data(), data.size());
        if (ec != DATA_VALID)
        {
            UNIT_THROW("Run " << current_run << " listen_handler "
                              << data.size() << " / " << data
                              << "\nDataError:" << ec);
            m_running = false;
        }
    };

  public:
    test_suite(const client_settings &p_cl_set,
               const server_settings &p_srv_set,
               const client_config &p_cl_cfg,
               const client_timeout &p_cl_timeout) :
        m_cl_set(p_cl_set),
        m_srv_set(p_srv_set),
        m_cl_cfg(p_cl_cfg),
        m_cl_timeout(p_cl_timeout),
        m_test_data({}),
        m_running(false),
        m_pool(p_cl_set.max_threads, p_cl_set.max_threads * 2)
    {
        m_test_data.init_test_data(m_srv_set.test_entries);
        handshake_request req;
        req.insert("y", "yyyyyyyyyy");
        m_plain_clients.reserve(m_cl_set.clients_per_mode);
#ifdef RADRPC_SSL_SUPPORT
        m_ssl_clients.reserve(m_cl_set.clients_per_mode);
#endif
        for (size_t i = 0; i < m_cl_set.clients_per_mode; i++)
        {
#ifdef RADRPC_SSL_SUPPORT
            ssl::context ssl_ctx(ssl::context::sslv23);
            boost::system::error_code ec;
            ssl_ctx.add_certificate_authority(
                boost::asio::buffer(
                    ssl_test_files::client_certificate,
                    std::strlen(ssl_test_files::client_certificate)),
                ec);
            if (ec)
                UNIT_THROW(
                    "+test_suite: add_certificate_authority: " << ec.message());
            m_ssl_clients.emplace_back(
                std::make_unique<client<client_mode::ssl>>(
                    m_cl_cfg, m_cl_timeout, std::move(ssl_ctx)));
            m_ssl_clients[i]->set_handshake_request(req);
            m_ssl_clients[i]->listen_broadcast(
                STRESS_RPC_SERVER_MSG,
                std::bind(
                    &test_suite::listen_handler, this, std::placeholders::_1));
#endif
            m_plain_clients.emplace_back(
                std::make_unique<client<client_mode::plain>>(m_cl_cfg,
                                                             m_cl_timeout));
            m_plain_clients[i]->set_handshake_request(req);
            m_plain_clients[i]->listen_broadcast(
                STRESS_RPC_SERVER_MSG,
                std::bind(
                    &test_suite::listen_handler, this, std::placeholders::_1));
        }
    }

    ~test_suite()
    {
        if (m_worker.joinable())
            m_worker.join();
    }

    void connect_clients()
    {
        for (uint32_t i = 0; i < m_cl_set.clients_per_mode; ++i)
        {
            m_plain_clients[i]->connect();
#ifdef RADRPC_SSL_SUPPORT
            m_ssl_clients[i]->connect();
#endif
        }
    }

    bool set_wait_server()
    {
        auto cfg = default_client_config();
        cfg.port = 3378;
        auto timeout = default_client_timeout();
        // It takes some time to initialize the test data serverside with big
        // data
        timeout.response_timeout =
            std::chrono::seconds(10) * m_srv_set.test_entries;
        client<client_mode::plain> cl(cfg, timeout);
        if (cl.connect(10, std::chrono::seconds(1)))
        {
            auto settings_bytes = to_bytes(m_srv_set);
            auto recv_bytes = cl.send_recv(STRESS_RPC_INIT, settings_bytes);
            return !recv_bytes.empty();
        }
        return false;
    }

    void run()
    {
        m_running = true;
        m_worker = std::thread([&] {
            while (m_running)
            {
                if (m_pool.can_queue())
                {
                    m_pool.enqueue([&] {
#ifdef RADRPC_SSL_SUPPORT
                        if (rnd_bool(50))
                        {
                            pulse(m_ssl_clients);
                        }
                        else
#endif
                        {
                            pulse(m_plain_clients);
                        }
                    });
                    rnd_sleep_ms(m_cl_set.min_queue_delay_ms,
                              m_cl_set.max_queue_delay_ms);
                }
                sleep_ms(1);
            }
        });
    }

    void stop()
    {
        m_running = false;
        if (m_worker.joinable())
            m_worker.join();
    }

    template <class Clients> bool pulse(Clients &clients)
    {
        int idx = rnd(0, m_cl_set.clients_per_mode - 1);
        auto action = (StressRpcCommands)rnd(0, STRESS_RPC_INIT - 1);
        const std::vector<char> rdata = m_test_data.get_random_data();

        if (!execute_action(clients, idx, action, rdata))
            return false;

        if (rnd_bool(m_cl_set.disconnect_chance))
            clients[idx]->disconnect();
        if (rnd_bool(m_cl_set.restart_chance))
            clients[idx]->send_recv(STRESS_RPC_RESTART, std::vector<char>());
        return true;
    }

    template <class Clients>
    bool execute_action(Clients &clients,
                        int client_idx,
                        StressRpcCommands action,
                        const std::vector<char> &data)
    {
        if (m_cl_set.random_send_timeout)
        {
            clients[client_idx]->set_send_timeout(
                std::chrono::milliseconds(rnd(0, m_cl_set.timeout_ms)));
            clients[client_idx]->set_response_timeout(
                std::chrono::milliseconds(rnd(0, m_cl_set.timeout_ms)));
        }
        switch (action)
        {
            case STRESS_RPC_ECHO:
            {
                auto recv_bytes = clients[client_idx]->send_recv(action, data);
                if (recv_bytes.empty() &&
                    (m_srv_set.response_chance != 100 ||
                     m_srv_set.close_chance != 100 ||
                     m_cl_set.restart_chance != 0 ||
                     m_cl_set.timeout_ms != WAIT_RESPONSE_MS))
                    break;
                if (!m_test_data.data_compare(recv_bytes.data(),
                                              recv_bytes.size(),
                                              data.data(),
                                              data.size()))
                {
                    UNIT_THROW("Run " << current_run << " STRESS_RPC_ECHO "
                                      << recv_bytes.size()
                                      << " != " << data.size());
                    return false;
                }
                break;
            }
            case STRESS_RPC_SEND_RECV:
            {
                auto data_size = data.size();
                auto recv_bytes = clients[client_idx]->send_recv(action, data);
                if (recv_bytes.empty() &&
                    (m_srv_set.response_chance != 100 ||
                     m_srv_set.close_chance != 100 ||
                     m_cl_set.restart_chance != 0 ||
                     m_cl_set.timeout_ms != WAIT_RESPONSE_MS))
                    break;
                auto ec = m_test_data.data_entry_valid(recv_bytes.data(),
                                                       recv_bytes.size());
                if (ec != DATA_VALID)
                {
                    UNIT_THROW("Run " << current_run << " STRESS_RPC_SEND_RECV "
                                      << recv_bytes.size() << " / " << data_size
                                      << "\nDataError:" << ec);
                    return false;
                }
                break;
            }
            case STRESS_RPC_SEND:
            {
                if (!clients[client_idx]->send(action, data) &&
                    m_srv_set.close_chance == 0 &&
                    m_cl_set.restart_chance == 0 &&
                    !m_cl_set.random_send_timeout &&
                    m_cl_set.disconnect_chance == 0)
                {
                    UNIT_THROW("Run " << current_run
                                      << " STRESS_RPC_SEND failed to send");
                    return false;
                }
                break;
            }
            case STRESS_RPC_SEND_BROADCAST:
            {
                auto data_size = data.size();
                auto recv_bytes = clients[client_idx]->send_recv(action, data);
                if (recv_bytes.empty() &&
                    (m_srv_set.response_chance != 100 ||
                     m_srv_set.close_chance != 100 ||
                     m_cl_set.restart_chance != 0 ||
                     m_cl_set.timeout_ms != WAIT_RESPONSE_MS))
                    break;
                auto ec = m_test_data.data_entry_valid(recv_bytes.data(),
                                                       recv_bytes.size());
                if (ec != DATA_VALID)
                {
                    UNIT_THROW("Run " << current_run
                                      << " STRESS_RPC_SEND_BROADCAST "
                                      << recv_bytes.size() << " / " << data_size
                                      << "\nDataError:" << ec);
                    return false;
                }
                break;
            }
            case STRESS_RPC_PING:
            {
                if (!clients[client_idx]->ping() &&
                    m_srv_set.close_chance == 0 &&
                    m_cl_set.restart_chance == 0 &&
                    !m_cl_set.random_send_timeout &&
                    m_cl_set.timeout_ms == WAIT_RESPONSE_MS)

                {
                    UNIT_THROW("Run " << current_run << " STRESS_RPC_PING");
                    return false;
                }
                break;
            }
            default:
                break;
        }
        return true;
    }
};




inline auto run_time()
{
    static auto time = std::chrono::minutes(20);
    //static auto time = std::chrono::seconds(12);
    return time;
}
int main(int argc, char *argv[])
{
    // Todo parse commands & set the stress test
    sanitizer_info();
    int result = Catch::Session().run(argc, argv);
    return result;
}




////////////////////////////////////////////////////////
// Test cases for different actions
////////////////////////////////////////////////////////

TEST_CASE("basic")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.random_send_timeout = false;
    auto srv_set = default_server_set();
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'basic'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->connect_clients();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}

TEST_CASE("reject clients in listener")
{
	++current_run;
	auto cl_set = default_client_set();
	cl_set.timeout_ms = 300;
	auto srv_set = default_server_set();
	srv_set.connect_chance = 80;
	auto cl_cfg = default_client_config();
	auto cl_timeout = default_client_timeout();

	INFO("Run 'reject clients in listener'\n\nclient_settings:\n"
		<< cl_set << "\nserver_settings:\n"
		<< srv_set);

	auto test =
		std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
	test->set_wait_server();
	test->run();
	std::this_thread::sleep_for(run_time());
	test->stop();
}

TEST_CASE("reject clients in session")
{
	++current_run;
	auto cl_set = default_client_set();
	cl_set.timeout_ms = 300;
	auto srv_set = default_server_set();
	srv_set.accept_chance = 80;
	auto cl_cfg = default_client_config();
	auto cl_timeout = default_client_timeout();

	INFO("Run 'reject clients in session'\n\nclient_settings:\n"
		<< cl_set << "\nserver_settings:\n"
		<< srv_set);

	auto test =
		std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
	test->set_wait_server();
	test->run();
	std::this_thread::sleep_for(run_time());
	test->stop();
}

TEST_CASE("restart server")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.restart_chance = 20;
    cl_set.timeout_ms = 300;
    auto srv_set = default_server_set();
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'restart server'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}

TEST_CASE("client disconnect")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.disconnect_chance = 20;
    auto srv_set = default_server_set();
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'client disconnect'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}

TEST_CASE("no response")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.timeout_ms = 300;
    auto srv_set = default_server_set();
    srv_set.response_chance = 80;
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'no response'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}

TEST_CASE("serverside close")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.timeout_ms = 300;
    auto srv_set = default_server_set();
    srv_set.close_chance = 20;
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'serverside close'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}

TEST_CASE("All")
{
    ++current_run;
    auto cl_set = default_client_set();
    cl_set.restart_chance = 10;
    cl_set.disconnect_chance = 10;
    cl_set.timeout_ms = 300;
    auto srv_set = default_server_set();
	srv_set.accept_chance = 90;
	srv_set.close_chance = 10;
	srv_set.connect_chance = 90;
    srv_set.response_chance = 90;
    auto cl_cfg = default_client_config();
    auto cl_timeout = default_client_timeout();

    INFO("Run 'All'\n\nclient_settings:\n"
         << cl_set << "\nserver_settings:\n"
         << srv_set);

    auto test =
        std::make_unique<test_suite>(cl_set, srv_set, cl_cfg, cl_timeout);
    test->set_wait_server();
    test->run();
    std::this_thread::sleep_for(run_time());
    test->stop();
}




TEST_CASE("shutdown server")
{
    shutdown_server();
}