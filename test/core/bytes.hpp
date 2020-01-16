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

#ifndef RADRPC_TEST_CORE_BYTES_HPP
#define RADRPC_TEST_CORE_BYTES_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace test {
namespace core{

inline void dump_bytes(const std::string &file_name,
                       std::vector<char> &byteCode)
{
    std::ofstream os;
    os.open((file_name).c_str());
    for (size_t i = 0; i < byteCode.size(); i++)
    {
        char buff[100];
        snprintf(buff, sizeof(buff), "%02X", byteCode[i]);
        os << buff << " ";
    }
    os.close();
}

inline void printf_bytes(std::vector<char> &byteCode)
{
    for (size_t i = 0; i < byteCode.size(); i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

inline void printf_bytes(const std::vector<char> &byteCode)
{
    for (size_t i = 0; i < byteCode.size(); i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

inline void printf_bytes(const char *byteCode, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

template <typename T> T bytes_to_obj(std::vector<char> bytes)
{
    T object_type = *reinterpret_cast<T *>(&bytes[0]);
    return object_type;
}

template <typename T> std::vector<char> obj_to_bytes(const T &object)
{
    std::vector<char> bytes;
    bytes.resize(sizeof(T));
    const char *begin = reinterpret_cast<const char *>(std::addressof(object));
    const char *end = begin + sizeof(T);
    std::copy(begin, end, std::begin(bytes));
    return bytes;
}

template <typename T> std::vector<T> vector_of_object(const T &object)
{
    return std::vector<T>();
}

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_BYTES_HPP