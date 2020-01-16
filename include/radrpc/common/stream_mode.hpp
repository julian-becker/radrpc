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

#ifndef RADRPC_COMMON_STREAM_MODE_HPP
#define RADRPC_COMMON_STREAM_MODE_HPP

#include <string>
#include <cstdint>

namespace radrpc {
namespace common {

enum class stream_mode : unsigned char
{
    plain = 1 << 0,
    ssl = 1 << 1,
};

inline stream_mode operator|(stream_mode lhs, stream_mode rhs)
{
    return static_cast<stream_mode>(
        static_cast<std::underlying_type<stream_mode>::type>(lhs) |
        static_cast<std::underlying_type<stream_mode>::type>(rhs));
}

inline bool operator&(stream_mode lhs, stream_mode rhs)
{
    return static_cast<bool>(
        static_cast<std::underlying_type<stream_mode>::type>(lhs) &
        static_cast<std::underlying_type<stream_mode>::type>(rhs));
}

} // namespace common

using stream_mode = radrpc::common::stream_mode;

} // namespace radrpc

#endif // RADRPC_COMMON_STREAM_MODE_HPP