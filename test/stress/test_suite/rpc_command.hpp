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

#ifndef RADRPC_TEST_COMMON_RPC_COMMAND_HPP
#define RADRPC_TEST_COMMON_RPC_COMMAND_HPP

#include <cstdint>
#include <string>

namespace test {
namespace common {

enum class rpc_command : uint32_t
{
    echo = 1 << 0,
    send = 1 << 1,
    send_recv = 1 << 2,
    send_broadcast = 1 << 3,
    ping = 1 << 4,
    init = 1 << 5,
    restart = 1 << 6,
    shutdown = 1 << 7,
    server_msg = 1 << 8,
};

inline rpc_command operator|(rpc_command lhs, rpc_command rhs)
{
    return static_cast<rpc_command>(
        static_cast<std::underlying_type<rpc_command>::type>(lhs) |
        static_cast<std::underlying_type<rpc_command>::type>(rhs));
}

inline bool operator&(rpc_command lhs, rpc_command rhs)
{
    return static_cast<bool>(
        static_cast<std::underlying_type<rpc_command>::type>(lhs) &
        static_cast<std::underlying_type<rpc_command>::type>(rhs));
}

} // namespace common

using rpc_command = test::common::rpc_command;

} // namespace test

#endif // RADRPC_TEST_COMMON_RPC_COMMAND_HPP