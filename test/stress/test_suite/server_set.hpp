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

#ifndef RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP
#define RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP

#include <cstdint>
#include <ostream>

namespace test {
namespace stress {
namespace test_suite {

#ifdef _WIN32
#pragma pack(push, 1)
struct server_set
#else
struct __attribute__((packed)) server_set
#endif
{
    uint32_t accept_chance;
    uint32_t connect_chance;
    uint32_t response_chance;
    uint32_t close_chance;
    uint32_t min_delay_ms;
    uint32_t max_delay_ms;
    uint32_t broadcast_delay_ms;
    uint32_t test_entries;

    server_set() :
        accept_chance(100),
        connect_chance(100),
        response_chance(100),
        close_chance(0),
        min_delay_ms(20),
        max_delay_ms(50),
        broadcast_delay_ms(1000),
        test_entries(1500)
    {
    }
};
#ifdef _WIN32
#pragma pack(pop)
#endif

inline std::ostream &operator<<(std::ostream &os, const server_set &o)
{
    os << "accept_chance: " << o.accept_chance << std::endl
       << "connect_chance: " << o.connect_chance << std::endl
       << "response_chance: " << o.response_chance << std::endl
       << "close_chance: " << o.close_chance << std::endl
       << "min_delay_ms: " << o.min_delay_ms << std::endl
       << "max_delay_ms: " << o.max_delay_ms << std::endl
       << "broadcast_delay_ms: " << o.broadcast_delay_ms << std::endl
       << "test_entries: " << o.test_entries << std::endl;
    return os;
}

} // namespace test_suite
} // namespace stress
} // namespace test

#endif // RADRPC_TEST_STRESS_TEST_SUITE_SERVER_SET_HPP