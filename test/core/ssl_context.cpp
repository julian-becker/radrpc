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

namespace test {
namespace core {

#ifdef RADRPC_SSL_SUPPORT

ssl::context client_ssl_context(const std::string &certificate)
{
    ssl::context ctx(ssl::context::sslv23);
    boost::system::error_code ec;
    ctx.add_certificate_authority(
        boost::asio::buffer(certificate.data(), certificate.size()), ec);
    if (ec)
        throw std::runtime_error("add_certificate_authority:" + ec.message());
    return ctx;
}

ssl::context server_ssl_context(const std::string &certificate,
                                const std::string &private_key,
                                const std::string &private_key_pass,
                                const std::string &dh_key)
{
    ssl::context ctx(ssl::context::sslv23);
    boost::system::error_code ec;

    ctx.set_password_callback(
        [&](std::size_t, boost::asio::ssl::context_base::password_purpose) {
            return private_key_pass;
        },
        ec);
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

#endif

} // namespace unit
} // namespace test