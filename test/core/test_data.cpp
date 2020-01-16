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

#include <test/core/random.hpp>
#include <test/core/test_data.hpp>

namespace test {
namespace core {

void test_data::init_test_data(std::size_t entries)
{
    m_entries = entries;
    m_data = std::map<std::size_t, std::vector<char>>();
    for (size_t i = 1; i <= m_entries; i++)
    {
        std::vector<char> rdata;
        rdata.reserve(i);
        for (size_t j = 0; j < i; j++)
        {
            rdata.insert(rdata.end(), m_sequence.begin(), m_sequence.end());
        }
        m_data.insert(
            std::pair<std::size_t, std::vector<char>>(rdata.size(), rdata));
    }
}

std::size_t test_data::init_test_data_bytes(std::size_t max_size)
{
    m_data = std::map<std::size_t, std::vector<char>>();
    const std::size_t seq_size = m_sequence.size();
    std::size_t byte_counter = 0;
    m_entries = 0;

    while (true)
    {
        ++m_entries;
        const std::size_t rdata_size = seq_size * m_entries;
        if (byte_counter + rdata_size >= max_size)
            break;

        std::vector<char> rdata;
        rdata.reserve(m_entries);
        for (size_t j = 0; j < m_entries; j++)
        {
            rdata.insert(rdata.end(), m_sequence.begin(), m_sequence.end());
        }

        m_data.insert(
            std::pair<std::size_t, std::vector<char>>(rdata.size(), rdata));
        byte_counter += rdata_size;
    }

    m_entries = m_data.size();
    return byte_counter;
}

const std::vector<char> test_data::get_random_data() const
{
    auto idx = rnd((std::size_t)0, m_entries - 1);
    std::size_t c = 0;
    for (const auto &d : m_data)
    {
        if (c == idx)
            return std::vector<char>(d.second);
        ++c;
    }
    return std::vector<char>();
}

data_state test_data::data_entry_valid(const std::vector<char> &data) const
{
    auto data_itr = m_data.find(data.size());
    if (data_itr == m_data.end())
        return data_state::not_found;
    if (data != data_itr->second)
    {
        return data_state::corrupted;
    }
    return data_state::valid;
}

data_state test_data::data_entry_valid(const char *data,
                                       std::size_t data_size) const
{
    auto data_itr = m_data.find(data_size);
    if (data_itr == m_data.end() || data_size == 0)
        return data_state::not_found;
    for (auto i = 0; i < data_size; ++i)
    {
        if (data_itr->second[i] != data[i])
        {
            return data_state::corrupted;
        }
    }
    return data_state::valid;
}

bool test_data::data_compare(const char *lhs_data,
                             std::size_t lhs_size,
                             const char *rhs_data,
                             std::size_t rhs_size) const
{
    if (lhs_size != rhs_size)
        return false;
    for (auto i = 0; i < lhs_size; ++i)
    {
        if (lhs_data[i] != rhs_data[i])
        {
            return false;
        }
    }
    return true;
}

} // namespace core
} // namespace test