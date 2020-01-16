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

#include <test/core/ssl_context.hpp>
#include <test/unit/construct/client.hpp>

namespace test {
namespace unit {
namespace construct {

std::unique_ptr<client::plain> create_plain_client(
    const client_config &cfg,
    const client_timeout &timeout)
{
    auto cl = new client::plain(cfg, timeout);
    return std::unique_ptr<client::plain>(cl);
}

#ifdef RADRPC_SSL_SUPPORT

std::unique_ptr<client::ssl> create_ssl_client(const client_config &cfg,
                                               const client_timeout &timeout)
{
    auto ssl_ctx = client_ssl_context(ssl_test_files::client_certificate);
    auto cl = new client::ssl(cfg, timeout, std::move(ssl_ctx));
    return std::unique_ptr<client::ssl>(cl);
}

#endif

} // namespace construct
} // namespace unit
} // namespace test