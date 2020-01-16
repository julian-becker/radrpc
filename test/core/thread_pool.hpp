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

#ifndef RADRPC_TEST_CORE_THREAD_POOL_HPP
#define RADRPC_TEST_CORE_THREAD_POOL_HPP

#include <cstdint>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <ostream>
#include <queue>
#include <thread>
#include <vector>

namespace test {
namespace core {

/**
 * @brief Thread pool from https://github.com/progschj/ThreadPool
 *
 */
class thread_pool
{
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_cv;
    bool m_stop;
    const size_t m_max_queue;

  public:
    thread_pool(size_t threads, size_t max_queue) :
        m_stop(false),
        m_max_queue(max_queue)
    {
        for (auto i = 0; i < threads; ++i)
        {
            m_workers.emplace_back([this] {
                for (;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                        this->m_cv.wait(lock, [this] {
                            return this->m_stop || !this->m_tasks.empty();
                        });
                        if (this->m_stop && this->m_tasks.empty())
                            return;
                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~thread_pool()
    {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto &worker : m_workers)
            worker.join();
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return res;
    }

    bool can_queue()
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        return m_tasks.size() <= m_max_queue;
    }
};

} // namespace core

using thread_pool = test::core::thread_pool;

} // namespace test

#endif // RADRPC_TEST_CORE_THREAD_POOL_HPP