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

#include <boost/beast/core/buffers_prefix.hpp>

#include <radrpc/common/receive_buffer.hpp>

namespace radrpc {
namespace common {

receive_buffer::receive_buffer() = default;

boost::beast::flat_buffer &receive_buffer::base()
{
    return static_cast<boost::beast::flat_buffer &>(*this);
}

receive_buffer::operator bool() { return !empty(); }

const char *receive_buffer::data()
{
    return reinterpret_cast<const char *>(
        boost::beast::buffers_front(base().data()).data());
}

std::size_t receive_buffer::size()
{
    return boost::beast::buffers_front(base().data()).size();
}

void receive_buffer::clear() { base().consume(base().size()); }

bool receive_buffer::empty()
{
    return boost::beast::buffers_front(base().data()).size() == 0;
}

} // namespace common
} // namespace radrpc