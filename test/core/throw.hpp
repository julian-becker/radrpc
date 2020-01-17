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

#ifdef _WIN32
#define RAD_FILENAME                                                           \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define RAD_FILENAME                                                           \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define RAD_THROW(MSG)                                                         \
    do                                                                         \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << MSG << "\nIn " << __FUNCTION__ << " " << RAD_FILENAME << ":"     \
           << __LINE__;                                                        \
        TEST_INFO_FAIL(ss.str().c_str());                                               \
        std::raise(SIGTERM);                                                   \
    } while (false)

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_THROW_HPP