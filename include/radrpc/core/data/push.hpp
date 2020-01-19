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

#ifndef RADRPC_CORE_DATA_PUSH_HPP
#define RADRPC_CORE_DATA_PUSH_HPP

#include <memory>
#include <vector>

#include <radrpc/common/io_header.hpp>
#include <radrpc/config.hpp>

namespace radrpc {
namespace core {
namespace data {

using io_header = radrpc::common::io_header;

/**
 * A container to send data to clients.
 */
class push : public std::enable_shared_from_this<push>
{
  public:
    /// The header of the data.
    const io_header header;

    /// The bytes to send as body.
    const std::vector<char> body;

    explicit push(const io_header &p_header, const std::vector<char> &p_body) :
        header(p_header),
        body(p_body)
    {
    }

    explicit push(const io_header &p_header, const std::vector<char> &&p_body) :
        header(p_header),
        body(std::move(p_body))
    {
    }
};

} // namespace data
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_DATA_PUSH_HPP