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

#include <cstdint>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <test/dep/cpptoml.h>

#include <test/core/defaults.hpp>
#include <test/core/log.hpp>
#include <test/core/throw.hpp>
#include <test/core/toml_helper.hpp>
#include <test/stress/config_file.hpp>
#include <test/stress/test_suite/rpc_command.hpp>

namespace test {
namespace stress {

using namespace radrpc;
using namespace test::common;
using namespace test::core;

void config_file::load_config(const boost::filesystem::path &file)
{
    try
    {
        auto data = cpptoml::parse_file(file.c_str());

        // client
        m_client_cfg.host_address = //
            get_field<std::string>(data, "client", "host_address");
        m_client_cfg.port = //
            get_field<uint32_t>(data, "client", "port");
        m_client_cfg.max_read_bytes = //
            get_field<uint32_t>(data, "client", "max_read_bytes");
        m_client_cfg.send_attempts = //
            get_field<uint32_t>(data, "client", "send_attempts");
        m_client_cfg.send_attempt_delay = std::chrono::milliseconds( //
            get_field<uint32_t>(data, "client", "send_attempt_delay_ms"));

        // server
        m_server_cfg.host_address = //
            get_field<std::string>(data, "server", "host_address");
        m_server_cfg.port = //
            get_field<uint32_t>(data, "server", "port");
        m_server_cfg.workers = //
            get_field<uint32_t>(data, "server", "workers");
        m_server_cfg.max_sessions = //
            get_field<uint32_t>(data, "server", "max_sessions");
        m_server_cfg.max_handshake_bytes = //
            get_field<uint32_t>(data, "server", "max_handshake_bytes");
        bool plain = get_field<bool>(data, "server", "mode_plain");
        bool ssl = get_field<bool>(data, "server", "mode_ssl");
        if ((!plain && !ssl) || (plain && ssl))
        {
            m_server_cfg.mode = stream_mode::plain | stream_mode::ssl;
        }
        else
        {
            if (plain)
                m_server_cfg.mode = stream_mode::plain;
            else
                m_server_cfg.mode = stream_mode::ssl;
        }

        // session
        m_session_cfg.max_transfer_bytes = //
            get_field<uint32_t>(data, "session", "max_transfer_bytes");
        m_session_cfg.ping_delay = std::chrono::milliseconds( //
            get_field<uint32_t>(data, "session", "ping_delay_ms"));

        // client_set
        m_client_set.clients_per_mode = //
            get_field<uint32_t>(data, "client_set", "clients_per_mode");
        m_client_set.max_threads = //
            get_field<uint32_t>(data, "client_set", "max_threads");
        m_client_set.min_queue_delay_ms = //
            get_field<uint32_t>(data, "client_set", "min_queue_delay_ms");
        m_client_set.max_queue_delay_ms = //
            get_field<uint32_t>(data, "client_set", "max_queue_delay_ms");
        m_client_set.disconnect_chance = //
            get_field<uint32_t>(data, "client_set", "disconnect_chance");
        m_client_set.restart_chance = //
            get_field<uint32_t>(data, "client_set", "restart_chance");
        m_client_set.runtime_secs = //
            get_field<uint32_t>(data, "client_set", "runtime_secs");
        auto rpc_command_flags =
            data->get_table("client_set")
                ->get_array_of<int64_t>("rpc_command_flags");
        for (const auto &val : *rpc_command_flags)
            m_client_set.rpc_command_flags |= val;
        m_client_set.random_send_timeout = //
            get_field<bool>(data, "client_set", "random_send_timeout");

        // server_set
        m_server_set.accept_chance = //
            get_field<uint32_t>(data, "server_set", "accept_chance");
        m_server_set.connect_chance = //
            get_field<uint32_t>(data, "server_set", "connect_chance");
        m_server_set.close_chance = //
            get_field<uint32_t>(data, "server_set", "close_chance");
        m_server_set.min_delay_ms = //
            get_field<uint32_t>(data, "server_set", "min_delay_ms");
        m_server_set.max_delay_ms = //
            get_field<uint32_t>(data, "server_set", "max_delay_ms");
        m_server_set.broadcast_delay_ms = //
            get_field<uint32_t>(data, "server_set", "broadcast_delay_ms");
        m_server_set.min_send_bytes = //
            get_field<uint32_t>(data, "server_set", "min_send_bytes");
        m_server_set.max_send_bytes = //
            get_field<uint32_t>(data, "server_set", "max_send_bytes");
    }
    catch (...)
    {
        TEST_THROW("config::load_config: Could not parse '" << file.c_str()
                                                           << "'");
    }
}

void config_file::store_config(const boost::filesystem::path file)
{
    try
    {
        std::ofstream os(file.c_str());
        if (!os.is_open())
            return;

        std::stringstream ss;
        ss << R"(
                title = "Stress Test Configuration"

                # Available rpc command flags:
                #   echo = 1
                #   send = 2
                #   send_recv = 3
                #   send_broadcast = 4
                #   ping = 5)";
        ss << "\n\n";

        // client
        {
            auto cfg = default_client_config();
            ss << "[client]"
               << "\n";
            ss << "host_address = \"" << cfg.host_address << "\"\n";
            ss << "port = " << cfg.port << "\n";
            ss << "max_read_bytes = " << cfg.max_read_bytes << "\n";
            ss << "send_attempts = " << cfg.send_attempts << "\n";
            ss << "send_attempt_delay_ms = "
               << std::chrono::duration_cast<std::chrono::milliseconds>(
                      cfg.send_attempt_delay)
                      .count()
               << "\n";
            ss << "\n";
        }

        // server
        {
            auto cfg = default_server_config();
            ss << "[server]"
               << "\n";
            ss << "host_address = \"" << cfg.host_address << "\"\n";
            ss << "port = " << cfg.port << "\n";
            ss << "workers = " << cfg.workers << "\n";
            ss << "max_sessions = " << cfg.max_sessions << "\n";
            ss << "max_handshake_bytes = " << cfg.max_handshake_bytes << "\n";
            ss << "mode_plain = "
               << ((cfg.mode & stream_mode::plain) ? "true" : "false") << "\n";
            ss << "mode_ssl = "
               << ((cfg.mode & stream_mode::ssl) ? "true" : "false") << "\n";
            ss << "\n";
        }

        // session
        {
            auto cfg = default_session_config();
            ss << "[session]"
               << "\n";
            ss << "max_transfer_bytes = " << cfg.max_transfer_bytes << "\n";
            ss << "ping_delay_ms = "
               << std::chrono::duration_cast<std::chrono::milliseconds>(
                      cfg.ping_delay)
                      .count()
               << "\n";
            ss << "\n";
        }

        // client_set
        {
            test_suite::client_set cfg;
            ss << "[client_set]"
               << "\n";
            ss << "clients_per_mode = " << cfg.clients_per_mode << "\n";
            ss << "max_threads = " << cfg.max_threads << "\n";
            ss << "min_queue_delay_ms = " << cfg.min_queue_delay_ms << "\n";
            ss << "max_queue_delay_ms = " << cfg.max_queue_delay_ms << "\n";
            ss << "disconnect_chance = " << cfg.disconnect_chance << "\n";
            ss << "restart_chance = " << cfg.restart_chance << "\n";
            ss << "runtime_secs = " << cfg.runtime_secs << "\n";
            ss << "rpc_command_flags = "
               << "[1, 2, 3, 4, 5]"
               << "\n";
            ss << "random_send_timeout = "
               << (cfg.random_send_timeout ? "true" : "false") << "\n";
            ss << "\n";
        }

        // server_set
        {
            test_suite::server_set cfg;
            ss << "[server_set]"
               << "\n";
            ss << "accept_chance = " << cfg.accept_chance << "\n";
            ss << "connect_chance = " << cfg.connect_chance << "\n";
            ss << "response_chance = " << cfg.response_chance << "\n";
            ss << "close_chance = " << cfg.close_chance << "\n";
            ss << "min_delay_ms = " << cfg.min_delay_ms << "\n";
            ss << "max_delay_ms = " << cfg.max_delay_ms << "\n";
            ss << "broadcast_delay_ms = " << cfg.broadcast_delay_ms << "\n";
            ss << "min_send_bytes = " << cfg.min_send_bytes << "\n";
            ss << "max_send_bytes = " << cfg.max_send_bytes << "\n";
            ss << "\n";
        }

        auto str = ss.str();
        boost::replace_all(str, "                ", "");
        boost::replace_all(str, "	", "");
        os << str;
        os.close();
    }
    catch (...)
    {
        TEST_THROW("config::store_config: Could not store '" << file.c_str()
                                                            << "'");
    }
}

config_file::config_file(const boost::filesystem::path file) :
    m_client_cfg{},
    m_server_cfg{},
    m_session_cfg{},
    m_client_set{},
    m_server_set{}
{
    if (!boost::filesystem::exists(file))
    {
        store_config(file);
        if (!boost::filesystem::exists(file))
            TEST_THROW("+config: Could not create file '" << file.c_str()
                                                         << "'");
    }
    load_config(file);
}

client_config config_file::get_client_config() { return m_client_cfg; }

server_config config_file::get_server_config() { return m_server_cfg; }

session_config config_file::get_session_config() { return m_session_cfg; }

test_suite::client_set config_file::get_client_set() { return m_client_set; }

test_suite::server_set config_file::get_server_set() { return m_server_set; }

} // namespace stress
} // namespace test