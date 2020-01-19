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

#ifndef TEST_TEST_CORE_LOG_HPP
#define TEST_TEST_CORE_LOG_HPP

#include <cstring>
#include <iostream>
#include <iomanip>
#include <mutex>

#include <test/dep/termcolor.hpp>

namespace test {
namespace core {

inline std::mutex &log_mtx()
{
    static std::mutex m_log_mtx;
    return m_log_mtx;
}

#ifdef _WIN32
#define TEST_FILENAME                                                          \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define TEST_FILENAME                                                          \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define TEST_INFO(str)                                                         \
    do                                                                         \
    {                                                                          \
        log_mtx().lock();                                                      \
        std::cout << "[" << termcolor::cyan << "INFO" << termcolor::reset      \
                  << "] " << str << std::endl;                                 \
        log_mtx().unlock();                                                    \
    } while (false)

#define TEST_DINFO(str)                                                        \
    do                                                                         \
    {                                                                          \
        log_mtx().lock();                                                      \
        std::cout << "[" << termcolor::cyan << "INFO" << termcolor::reset      \
                  << "] " << __FUNCTION__ << " " << TEST_FILENAME << ":"       \
                  << __LINE__ << "\t" << str << std::endl;                     \
        log_mtx().unlock();                                                    \
    } while (false)

#define TEST_INFO_FAIL(str)                                                    \
    do                                                                         \
    {                                                                          \
        log_mtx().lock();                                                      \
        std::cout << "[" << termcolor::red << "FAILED" << termcolor::reset     \
                  << "] " << str << std::endl;                                 \
        log_mtx().unlock();                                                    \
    } while (false)
inline void rad_log_lock() { log_mtx().lock(); }
inline void rad_log_unlock() { log_mtx().unlock(); }

} // namespace core
} // namespace test

#endif // TEST_TEST_CORE_LOG_HPP