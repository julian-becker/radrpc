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

#ifndef RADRPC_CORE_ENDIAN_HPP
#define RADRPC_CORE_ENDIAN_HPP

#include <cstdint>

namespace radrpc {
namespace core {
namespace endian {

// https://github.com/fraillt/bitsery/blob/master/include/bitsery/common.h
// https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
// https://code.woboq.org/userspace/glibc/inet/htonl.c.html

struct endianness_test_data
{
    static constexpr uint32_t sample4_bytes = 0x01020304;
    static constexpr uint8_t sample1st_byte = static_cast<const uint8_t &>(sample4_bytes);
};

static_assert(endianness_test_data::sample1st_byte == 0x04 ||
                  endianness_test_data::sample1st_byte == 0x01,
              "system must be either little or big endian");

constexpr bool is_little_endian()
{
    return endianness_test_data::sample1st_byte == 0x04;
}

inline uint64_t htonl_uint64(uint64_t x)
{
    if (is_little_endian())
    {
        x = ((x << 8) & 0xFF00FF00FF00FF00ULL) |
              ((x >> 8) & 0x00FF00FF00FF00FFULL);
        x = ((x << 16) & 0xFFFF0000FFFF0000ULL) |
              ((x >> 16) & 0x0000FFFF0000FFFFULL);
        return (x << 32) | (x >> 32);
    }
    else
    {
        return x;
    }
}

inline uint64_t ntohl_uint64(uint64_t x) { return htonl_uint64(x); }

} // namespace endian
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_ENDIAN_HPP