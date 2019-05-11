/*
 * MIT License

 * Copyright (c) 2019 reapler

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

#include "ssl_test_files.hpp"
#include "radrpc.hpp"




// All available remote procedure calls
enum RpcCommands
{
    RPC_ECHO_MSG,
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




    // This is just an example of many how to initialize
    // a ssl context.
    ssl::context ssl_ctx(ssl::context::sslv23);
    boost::system::error_code ec;

    // This is needed, if the key is encrypted with a password.
    ssl_ctx.set_password_callback(
        [&](std::size_t, boost::asio::ssl::context_base::password_purpose) {
            return ssl_test_files::server_key_pass;
        },
        ec);
    if (ec)
    {
        LOG("set_password_callback: " << ec.message());
        return 0;
    }

    // To load from a file, 'use_certificate_chain_file()' can be used.
    ssl_ctx.use_certificate_chain(
        boost::asio::buffer(ssl_test_files::server_certificate,
                            std::strlen(ssl_test_files::server_certificate)),
        ec);
    if (ec)
    {
        LOG("use_certificate_chain: " << ec.message());
        return 0;
    }

    // To load from a file, 'use_private_key_file()' can be used.
    ssl_ctx.use_private_key(
        boost::asio::buffer(ssl_test_files::server_key,
                            std::strlen(ssl_test_files::server_key)),
        boost::asio::ssl::context::file_format::pem,
        ec);
    if (ec)
    {
        LOG("use_private_key: " << ec.message());
        return 0;
    }

    // To load from a file, 'use_tmp_dh_file()' can be used.
    ssl_ctx.use_tmp_dh(boost::asio::buffer(ssl_test_files::dh_key,
                                           std::strlen(ssl_test_files::dh_key)),
                       ec);
    if (ec)
    {
        LOG("use_tmp_dh: " << ec.message());
        return 0;
    }




    server_config cfg;
    cfg.host_address = "0.0.0.0";
    cfg.port = 3377;
    cfg.workers = 2;
    cfg.max_sessions = 1000;
    cfg.max_handshake_bytes = 0xFFFF;
    cfg.mode = server_mode::plain | server_mode::ssl;

    server_timeout timeout;
    timeout.handshake_or_close_timeout = std::chrono::seconds(5);

    session_config default_session_cfg;
    default_session_cfg.max_transfer_bytes = 0xFFFFFFFF;
    default_session_cfg.ping_delay = std::chrono::seconds(30);

    // Let the server own ssl context.
    server srv(cfg, timeout, default_session_cfg, std::move(ssl_ctx));

    srv.bind(RPC_ECHO_MSG, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        LOG("Request from " << ctx->remote_host << " Message: " << received);
        ctx->response =
            std::vector<char>(ctx->data(), ctx->data() + ctx->size());
    });

    LOG("Start server");
    srv.start();
    srv.stop();
    LOG("Server stopped.");
}