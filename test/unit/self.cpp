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

#include <utility>
#include <iostream>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "catch.hpp"

#include <test/core/log.hpp>
#include <test/core/random.hpp>
#define private public
#include <test/core/test_data.hpp>

using namespace test::common;
using namespace test::core;




TEST_CASE("utils")
{
    SECTION("random functions")
    {
        TEST_DINFO("");
        INFO("rnd()");
        REQUIRE(rnd(-100, -100) == -100);
        REQUIRE(rnd(0, 0) == 0);
        REQUIRE(rnd(100, 100) == 100);
        bool rnd_ok = true;
        for (auto i = 0; i < 100000; ++i)
        {
            auto n = rnd(-100, 100);
            if (!(n >= -100 && n <= 100))
            {
                rnd_ok = false;
                break;
            }
        }
        REQUIRE(rnd_ok);

        INFO("rnd_bool()");
        bool rnd__bool_ok = true;
        for (auto i = 0; i < 100000; ++i)
        {
            if (!rnd_bool(100) || rnd_bool(0))
            {
                rnd__bool_ok = false;
                break;
            }
        }
        REQUIRE(rnd__bool_ok);
    }
}

TEST_CASE("test_data")
{
    TEST_DINFO("");
    class test_data_access : public test_data
    {
      public:
        test_data_access() : test_data() {}
    };
    auto tdata = std::make_unique<test_data_access>();
    uint32_t offset = 0;
    const char *ptr = nullptr;
    std::size_t size = 0;

    SECTION("throw")
    {
        REQUIRE_THROWS(tdata->get_random_data(offset, ptr, size));
        REQUIRE_THROWS(tdata->set_limit(0, 0));
        REQUIRE_THROWS(tdata->set_limit(2, 1));
        REQUIRE_THROWS(tdata->set_limit(1, test_data::max_data_size));
        REQUIRE_NOTHROW(tdata->set_limit(test_data::max_data_size - 1,
                                         test_data::max_data_size - 1));
    }

    SECTION("get_random_data - max_size | max_idx")
    {
        // get_random_data()
        // min/max bytes are checked
        {
            // size = rnd(m_min_bytes, m_max_bytes);
            size = test_data::max_data_size - 1;
            // offset = rnd(0, m_max_idx);
            offset = tdata->m_max_idx;
            offset = std::min((uint32_t)(tdata->m_max_idx - size), offset);
            ptr = tdata->m_data_ptr + offset;
        }
        REQUIRE(offset + size <= tdata->m_max_idx);
        // data_valid
        std::vector<char> data(size, 0x0);
        memcpy(data.data(), ptr, size);
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::valid);
        data.back() = 'x';
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::corrupted);
    }

    SECTION("get_random_data - min_size | min_idx")
    {
        // get_random_data()
        {
            // size = rnd(m_min_bytes, m_max_bytes);
            size = 1;
            // offset = rnd(0, m_max_idx);
            offset = 0;
            offset = std::min((uint32_t)(tdata->m_max_idx - size), offset);
            ptr = tdata->m_data_ptr + offset;
        }
        REQUIRE(offset + size <= tdata->m_max_idx);
        // data_valid
        std::vector<char> data(size, 0x0);
        memcpy(data.data(), ptr, size);
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::valid);
        data.back() = 'x';
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::corrupted);
    }

    SECTION("get_random_data - max_size | min_idx")
    {
        // get_random_data()
        { 
            // size = rnd(m_min_bytes, m_max_bytes);
            size = test_data::max_data_size - 1;
            // offset = rnd(0, m_max_idx);
            offset = 0;
            offset = std::min((uint32_t)(tdata->m_max_idx - size), offset);
            ptr = tdata->m_data_ptr + offset;
        }
        REQUIRE(offset + size <= tdata->m_max_idx);
        // data_valid
        std::vector<char> data(size, 0x0);
        memcpy(data.data(), ptr, size);
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::valid);
        data.back() = 'x';
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::corrupted);
    }

    SECTION("get_random_data - min_size | max_idx")
    {
        // get_random_data()
        {
            // size = rnd(m_min_bytes, m_max_bytes);
            size = 1;
            // offset = rnd(0, m_max_idx);
            offset = tdata->m_max_idx;
            offset = std::min((uint32_t)(tdata->m_max_idx - size), offset);
            ptr = tdata->m_data_ptr + offset;
        }
        REQUIRE(offset + size <= tdata->m_max_idx);
        // data_valid
        std::vector<char> data(size, 0x0);
        memcpy(data.data(), ptr, size);
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::valid);
        data.back() = 'x';
        REQUIRE(tdata->data_valid(offset, data.data(), data.size()) ==
                data_state::corrupted);
    }
}