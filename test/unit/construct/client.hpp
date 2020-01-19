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

#ifndef RADRPC_TEST_UNIT_CONSTRUCT_CLIENT_HPP
#define RADRPC_TEST_UNIT_CONSTRUCT_CLIENT_HPP

#include <radrpc/radrpc.hpp>

#include <test/core/defaults.hpp>

namespace test {
namespace unit {
namespace construct {

using namespace radrpc;
using namespace test::core;

std::unique_ptr<client::plain> create_plain_client(
    const client_config &cfg = default_client_config(),
    const client_timeout &timeout = default_client_timeout());

#ifdef RADRPC_SSL_SUPPORT

std::unique_ptr<client::ssl> create_ssl_client(
    const client_config &cfg = default_client_config(),
    const client_timeout &timeout = default_client_timeout());

#endif

} // namespace construct
} // namespace unit
} // namespace test

#endif // RADRPC_TEST_UNIT_CONSTRUCT_CLIENT_HPP