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

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <test/common/data_state.hpp>

namespace test {
namespace core {

class test_data
{
    public:
      const static std::size_t sequence_size = 16;
      const static std::size_t byte_sequences = 0xFFFFF;
      const static std::size_t max_data_size = byte_sequences * sequence_size;

    private:

#ifdef _WIN32
#pragma pack(push, 1)
    struct byte_sequence
#else
    struct __attribute__((packed)) byte_sequence
#endif
    {
        char byte[sequence_size] = {
            0x0,
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
            0xF,
        };
    };
#ifdef _WIN32
#pragma pack(pop)
#endif

    const byte_sequence m_data[byte_sequences];
    const char *m_data_ptr;
    const std::size_t m_data_size;
    const std::size_t m_max_idx;
    std::size_t m_min_bytes;
    std::size_t m_max_bytes;

    /**
     * @brief Get the random data object
     * 
     * @param offset 
     * @param data 
     * @param size 
     */
    void get_random_data(uint32_t &offset,
                         const char *&data,
                         std::size_t &size) const;

    /**
     * @brief 
     * 
     * @param offset 
     * @param data 
     * @param size 
     * @return data_state 
     */
    data_state data_valid(uint32_t &offset,
                          const char *data,
                          std::size_t size) const;

  public:
    test_data();

    /**
     * @brief Set the limit object
     * 
     * @param min_size 
     * @param max_size 
     */
    void set_limit(std::size_t min_size, std::size_t max_size);

    /**
     * @brief Get the random data object
     * 
     * @return std::vector<char> 
     */
    std::vector<char> get_random_data() const;

    /**
     * @brief 
     * 
     * @param data 
     * @param size 
     * @return data_state 
     */
    data_state data_valid(const char *data, std::size_t size) const;

    /**
     * @brief 
     * 
     * @param data 
     * @return data_state 
     */
    data_state data_valid(const std::vector<char> &data) const;
};

} // namespace core
} // namespace test

#endif // RADRPC_TEST_CORE_TEST_DATA_HPP