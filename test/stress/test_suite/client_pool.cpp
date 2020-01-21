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

#include <test/common/data_state.hpp>
#include <test/common/ssl_test_files.hpp>
#include <test/core/bytes.hpp>
#include <test/core/defaults.hpp>
#include <test/core/log.hpp>
#include <test/core/random.hpp>
#include <test/core/sleep.hpp>
#include <test/core/throw.hpp>

#include <test/stress/test_suite/client_pool.hpp>

namespace test {
namespace stress {
namespace test_suite {

using namespace radrpc;
using namespace test::common;

void client_pool::listen_handler(receive_buffer &data)
{
    auto ec = m_test_data->data_valid(data.data(), data.size());
    if (ec != data_state::valid)
    {
        TEST_THROW("Error: " << m_current_test_case << " listen_handler "
                           << data.size() << " / " << data
                           << "\ndata_state:" << static_cast<uint32_t>(ec));
        m_running = false;
    }
};

client_pool::client_pool(const client_set &p_cl_set,
                         const server_set &p_srv_set,
                         const client_config &p_cl_cfg,
                         const client_timeout &p_cl_timeout) :
    m_cl_set(p_cl_set),
    m_srv_set(p_srv_set),
    m_cl_cfg(p_cl_cfg),
    m_cl_timeout(p_cl_timeout),
    m_test_data(new test_data()),
    m_running(false),
    m_pool(p_cl_set.max_threads, p_cl_set.max_threads * 2),
    m_current_test_case("")
{
    // Initialize test data
    m_test_data->set_limit(m_srv_set.min_send_bytes, m_srv_set.max_send_bytes);

    // Set handshake
    handshake_request req;
    req.insert("y", "yyyyyyyyyy");

    // Reserve memory for clients
    m_plain_clients.reserve(m_cl_set.clients_per_mode);
#ifdef RADRPC_SSL_SUPPORT
    m_ssl_clients.reserve(m_cl_set.clients_per_mode);
#endif

    // Initialize clients with handshake & handlers
    for (size_t i = 0; i < m_cl_set.clients_per_mode; i++)
    {
#ifdef RADRPC_SSL_SUPPORT
        // Create ssl context
        ssl::context ssl_ctx(ssl::context::sslv23);
        boost::system::error_code ec;
        ssl_ctx.add_certificate_authority(
            boost::asio::buffer(
                ssl_test_files::client_certificate,
                std::strlen(ssl_test_files::client_certificate)),
            ec);
        if (ec)
            TEST_THROW(
                "+client_pool: add_certificate_authority: " << ec.message());
        // Create ssl client
        m_ssl_clients.emplace_back(std::make_unique<client::ssl>(
            m_cl_cfg, m_cl_timeout, std::move(ssl_ctx)));
        m_ssl_clients[i]->set_handshake_request(req);
        m_ssl_clients[i]->listen_broadcast(
            static_cast<uint32_t>(rpc_command::server_msg),
            std::bind(
                &client_pool::listen_handler, this, std::placeholders::_1));

#endif
        // Create plain client
        m_plain_clients.emplace_back(
            std::make_unique<client::plain>(m_cl_cfg, m_cl_timeout));
        m_plain_clients[i]->set_handshake_request(req);
        m_plain_clients[i]->listen_broadcast(
            static_cast<uint32_t>(rpc_command::server_msg),
            std::bind(
                &client_pool::listen_handler, this, std::placeholders::_1));
    }
}

client_pool::~client_pool()
{
    if (m_worker.joinable())
        m_worker.join();
}

void client_pool::set_test_case(const std::string &name)
{
    m_current_test_case = name;
}

bool client_pool::set_wait_server(uint32_t seconds)
{
    if (seconds == 0)
        seconds = 5;
    auto cfg = default_client_config();
    cfg.port = 3378;
    auto timeout = default_client_timeout();
    timeout.response_timeout = std::chrono::seconds(seconds);
    client::plain cl(cfg, timeout);
    if (cl.connect(10, std::chrono::seconds(1)))
    {
        auto net_srv_set = m_srv_set;
        net_srv_set.to_network();
        auto recv_bytes =
            cl.send_recv(static_cast<uint32_t>(rpc_command::init), obj_to_bytes(net_srv_set));
        return !recv_bytes.empty();
    }
    return false;
}

void client_pool::connect_clients()
{
    for (uint32_t i = 0; i < m_cl_set.clients_per_mode; ++i)
    {
        if (!m_plain_clients[i]->connect())
            TEST_THROW("Error client couldn't connect");
#ifdef RADRPC_SSL_SUPPORT
        if (!m_ssl_clients[i]->connect())
            TEST_THROW("Error client couldn't connect");
#endif
    }
}

void client_pool::run()
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
                sleep_ms_rnd(m_cl_set.min_queue_delay_ms,
                             m_cl_set.max_queue_delay_ms);
            }
            sleep_ms(1);
        }
    });
}

void client_pool::wait(uint32_t seconds)
{
    if (seconds == 0)
        seconds = m_cl_set.runtime_secs;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void client_pool::stop()
{
    m_running = false;
    if (m_worker.joinable())
        m_worker.join();
}

template <class Clients> bool client_pool::pulse(Clients &clients)
{
    std::size_t idx = static_cast<std::size_t>(rnd(0u, m_cl_set.clients_per_mode - 1));
    auto action = static_cast<rpc_command>(rnd(0u, static_cast<uint32_t>(rpc_command::init) - 1));
    const std::vector<char> rdata = m_test_data->get_random_data();

    if (!execute_action(clients, idx, action, rdata))
        return false;

    if (rnd_bool(m_cl_set.disconnect_chance))
        clients[idx]->disconnect();
    if (rnd_bool(m_cl_set.restart_chance))
        clients[idx]->send_recv(static_cast<uint32_t>(rpc_command::restart),
                                std::vector<char>());
    return true;
}

template <class Clients>
bool client_pool::execute_action(Clients &clients,
                                 std::size_t client_idx,
                                 rpc_command action,
                                 const std::vector<char> &data)
{
    // Set random timeouts
    if (m_cl_set.random_send_timeout)
    {
        clients[client_idx]->set_send_timeout(
            std::chrono::milliseconds(rnd(0u, m_cl_set.timeout_ms)));
        clients[client_idx]->set_response_timeout(
            std::chrono::milliseconds(rnd(0u, m_cl_set.timeout_ms)));
    }
    // If flag not found, just skip it
    if (!(m_cl_set.rpc_command_flags & static_cast<uint32_t>(action)))
        return true;
    switch (action)
    {
        case rpc_command::echo:
        {
            auto recv_bytes =
                clients[client_idx]->send_recv(static_cast<uint32_t>(action), data);
            if (recv_bytes.empty() &&                //
                (m_srv_set.response_chance != 100 || //
                 m_srv_set.close_chance != 100 ||    //
                 m_cl_set.restart_chance != 0 ||     //
                 m_cl_set.timeout_ms != defaults::wait_response_ms))
                break;
            if (!compare_bytes(recv_bytes.data(),
                               recv_bytes.size(),
                               data.data(),
                               data.size()))
            {
                TEST_THROW("Error: "
                           << m_current_test_case << " rpc_command::echo "
                           << recv_bytes.size() << " != " << data.size());
                return false;
            }
            break;
        }
        case rpc_command::send_recv:
        {
            auto data_size = data.size();
            auto recv_bytes =
                clients[client_idx]->send_recv(static_cast<uint32_t>(action), data);
            if (recv_bytes.empty() &&                //
                (m_srv_set.response_chance != 100 || //
                 m_srv_set.close_chance != 100 ||    //
                 m_cl_set.restart_chance != 0 ||     //
                 m_cl_set.timeout_ms != defaults::wait_response_ms))
                break;
            auto ec = m_test_data->data_valid(recv_bytes.data(), recv_bytes.size());
            if (ec != data_state::valid)
            {
                TEST_THROW("Error: " << m_current_test_case
                                   << " rpc_command::send_recv "
                                   << recv_bytes.size() << " / " << data_size
                                   << "\ndata_state:" << static_cast<uint32_t>(ec));
                return false;
            }
            break;
        }
        case rpc_command::send:
        {
            if (!clients[client_idx]->send(static_cast<uint32_t>(action), data) && //
                m_srv_set.close_chance == 0 &&                        //
                m_cl_set.restart_chance == 0 &&                       //
                !m_cl_set.random_send_timeout &&                      //
                m_cl_set.disconnect_chance == 0)
            {
                TEST_THROW("Error: " << m_current_test_case
                                   << " rpc_command::send failed to send");
                return false;
            }
            break;
        }
        case rpc_command::send_broadcast:
        {
            auto data_size = data.size();
            auto recv_bytes =
                clients[client_idx]->send_recv(static_cast<uint32_t>(action), data);
            if (recv_bytes.empty() &&                //
                (m_srv_set.response_chance != 100 || //
                 m_srv_set.close_chance != 100 ||    //
                 m_cl_set.restart_chance != 0 ||     //
                 m_cl_set.timeout_ms != defaults::wait_response_ms))
                break;
            auto ec =
                m_test_data->data_valid(recv_bytes.data(), recv_bytes.size());
            if (ec != data_state::valid)
            {
                TEST_THROW("Error: " << m_current_test_case
                                   << " rpc_command::send_broadcast "
                                   << recv_bytes.size() << " / " << data_size
                                   << "\ndata_state:" << static_cast<uint32_t>(ec));
                return false;
            }
            break;
        }
        case rpc_command::ping:
        {
            if (!clients[client_idx]->ping() &&  //
                m_srv_set.close_chance == 0 &&   //
                m_cl_set.restart_chance == 0 &&  //
                !m_cl_set.random_send_timeout && //
                m_cl_set.timeout_ms == defaults::wait_response_ms)
            {
                TEST_THROW("Error: " << m_current_test_case
                                     << " rpc_command::ping");
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

} // namespace test_suite
} // namespace stress
} // namespace test