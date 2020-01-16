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

#include "radrpc.hpp"




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

    server_config cfg;
    cfg.host_address = "0.0.0.0";
    cfg.port = 3377;
    cfg.workers = 2;
    cfg.max_sessions = 1000;
    cfg.max_handshake_bytes = 0xFFFF;
    cfg.mode = stream_mode::plain;

    server_timeout timeout;
    timeout.handshake_or_close_timeout = std::chrono::seconds(5);

    session_config default_session_cfg;
    default_session_cfg.max_transfer_bytes = 0xFFFFFFFF;
    default_session_cfg.ping_delay = std::chrono::seconds(30);

    server srv(cfg, timeout, default_session_cfg);

    srv.bind(MY_RPC_ECHO_MSG, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        LOG("Request from " << ctx->remote_host << " Message: " << received);
        // Assign the response.
        ctx->response =
            std::vector<char>(ctx->data(), ctx->data() + ctx->size());
    });

    LOG("Start server");
    srv.start(); // Blocks until stopped, or got a signal with SIGINT or
                 // SIGTERM
    srv.stop();
    LOG("Server stopped.");
}