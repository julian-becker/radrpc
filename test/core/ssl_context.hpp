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

#ifndef RADRPC_TEST_CORE_SSL_CONTEXT_HPP
#define RADRPC_TEST_CORE_SSL_CONTEXT_HPP

#include "radrpc.hpp"

#include <test/common/ssl_test_files.hpp>

namespace test {
namespace core {

using namespace radrpc;
using namespace test::common;

#ifdef RADRPC_SSL_SUPPORT

ssl::context client_ssl_context(
    const std::string &certificate = ssl_test_files::client_certificate);

ssl::context server_ssl_context(
    const std::string &certificate = ssl_test_files::server_certificate,
    const std::string &private_key = ssl_test_files::server_key,
    const std::string &private_key_pass = ssl_test_files::server_key_pass,
    const std::string &dh_key = ssl_test_files::dh_key);

#endif

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_SSL_CONTEXT_HPP