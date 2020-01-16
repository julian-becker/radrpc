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

#ifndef RADRPC_COMMON_IO_HEADER_HPP
#define RADRPC_COMMON_IO_HEADER_HPP

#include <cstdint>
#include <ostream>

namespace radrpc {
namespace common {

#ifdef _WIN32
#pragma pack(push, 1)
/**
 * A header which is prepend to all data transferred between
 * two hosts & allows to identify these.
 */
struct io_header
#else
/**
 * A header which is prepend to all data transferred between
 * two hosts & allows to identify these.
 */
struct __attribute__((packed)) io_header
#endif
{
    /// The id to call on the remote host.
    uint32_t call_id;

    /// Reserved field used as padding.
    uint32_t pad0;

    /// The id which will be sent back to caller.
    uint64_t result_id;

    io_header(uint32_t p_call_id, uint64_t p_result_id) :
        call_id(p_call_id),
        pad0(0),
        result_id(p_result_id)
    {
    }
};
#ifdef _WIN32
#pragma pack(pop)
#endif

inline std::ostream &operator<<(std::ostream &os, const io_header &o)
{
    os << "call_id: " << o.call_id << std::endl
       << "pad0: " << o.pad0 << std::endl
       << "result_id: " << o.result_id << std::endl;
    return os;
}

} // namespace common
} // namespace radrpc

#endif // RADRPC_COMMON_IO_HEADER_HPP