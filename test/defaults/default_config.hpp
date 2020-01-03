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

#ifndef RADRPC_TEST_DEFAULT_CONFIG_HPP
#define RADRPC_TEST_DEFAULT_CONFIG_HPP
#define RADRPC_TEST_BINARY
#include <vector>

#include "radrpc.hpp"
#include "ssl_test_files.hpp"

using namespace radrpc;
using duration = std::chrono::steady_clock::duration;




enum UnitRpcCommands
{
    UNIT_RPC_STOP,
    UNIT_RPC_SEND,
    UNIT_RPC_SEND_RECV,
    UNIT_RPC_CLOSE,
    UNIT_RPC_SERVER_MSG,
};




#ifdef BUILD_VALGRIND

const int sleep_low_delay_ms = 60;
const int sleep_high_delay_ms = 800;

inline server_timeout default_server_timeout()
{
    server_timeout cfg;
    cfg.handshake_or_close_timeout = std::chrono::milliseconds(1600);
    return cfg;
}

inline client_timeout default_client_timeout()
{
    radrpc::client_timeout cfg;
    cfg.handshake_timeout = std::chrono::milliseconds(1600);
    cfg.send_timeout = std::chrono::milliseconds(1000);
    cfg.response_timeout = std::chrono::milliseconds(1000);
    return cfg;
}

#else

const int sleep_low_delay_ms = 20;
const int sleep_high_delay_ms = 400;

inline server_timeout default_server_timeout()
{
    server_timeout cfg;
    cfg.handshake_or_close_timeout = std::chrono::milliseconds(400);
    return cfg;
}

inline client_timeout default_client_timeout()
{
    radrpc::client_timeout cfg;
    cfg.handshake_timeout = std::chrono::milliseconds(400);
    cfg.send_timeout = std::chrono::milliseconds(200);
    cfg.response_timeout = std::chrono::milliseconds(200);
    return cfg;
}

#endif

inline server_config default_server_config()
{
    server_config cfg;
    cfg.host_address = "0.0.0.0";
    cfg.port = 3377;
    cfg.workers = 2;
    cfg.max_sessions = 20;
    cfg.max_handshake_bytes = 1024 * 200;
    cfg.mode = server_mode::plain | server_mode::ssl;
    return cfg;
}

inline session_config default_session_config()
{
    session_config cfg;
    cfg.max_transfer_bytes = 1024 * 200;
    cfg.ping_delay = std::chrono::milliseconds(500);
    return cfg;
}

inline client_config default_client_config()
{
    client_config cfg;
    cfg.host_address = "127.0.0.1";
    cfg.port = 3377;
    cfg.max_read_bytes = 0xFFFFFFFF;
    cfg.send_attempts = 0;
    cfg.send_attempt_delay = duration::zero();
    return cfg;
}

inline void sleep_ms(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

template <typename T> std::vector<T> vector_of_object(const T &obj)
{
    return std::vector<T>();
}




std::unique_ptr<client<client_mode::plain>>
plain_create_client(const client_config &cfg = default_client_config(),
                    const client_timeout &timeout = default_client_timeout());

bool plain_connect_send_recv();

std::unique_ptr<server> plain_create_server(
    const server_config &srv_cfg = default_server_config(),
    const server_timeout &timeout = default_server_timeout(),
    const session_config &session_cfg = default_session_config());

#ifdef RADRPC_SSL_SUPPORT

ssl::context
client_ssl(const std::string &certificate = ssl_test_files::client_certificate);

ssl::context server_ssl(
    const std::string &certificate = ssl_test_files::server_certificate,
    const std::string &private_key = ssl_test_files::server_key,
    const std::string &private_key_pass = ssl_test_files::server_key_pass,
    const std::string &dh_key = ssl_test_files::dh_key);

std::unique_ptr<client<client_mode::ssl>>
ssl_create_client(const client_config &cfg = default_client_config(),
                  const client_timeout &timeout = default_client_timeout());

std::unique_ptr<server>
ssl_create_server(const server_config &srv_cfg = default_server_config(),
                  const server_timeout &timeout = default_server_timeout(),
                  const session_config &session_cfg = default_session_config());

bool ssl_connect_send_recv();

#endif

#endif // RADRPC_TEST_DEFAULT_CONFIG_HPP