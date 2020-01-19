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

#ifndef RADRPC_TEST_CORE_THROW_HPP
#define RADRPC_TEST_CORE_THROW_HPP

#include <sstream>

#include <test/core/log.hpp>

namespace test {
namespace core {

#define TEST_THROW(MSG)                                                        \
    do                                                                         \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << MSG << "\nIn " << __FUNCTION__ << " " << TEST_FILENAME << ":"    \
           << __LINE__;                                                        \
        TEST_INFO_FAIL(ss.str().c_str());                                      \
        std::raise(SIGTERM);                                                   \
    } while (false)

#define TEST_ASSERT(BOOLEAN)                                                   \
    do                                                                         \
    {                                                                          \
        if (!(BOOLEAN))                                                        \
        {                                                                      \
            std::stringstream ss;                                              \
            ss << "In " << __FUNCTION__ << " " << TEST_FILENAME << ":"         \
               << __LINE__;                                                    \
            throw std::runtime_error(ss.str().c_str());                        \
        }                                                                      \
    } while (false)

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_THROW_HPP