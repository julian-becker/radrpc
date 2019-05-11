/*
 * MIT License

 * Copyright (c) 2019 reapler

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

#ifndef RADRPC_DEBUG_HPP
#define RADRPC_DEBUG_HPP

#include <iostream>
#include <iomanip>
#include <mutex>

#ifdef _WIN32
#define RADRPC_FILENAME                                                        \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define RADRPC_FILENAME                                                        \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define RADRPC_THROW(MSG)                                                      \
    do                                                                         \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << MSG << "\n           In " << __FUNCTION__ << " "                 \
           << RADRPC_FILENAME << ":" << __LINE__;                              \
        throw std::runtime_error(ss.str().c_str());                            \
    } while (false)

#define RADRPC_ASSERT(BOOLEAN)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(BOOLEAN))                                                        \
        {                                                                      \
            std::stringstream ss;                                              \
            ss << "In " << __FUNCTION__ << " " << RADRPC_FILENAME << ":"       \
               << __LINE__;                                                    \
            throw std::runtime_error(ss.str().c_str());                        \
        }                                                                      \
    } while (false)

#if defined RADRPC_LOGGING
namespace radrpc {
namespace debug {
inline std::mutex &log_mtx()
{
    static std::mutex m_log_mtx;
    return m_log_mtx;
}
} // namespace debug
} // namespace radrpc
#define RADRPC_LOG(str)                                                        \
    do                                                                         \
    {                                                                          \
        debug::log_mtx().lock();                                               \
        std::cout << std::fixed << std::setprecision(5) << str << std::endl;   \
        debug::log_mtx().unlock();                                             \
    } while (false)

#define RADRPC_DLOG(str)                                                       \
    do                                                                         \
    {                                                                          \
        debug::log_mtx().lock();                                               \
        std::cout << std::fixed << std::setprecision(5) << "[DEBUG] " << str   \
                  << std::endl;                                                \
        debug::log_mtx().unlock();                                             \
    } while (false)
#else
#define RADRPC_LOG(str)                                                        \
    do                                                                         \
    {                                                                          \
    } while (false)
#define RADRPC_DLOG(str)                                                       \
    do                                                                         \
    {                                                                          \
    } while (false)
#endif

#define WHILE_TIMEOUT(EXPRESSION, TIMEOUT_SECS)                                \
    auto start_measure = std::chrono::steady_clock::now();                     \
    while ((EXPRESSION) &&                                                     \
           !(timeout = !(std::chrono::duration_cast<std::chrono::seconds>(     \
                             std::chrono::steady_clock::now() - start_measure) \
                             .count() < TIMEOUT_SECS)))

#endif // RADRPC_DEBUG_HPP