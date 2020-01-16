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

#ifndef RADRPC_TEST_CORE_LOG_HPP
#define RADRPC_TEST_CORE_LOG_HPP

#include <mutex>

#include <test/dep/termcolor.hpp>

namespace test {
namespace core {

inline std::mutex &rad_log_mtx()
{
    static std::mutex m_rad_log_mtx;
    return m_rad_log_mtx;
}
#define RAD_INFO(str)                                                          \
    do                                                                         \
    {                                                                          \
        rad_log_mtx().lock();                                                 \
        std::cout << "[" << termcolor::cyan << "INFO" << termcolor::reset      \
                  << "] " << str << std::endl;                                 \
        rad_log_mtx().unlock();                                               \
    } while (false)
#define RAD_FAIL(str)                                                          \
    do                                                                         \
    {                                                                          \
        rad_log_mtx().lock();                                                 \
        std::cout << "[" << termcolor::red << "FAILED" << termcolor::reset     \
                  << "] " << str << std::endl;                                 \
        rad_log_mtx().unlock();                                               \
    } while (false)
inline void rad_log_lock() { rad_log_mtx().lock(); }
inline void rad_log_unlock() { rad_log_mtx().unlock(); }

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_LOG_HPP