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

#ifndef RADRPC_COMMON_ANY_PTR_HPP
#define RADRPC_COMMON_ANY_PTR_HPP

#include <string>
#include <cstdint>

#include <boost/type_index.hpp>

namespace radrpc {
namespace common {

class any_ptr
{
    template <typename Type> static size_t get_static_address()
    {
        // Get unique type address.
        static int _;
        return reinterpret_cast<std::size_t>(&_);
    }

    std::size_t m_type_id;
    void *m_ptr;

  public:
    any_ptr() : m_type_id(get_static_address<void>()), m_ptr(nullptr) {}

    any_ptr(std::nullptr_t) : any_ptr() {}

    template <typename T>
    any_ptr(T *ptr)
        :
        m_type_id(get_static_address<T>()),
        m_ptr(const_cast<void *>(reinterpret_cast<const void *>(ptr)))
    {
    }

    template <typename T> T *get() const
    {
        if (m_type_id != get_static_address<T>())
            return nullptr;
        return reinterpret_cast<T *>(m_ptr);
    }
};

} // namespace common

using any_ptr = radrpc::common::any_ptr;

} // namespace radrpc

#endif // RADRPC_COMMON_ANY_PTR_HPP