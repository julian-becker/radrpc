/*
 * MIT License

 * Copyright (c) 2019 reapler

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

#define CATCH_CONFIG_MAIN
#include <test/dep/unit_utils.hpp>
#include <test/dep/catch.hpp>




TEST_CASE("utils")
{
    SECTION("random functions")
    {
        INFO("rnd()");
        REQUIRE(rnd(-100, -100) == -100);
        REQUIRE(rnd(0, 0) == 0);
        REQUIRE(rnd(100, 100) == 100);
        bool rnd_ok = true;
        for (auto i = 0; i < 1000000; ++i)
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
        for (auto i = 0; i < 1000000; ++i)
        {
            if (!rnd_bool(100) || rnd_bool(0))
            {
                rnd__bool_ok = false;
                break;
            }
        }
        REQUIRE(rnd__bool_ok);
    }

    SECTION("test_data")
    {
        test_data tdata;
        INFO("init_test_data_bytes()");
        auto bytes = tdata.init_test_data_bytes(1000000);
        REQUIRE((bytes <= 1000000 && bytes > 0));

        INFO("get_random_data()");
        auto in = tdata.get_random_data();
        REQUIRE_FALSE(in.empty());

        INFO("data_entry_valid() == DATA_VALID");
        REQUIRE(tdata.data_entry_valid(in) == DATA_VALID);
        REQUIRE(tdata.data_entry_valid(in.data(), in.size()) == DATA_VALID);

        INFO("data_entry_valid() == DATA_CORRUPTED");
        in[0] = 0x0;
        REQUIRE(tdata.data_entry_valid(in) == DATA_CORRUPTED);
        REQUIRE(tdata.data_entry_valid(in.data(), in.size()) == DATA_CORRUPTED);

        INFO("data_entry_valid() == DATA_NOT_FOUND");
        in.pop_back();
        REQUIRE(tdata.data_entry_valid(in) == DATA_NOT_FOUND);
        REQUIRE(tdata.data_entry_valid(in.data(), in.size()) == DATA_NOT_FOUND);
    }
}