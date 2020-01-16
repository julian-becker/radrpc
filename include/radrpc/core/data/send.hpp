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

#ifndef RADRPC_CORE_DATA_SEND_HPP
#define RADRPC_CORE_DATA_SEND_HPP

#include <cstdint>
#include <functional>
#include <future>
#include <memory>

#include <radrpc/common/io_header.hpp>

namespace radrpc {
namespace core {
namespace data {

using io_header = radrpc::common::io_header;

/**
 * A container to write data to server with an
 * option to notify the caller.
 */
class send
{
  public:
    /// The callback to notify waiting functions.
    std::weak_ptr<std::promise<bool>> result_callback;

    /// The pointer of the body.
    const char *body_ptr;

    /// The size of the body.
    std::size_t body_size;

    /// The header to send.
    io_header header;

    /**
     * @param p_call_id The id to call on remote host.
     * @param p_result_id The id which will be sent back to the caller
     * @param p_result_callback The weak callback to notify waiting functions.
     * @param p_body_ptr The pointer of the body to send.
     * @param p_body_size The size of the body to send.
     */
    send(uint32_t p_call_id,
         uint64_t p_result_id,
         std::weak_ptr<std::promise<bool>> p_result_callback,
         const char *p_body_ptr,
         std::size_t p_body_size) :
        result_callback(std::move(p_result_callback)),
        body_ptr(p_body_ptr),
        body_size(p_body_size),
        header(p_call_id, p_result_id)
    {
    }
};

} // namespace data
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_DATA_SEND_HPP