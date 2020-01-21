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

#include <algorithm>

#include <test/core/random.hpp>
#include <test/core/test_data.hpp>
#include <test/core/throw.hpp>

namespace test {
namespace core {

void test_data::get_random_data(uint32_t &offset,
                                const char *&data,
                                std::size_t &size) const
{
    TEST_ASSERT(m_max_bytes > 0);
    size = static_cast<std::size_t>(rnd(m_min_bytes, m_max_bytes));
    offset = static_cast<uint32_t>(rnd(0ul, m_max_idx));
    offset = std::min(static_cast<uint32_t>(m_max_idx - size), offset);
    data = m_data_ptr + offset;
}

data_state test_data::data_valid(uint32_t &offset,
                                 const char *data,
                                 std::size_t size) const
{
    if (offset + size > m_max_idx)
        return data_state::corrupted;

    auto ptr = m_data_ptr + offset;
    for (std::size_t i = 0; i < size; i++)
    {
        if (*(ptr + i) != data[i])
            return data_state::corrupted;
    }
    return data_state::valid;
}

test_data::test_data() :
    m_data{},
    m_data_ptr(reinterpret_cast<const char *>(&m_data)),
    m_data_size(sizeof(m_data)),
    m_max_idx(m_data_size - 1),
    m_min_bytes(0),
    m_max_bytes(0)
{
    static_assert(sizeof(byte_sequence) == sequence_size,
                  "byte_sequence size invalid");
    static_assert(sizeof(m_data) == sequence_size * byte_sequences,
                  "data size invalid");
}

void test_data::set_limit(std::size_t min_size, std::size_t max_size)
{
    TEST_ASSERT(min_size > 0);
    TEST_ASSERT(min_size <= max_size);
    TEST_ASSERT(max_size < m_data_size);
    m_min_bytes = min_size;
    m_max_bytes = max_size;
}

std::vector<char> test_data::get_random_data() const
{
    uint32_t offset = 0;
    const char *data = nullptr;
    std::size_t size = 0;
    get_random_data(offset, data, size);

    std::vector<char> data_out(sizeof(uint32_t) + size, 0x0);
    *reinterpret_cast<uint32_t *>(data_out.data()) = offset;
    memcpy(data_out.data() + sizeof(uint32_t), data, size);

    return data_out;
}

data_state test_data::data_valid(const char *data, std::size_t size) const
{
    // Data buffer needs atleast 1 byte
    if (size <= sizeof(uint32_t))
        return data_state::corrupted;

    uint32_t offset = *reinterpret_cast<const uint32_t *>(data);
    const char *buffer_data = data + sizeof(uint32_t);
    std::size_t buffer_size = size - sizeof(uint32_t);

    return data_valid(offset, buffer_data, buffer_size);
}

data_state test_data::data_valid(const std::vector<char> &data) const
{
    return data_valid(data.data(), data.size());
}

} // namespace core
} // namespace test