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

#include <chrono>
#include <memory>

#include "radrpc.hpp"

#include <test/core/defaults.hpp>

namespace test {
namespace core {

const int defaults::wait_response_ms = 4000;

#ifdef BUILD_VALGRIND

const int defaults::sleep_low_delay_ms = 60;
const int defaults::sleep_high_delay_ms = 800;
const int defaults::handshake_or_close_timeout_ms = 1600;
const int defaults::send_timeout_ms = 1000;
const int defaults::response_timeout_ms = 1000;

#else

const int defaults::sleep_low_delay_ms = 20;
const int defaults::sleep_high_delay_ms = 400;
const int defaults::handshake_or_close_timeout_ms = 400;
const int defaults::send_timeout_ms = 200;
const int defaults::response_timeout_ms = 200;

#endif

client_config default_client_config()
{
    client_config cfg;
    cfg.host_address = "127.0.0.1";
    cfg.port = 3377;
    cfg.max_read_bytes = 0xFFFFFFFF;
    cfg.send_attempts = 0;
    cfg.send_attempt_delay = duration::zero();
    return cfg;
}

server_config default_server_config()
{
    server_config cfg;
    cfg.host_address = "0.0.0.0";
    cfg.port = 3377;
    cfg.workers = 2;
    cfg.max_sessions = 20;
    cfg.max_handshake_bytes = 1024 * 200;
    cfg.mode = stream_mode::plain | stream_mode::ssl;
    return cfg;
}

session_config default_session_config()
{
    session_config cfg;
    cfg.max_transfer_bytes = 1024 * 200;
    cfg.ping_delay = std::chrono::milliseconds(500);
    return cfg;
}

client_timeout default_client_timeout()
{
    radrpc::client_timeout cfg;
    cfg.handshake_timeout =
        std::chrono::milliseconds(defaults::handshake_or_close_timeout_ms);
    cfg.send_timeout = std::chrono::milliseconds(defaults::send_timeout_ms);
    cfg.response_timeout =
        std::chrono::milliseconds(defaults::response_timeout_ms);
    return cfg;
}

server_timeout default_server_timeout()
{
    server_timeout cfg;
    cfg.handshake_or_close_timeout =
        std::chrono::milliseconds(defaults::handshake_or_close_timeout_ms);
    return cfg;
}

} // namespace core
} // namespace test