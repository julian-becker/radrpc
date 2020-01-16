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

#ifndef RADRPC_TEST_CORE_TEST_DATA_HPP
#define RADRPC_TEST_CORE_TEST_DATA_HPP

#include <cstdint>
#include <map>
#include <vector>

#include <test/common/data_state.hpp>

namespace test {
namespace core {

class test_data
{
    /// 
    std::size_t m_entries;

    /// 
    const std::vector<char> m_sequence = {
        0x1,
        0x2,
        0x3,
        0x4,
        0x5,
        0x6,
        0x7,
        0x8,
        0x9,
        0xA,
        0xB,
        0xC,
        0xD,
        0xE,
    };

    /// 
    std::map<std::size_t, std::vector<char>> m_data;

  public:

    /**
     * @brief 
     * 
     * @param entries 
     */
    void init_test_data(std::size_t entries);

    /**
     * @brief 
     * 
     * @param max_size 
     * @return std::size_t 
     */
    std::size_t init_test_data_bytes(std::size_t max_size);

    /**
     * @brief Get the random data object
     * 
     * @return const std::vector<char> 
     */
    const std::vector<char> get_random_data() const;

    /**
     * @brief 
     * 
     * @param data 
     * @return data_state 
     */
    data_state data_entry_valid(const std::vector<char> &data) const;

    /**
     * @brief 
     * 
     * @param data 
     * @param data_size 
     * @return data_state 
     */
    data_state data_entry_valid(const char *data, std::size_t data_size) const;

    /**
     * @brief 
     * 
     * @param lhs_data 
     * @param lhs_size 
     * @param rhs_data 
     * @param rhs_size 
     * @return true 
     * @return false 
     */
    bool data_compare(const char *lhs_data,
                      std::size_t lhs_size,
                      const char *rhs_data,
                      std::size_t rhs_size) const;
};

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_TEST_DATA_HPP