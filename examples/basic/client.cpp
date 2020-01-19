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
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <radrpc/radrpc.hpp>



// User defined remote procedure calls
enum MyRpcCommands
{
    MY_RPC_ECHO_MSG,
};

inline std::mutex &log_mtx()
{
    static std::mutex m_log_mtx;
    return m_log_mtx;
}

// Small logger utility
#define LOG(str)                                                               \
    do                                                                         \
    {                                                                          \
        log_mtx().lock();                                                      \
        std::cout << std::fixed << std::setprecision(5) << str << std::endl;   \
        log_mtx().unlock();                                                    \
    } while (false)




int main()
{
    using namespace radrpc;

    client_config cfg;
    cfg.host_address = "127.0.0.1";
    cfg.port = 3377;
    cfg.max_read_bytes = 0xFFFFFFFF;
    cfg.send_attempts = 0;
    cfg.send_attempt_delay = duration::zero();

    client_timeout timeout;
    timeout.handshake_timeout = std::chrono::seconds(5);
    timeout.send_timeout = std::chrono::seconds(1);
    timeout.response_timeout = std::chrono::seconds(1);

    client::plain cl(cfg, timeout);

    LOG("Press enter to connect.");
    std::cin.get();
    // Connect manually.
    // if not used, 'send()' / 'send_recv()'
    // will consume an attempt.
    cl.connect();

    LOG("Please enter your echo message:");
    std::string msg;
    std::getline(std::cin, msg);
    std::vector<char> msg_bytes(msg.begin(), msg.end());
    auto response_bytes = cl.send_recv(MY_RPC_ECHO_MSG, msg_bytes);
    if (!response_bytes.empty())
    {
        std::string response(response_bytes.data(),
                             response_bytes.data() + response_bytes.size());
        LOG("Received message from server: " << response);
    }

    LOG("Press enter to disconnect.");
    std::cin.get();
    cl.disconnect();

    return 0;
}