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

#include <test/defaults/default_config.hpp>
#include <test/dep/catch.hpp>




std::unique_ptr<boost::beast::flat_buffer> create_buffer(
    const std::vector<char> &input =
        std::vector<char>(sizeof(radrpc::detail::io_header) + 10, 0x0))
{
    auto buffer = new receive_buffer();
    boost::asio::buffer_copy(buffer->prepare(input.size()),
                             boost::asio::buffer(input.data(), input.size()));
    buffer->commit(input.size());
    return std::unique_ptr<boost::beast::flat_buffer>(buffer);
}

bool data_compare(const char *lhs_data,
                  std::size_t lhs_size,
                  const char *rhs_data,
                  std::size_t rhs_size)
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




TEST_CASE("thread local storage timeout")
{
    using namespace radrpc::detail;
    REQUIRE(response_timeout() == duration::zero());
    response_timeout(std::chrono::milliseconds(100));
    duration t1_time;
    std::thread([&] { t1_time = response_timeout(); }).join();
    REQUIRE(response_timeout() == std::chrono::milliseconds(100));
    REQUIRE(t1_time == duration::zero());
    response_timeout(duration::zero());
    REQUIRE(response_timeout() == duration::zero());
    t1_time = duration::zero();
    std::thread([&] {
        response_timeout(std::chrono::milliseconds(100));
        t1_time = response_timeout();
    }).join();
    REQUIRE(t1_time == std::chrono::milliseconds(100));
}

TEST_CASE("data_cache implementation")
{
    SECTION("wait(receive_buffer)")
    {
        detail::data_cache cache(10);
        receive_buffer result;
        REQUIRE_FALSE(cache.wait(0, duration::zero(), result));
        REQUIRE(result.size() == 0);
    }

    SECTION("wait(std::shared_ptr<detail::data_read>)")
    {
        detail::data_cache cache(10);
        std::shared_ptr<detail::data_read> result;
        REQUIRE_FALSE(cache.wait(0, duration::zero(), result));
        REQUIRE(result == nullptr);
    }

    SECTION("queue")
    {
        detail::data_cache cache(10);
        REQUIRE(cache.queue(std::chrono::milliseconds(0)) == 1);
    }

    SECTION("queue clear")
    {
        detail::data_cache cache(10);
        REQUIRE(cache.queue(std::chrono::milliseconds(0)) == 1);
        REQUIRE(cache.size() == 1);
        cache.clear();
        REQUIRE(cache.size() == 0);
    }

    SECTION("queue clear referenced")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        cache.queue(std::chrono::milliseconds(0), delete_callback);
        cache.clear();
        REQUIRE(cache.size() == 0);
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
    }

    SECTION("queue limit & id distribution")
    {
        detail::data_cache cache(3);
        std::vector<char> result;
        for (int i = 0; i < 4; ++i)
        {
            if (i == 3)
                REQUIRE(cache.queue(std::chrono::milliseconds(0)) == 0);
            else
                REQUIRE(cache.queue(std::chrono::milliseconds(0)) == i + 1);
        }
        REQUIRE(cache.size() == 3);
    }

    SECTION("remove obsolete entries")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(2);
        auto delete_callback = [&] { --ref_counter; };
        cache.queue(std::chrono::milliseconds(1), delete_callback);
        sleep_ms(1);
        cache.queue(std::chrono::milliseconds(20), delete_callback);
        cache.remove_obsolete();
        REQUIRE(cache.size() == 1);
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 1);
        sleep_ms(20);
        cache.remove_obsolete();
        REQUIRE(cache.size() == 0);
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
    }

    SECTION("queue wait clear(receive_buffer)")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        receive_buffer result;
        auto id = cache.queue(std::chrono::milliseconds(0), delete_callback);
        auto buffer = create_buffer();
        std::thread t1([&] {
            sleep_ms(5);
            cache.clear();
        });
        REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
        t1.join();
        REQUIRE(cache.size() == 0);
    }

    SECTION("queue wait clear(std::shared_ptr<detail::data_read>)")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        {
            std::shared_ptr<detail::data_read> result;
            auto id =
                cache.queue(std::chrono::milliseconds(0), delete_callback);
            auto buffer = create_buffer();
            std::thread t1([&] {
                sleep_ms(5);
                cache.clear();
            });
            REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
            REQUIRE(result != nullptr);
            t1.join();
            REQUIRE(ref_counter == 1);
        }
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
        REQUIRE(cache.size() == 0);
    }

    SECTION("queue -> wait(receive_buffer) -> swap")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        receive_buffer result;
        auto id = cache.queue(std::chrono::milliseconds(0), delete_callback);
        auto buffer = create_buffer();
        std::thread t1([&] {
            sleep_ms(5);
            cache.swap_notify(id, *buffer);
        });
        REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
        REQUIRE_FALSE(result.empty());
        t1.join();
    }

    SECTION("queue -> wait(std::shared_ptr<detail::data_read>) -> swap")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        {
            std::shared_ptr<detail::data_read> result;
            auto id =
                cache.queue(std::chrono::milliseconds(0), delete_callback);
            auto buffer = create_buffer();
            std::thread t1([&] {
                sleep_ms(5);
                cache.swap_notify(id, *buffer);
            });
            REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
            REQUIRE(ref_counter == 1);
            REQUIRE(result != nullptr);
            REQUIRE(boost::beast::buffers_front(result->read_buffer.data())
                        .size() != 0);
            t1.join();
        }
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
    }

    SECTION("queue -> wait(receive_buffer) -> late swap")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        receive_buffer result;
        auto id = cache.queue(std::chrono::milliseconds(0), delete_callback);
        auto buffer = create_buffer();
        std::thread t1([&] {
            sleep_ms(40);
            cache.swap_notify(id, *buffer);
        });
        REQUIRE_FALSE(cache.wait(id, std::chrono::milliseconds(5), result));
        REQUIRE(ref_counter == 1);
        REQUIRE(result.empty());
        t1.join();
    }

    SECTION("queue -> wait(std::shared_ptr<detail::data_read>) -> late swap")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        {
            std::shared_ptr<detail::data_read> result;
            auto id =
                cache.queue(std::chrono::milliseconds(0), delete_callback);
            auto buffer = create_buffer();
            std::thread t1([&] {
                sleep_ms(40);
                cache.swap_notify(id, *buffer);
            });
            REQUIRE_FALSE(cache.wait(id, std::chrono::milliseconds(5), result));
            REQUIRE(ref_counter == 1);
            REQUIRE(result != nullptr);
            t1.join();
        }
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
    }

    SECTION("queue -> swap -> wait(receive_buffer)")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        receive_buffer result;
        auto id = cache.queue(std::chrono::milliseconds(0), delete_callback);
        auto buffer = create_buffer();
        cache.swap_notify(id, *buffer);
        REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
        REQUIRE_FALSE(result.empty());
    }

    SECTION("queue -> swap -> wait(std::shared_ptr<detail::data_read>)")
    {
        detail::data_cache cache(10);
        std::atomic<int> ref_counter = ATOMIC_VAR_INIT(1);
        auto delete_callback = [&] { --ref_counter; };
        {
            std::shared_ptr<detail::data_read> result;
            auto id =
                cache.queue(std::chrono::milliseconds(0), delete_callback);
            auto buffer = create_buffer();
            cache.swap_notify(id, *buffer);
            REQUIRE(cache.wait(id, std::chrono::milliseconds(40), result));
            REQUIRE(ref_counter == 1);
            REQUIRE(result != nullptr);
            REQUIRE(boost::beast::buffers_front(result->read_buffer.data())
                        .size() != 0);
        }
        sleep_ms(sleep_low_delay_ms);
        REQUIRE(ref_counter == 0);
    }
}

TEST_CASE("receive_buffer wrapper")
{
    std::string msg = "Hello wonderful bytes";
    std::vector<char> bytes(msg.begin(), msg.end());
    auto flbuffer = create_buffer(bytes);
    receive_buffer buffer;
    std::swap(buffer.base(), *flbuffer);

    SECTION("data compare")
    {
        REQUIRE(data_compare(static_cast<const char *>(buffer.data()),
                             buffer.size(),
                             bytes.data(),
                             bytes.size()));
    }

    SECTION("clear")
    {
        buffer.clear();
        REQUIRE(buffer.empty());
        REQUIRE_FALSE(data_compare(static_cast<const char *>(buffer.data()),
                                   buffer.size(),
                                   bytes.data(),
                                   bytes.size()));
    }
}