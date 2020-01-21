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

#ifndef RADRPC_TEST_CORE_SLEEP_HPP
#define RADRPC_TEST_CORE_SLEEP_HPP

#include <chrono>
#include <thread>

#include <test/core/random.hpp>

namespace test {
namespace core {

using duration = std::chrono::steady_clock::duration;

template <typename T> inline void sleep_ms(T ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

template <typename T> inline void sleep_ms_rnd(T min_ms, T max_ms)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(thread_random::get(min_ms, max_ms))));
}

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_SLEEP_HPP