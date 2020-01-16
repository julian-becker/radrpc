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

#ifndef RADRPC_COMMON_RECEIVE_BUFFER_HPP
#define RADRPC_COMMON_RECEIVE_BUFFER_HPP

#include <boost/beast/core/flat_buffer.hpp>

namespace radrpc {
namespace common {

/**
 * @brief 
 * 
 */
class receive_buffer : public boost::beast::flat_buffer
{
  public:
    receive_buffer();

    boost::beast::flat_buffer &base();

    operator bool();

    /**
     * Gets the pointer of the buffer.
     */
    const char *data();

    /**
     * Gets the size of the buffer.
     */
    std::size_t size();

    /**
     * Clears the buffer.
     */
    void clear();

    /**
     * Check whether the buffer is empty.
     */
    bool empty();
};

} // namespace core

using receive_buffer = radrpc::common::receive_buffer;

} // namespace radrpc

#endif // RADRPC_CORE_RECEIVE_BUFFER_HPP