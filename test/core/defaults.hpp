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

#ifndef RADRPC_TEST_CORE_DEFAULTS_HPP
#define RADRPC_TEST_CORE_DEFAULTS_HPP

#include <radrpc/radrpc.hpp>

namespace test {
namespace core {

using namespace radrpc;

class defaults
{
  public:
    static const uint32_t wait_response_ms;

    static const int sleep_low_delay_ms;
    static const int sleep_high_delay_ms;
    static const int handshake_or_close_timeout_ms;
    static const int send_timeout_ms;
    static const int response_timeout_ms;
};

client_config default_client_config();

server_config default_server_config();

session_config default_session_config();

client_timeout default_client_timeout();

server_timeout default_server_timeout();

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_DEFAULTS_HPP