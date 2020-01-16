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

#ifndef RADRPC_TEST_CORE_TIME_HPP
#define RADRPC_TEST_CORE_TIME_HPP

#include <chrono>
#include <memory>

namespace test {
namespace core {

template <typename F, typename... Args>
inline auto get_time(F &&f, Args &&... args)
{
    using namespace std::chrono;

    auto t0 = high_resolution_clock::now();
    std::forward<F>(f)(std::forward<Args>(args)...);
    auto t1 = high_resolution_clock::now();
    return t1 - t0;
}

#define RAD_REQUIRE_TIME(e, t) REQUIRE((get_time([&] { e; }) > t))

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_TIME_HPP