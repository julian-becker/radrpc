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

#include "default_config.hpp"
#ifdef RADRPC_SSL_SUPPORT

ssl::context
client_ssl(const std::string &certificate)
{
    ssl::context ctx(ssl::context::sslv23);
    boost::system::error_code ec;
    ctx.add_certificate_authority(
        boost::asio::buffer(certificate.data(),
                            certificate.size()),
        ec);
    if (ec)
        throw std::runtime_error("add_certificate_authority:" + ec.message());
    return ctx;
}

std::unique_ptr<client<client_mode::ssl>>
ssl_create_client(const client_config &cfg,
                  const client_timeout &timeout)
{
    auto ssl_ctx = client_ssl(ssl_test_files::client_certificate);
    auto cl = new client<client_mode::ssl>(cfg, timeout, std::move(ssl_ctx));
    return std::unique_ptr<client<client_mode::ssl>>(cl);
}

bool ssl_connect_send_recv()
{
    auto cl = ssl_create_client();
    cl->connect();
    return !cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()).empty();
}

#endif