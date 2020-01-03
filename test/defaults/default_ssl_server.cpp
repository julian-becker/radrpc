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

#include "default_config.hpp"
#ifdef RADRPC_SSL_SUPPORT

ssl::context server_ssl(
    const std::string &certificate,
    const std::string &private_key,
    const std::string &private_key_pass,
    const std::string &dh_key)
{
    ssl::context ctx(ssl::context::sslv23);
    boost::system::error_code ec;

    ctx.set_password_callback(
        [&](std::size_t, boost::asio::ssl::context_base::password_purpose) {
            return private_key_pass;
        }, ec);
    if (ec)
        throw std::runtime_error("set_password_callback:" + ec.message());

    ctx.use_certificate_chain(
        boost::asio::buffer(certificate.data(), certificate.size()), ec);
    if (ec)
        throw std::runtime_error("use_certificate_chain:" + ec.message());

    ctx.use_private_key(
        boost::asio::buffer(private_key.data(), private_key.size()),
        boost::asio::ssl::context::file_format::pem,
        ec);
    if (ec)
        throw std::runtime_error("use_private_key:" + ec.message());

    ctx.use_tmp_dh(boost::asio::buffer(dh_key.data(), dh_key.size()), ec);
    if (ec)
        throw std::runtime_error("use_tmp_dh:" + ec.message());

    return ctx;
}

std::unique_ptr<server>
ssl_create_server(const server_config &srv_cfg,
                  const server_timeout &timeout,
                  const session_config &session_cfg)
{
    auto ssl_ctx = server_ssl(ssl_test_files::server_certificate,
                              ssl_test_files::server_key,
                              ssl_test_files::server_key_pass,
                              ssl_test_files::dh_key);
    auto srv = new server(srv_cfg, timeout, session_cfg, std::move(ssl_ctx));
    // Capture also the server instance
    srv->bind(UNIT_RPC_STOP, [&, srv](session_context *ctx) { srv->stop(); });
    srv->bind(UNIT_RPC_CLOSE, [&](session_context *ctx) { ctx->close(); });
    srv->bind(UNIT_RPC_SEND,
              [&](session_context *ctx) { ctx->clear_buffer(); });
    srv->bind(UNIT_RPC_SEND_RECV, [&](session_context *ctx) {
        ctx->response.push_back(0x0);
        if (ctx->size() != 0 && ctx->data() == nullptr)
            ctx->response.push_back(0x0);
    });
    return std::unique_ptr<server>(srv);
}

#endif