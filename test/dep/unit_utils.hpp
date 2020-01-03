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

#ifndef RADRPC_TEST_UNIT_UTILS_HPP
#define RADRPC_TEST_UNIT_UTILS_HPP
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <cstdlib>
#include <csignal>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "radrpc.hpp"
#include "random.hpp"
#include "termcolor.hpp"
#include "ssl_test_files.hpp"

using thread_random = effolkronium::random_thread_local;
using duration = std::chrono::steady_clock::duration;
using namespace radrpc;




struct client_settings
{
    uint32_t clients_per_mode;
    uint32_t max_threads;
    uint32_t min_queue_delay_ms;
    uint32_t max_queue_delay_ms;
    uint32_t disconnect_chance;
    uint32_t timeout_ms;
    uint32_t restart_chance;
    uint32_t send_attempts;
    uint32_t attempts_delay_ms;
    bool random_send_timeout;
};
inline std::ostream &operator<<(std::ostream &os, const client_settings &obj)
{
    os << "clients_per_mode: " << obj.clients_per_mode << std::endl
       << "max_threads: " << obj.max_threads << std::endl
       << "min_queue_delay_ms: " << obj.min_queue_delay_ms << std::endl
       << "max_queue_delay_ms: " << obj.max_queue_delay_ms << std::endl
       << "disconnect_chance: " << obj.disconnect_chance << std::endl
       << "timeout_ms: " << obj.timeout_ms << std::endl
       << "restart_chance: " << obj.restart_chance << std::endl
       << "send_attempts: " << obj.send_attempts << std::endl
       << "attempts_delay_ms: " << obj.attempts_delay_ms << std::endl
       << "random_send_timeout: " << obj.random_send_timeout << std::endl;
    return os;
}

#ifdef _WIN32
#pragma pack(push, 1)
struct server_settings
#else
struct __attribute__((packed)) server_settings
#endif
{
    uint32_t accept_chance;
    uint32_t connect_chance;
    uint32_t response_chance;
    uint32_t close_chance;
    uint32_t min_delay_ms;
    uint32_t max_delay_ms;
    uint32_t broadcast_delay_ms;
    uint32_t test_entries;
};
#ifdef _WIN32
#pragma pack(pop)
#endif

inline std::ostream &operator<<(std::ostream &os, const server_settings &obj)
{
    os << "accept_chance: " << obj.accept_chance << std::endl
       << "connect_chance: " << obj.connect_chance << std::endl
       << "response_chance: " << obj.response_chance << std::endl
       << "close_chance: " << obj.close_chance << std::endl
       << "min_delay_ms: " << obj.min_delay_ms << std::endl
       << "max_delay_ms: " << obj.max_delay_ms << std::endl
       << "broadcast_delay_ms: " << obj.broadcast_delay_ms << std::endl
       << "test_entries: " << obj.test_entries << std::endl;
    return os;
}

// https://github.com/progschj/ThreadPool
class ThreadPool
{
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_cv;
    bool m_stop;
    const size_t m_max_queue;

  public:
    ThreadPool(size_t threads, size_t max_queue) :
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

    ~ThreadPool()
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

enum StressRpcCommands
{
    STRESS_RPC_ECHO,
    STRESS_RPC_SEND,
    STRESS_RPC_SEND_RECV,
    STRESS_RPC_SEND_BROADCAST,
    STRESS_RPC_PING,
    STRESS_RPC_INIT,
    STRESS_RPC_RESTART,
    STRESS_RPC_SHUTDOWN,
    STRESS_RPC_SERVER_MSG,
};

enum DataError
{
    DATA_VALID,
    DATA_NOT_FOUND,
    DATA_CORRUPTED,
};




inline std::mutex &unit_log_mtx()
{
    static std::mutex m_unit_log_mtx;
    return m_unit_log_mtx;
}
#define CINFO(str)                                                             \
    do                                                                         \
    {                                                                          \
        unit_log_mtx().lock();                                                 \
        std::cout << "[" << termcolor::cyan << "INFO" << termcolor::reset      \
                  << "] " << str << std::endl;                                 \
        unit_log_mtx().unlock();                                               \
    } while (false)
#define CFAIL(str)                                                             \
    do                                                                         \
    {                                                                          \
        unit_log_mtx().lock();                                                 \
        std::cout << "[" << termcolor::red << "FAILED" << termcolor::reset     \
                  << "] " << str << std::endl;                                 \
        unit_log_mtx().unlock();                                               \
    } while (false)
inline void clog_lock() { unit_log_mtx().lock(); }
inline void clog_unlock() { unit_log_mtx().unlock(); }

#ifdef _WIN32
#define UNIT_FILENAME                                                          \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define UNIT_FILENAME                                                          \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define UNIT_THROW(MSG)                                                        \
    do                                                                         \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << MSG << "\nIn " << __FUNCTION__ << " " << UNIT_FILENAME << ":"    \
           << __LINE__;                                                        \
        CFAIL(ss.str().c_str());                                               \
        std::raise(SIGTERM);                                                   \
    } while (false)

template <typename F, typename... Args> auto get_time(F &&f, Args &&... args)
{
    using namespace std::chrono;

    auto t0 = high_resolution_clock::now();
    std::forward<F>(f)(std::forward<Args>(args)...);
    auto t1 = high_resolution_clock::now();
    return t1 - t0;
}

#define REQUIRE_TIME(e, t) REQUIRE((get_time([&] { e; }) > t))

inline void sanitizer_info()
{
#ifdef ASAN
    CINFO("AddressSanitizer active");
    return;
#endif
#ifdef UBSAN
    CINFO("UndefinedBehaviorSanitizer active");
    return;
#endif
#ifdef MSAN
    CINFO("MemorySanitizer active");
    return;
#endif
#ifdef TSAN
    CINFO("ThreadSanitizer active");
    return;
#endif
    CINFO("No sanitizer active");
}

inline void dumpBytes(const std::string &file_name, std::vector<char> &byteCode)
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

inline void printfBytes(std::vector<char> &byteCode)
{
    for (size_t i = 0; i < byteCode.size(); i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

inline void printfBytes(const std::vector<char> &byteCode)
{
    for (size_t i = 0; i < byteCode.size(); i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

inline void printfBytes(const char *byteCode, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X ", byteCode[i]);
    }
    printf("\n");
}

template <typename T> T to_obj(std::vector<char> bytes)
{
    T object_type = *reinterpret_cast<T *>(&bytes[0]);
    return object_type;
}

template <typename T> std::vector<char> to_bytes(const T &object)
{
    std::vector<char> bytes;
    bytes.resize(sizeof(T));
    const char *begin = reinterpret_cast<const char *>(std::addressof(object));
    const char *end = begin + sizeof(T);
    std::copy(begin, end, std::begin(bytes));
    return bytes;
}

inline void sleep_ms(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline int rnd(int min, int max) { return thread_random::get(min, max); }

inline void rnd_sleep_ms(int min_ms, int max_ms)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)thread_random::get(min_ms, max_ms)));
}

inline bool rnd_bool(int true_chance_percent)
{
    if (true_chance_percent == 0)
        return false;
    return thread_random::get(0, 100) <= true_chance_percent;
}




class test_data
{
    std::size_t m_entries;
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
    std::map<std::size_t, std::vector<char>> m_data;

  public:
    void init_test_data(std::size_t entries)
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

    std::size_t init_test_data_bytes(std::size_t max_size)
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

    const std::vector<char> get_random_data() const
    {
        auto idx = thread_random::get((std::size_t)0, m_entries - 1);
        std::size_t c = 0;
        for (const auto &d : m_data)
        {
            if (c == idx)
                return std::vector<char>(d.second);
            ++c;
        }
        return std::vector<char>();
    }

    DataError data_entry_valid(const std::vector<char> &data) const
    {
        auto data_itr = m_data.find(data.size());
        if (data_itr == m_data.end())
            return DATA_NOT_FOUND;
        if (data != data_itr->second)
        {
            return DATA_CORRUPTED;
        }
        return DATA_VALID;
    }

    DataError data_entry_valid(const char *data, std::size_t data_size) const
    {
        auto data_itr = m_data.find(data_size);
        if (data_itr == m_data.end() || data_size == 0)
            return DATA_NOT_FOUND;
        for (auto i = 0; i < data_size; ++i)
        {
            if (data_itr->second[i] != data[i])
            {
                return DATA_CORRUPTED;
            }
        }
        return DATA_VALID;
    }

    bool data_compare(const char *lhs_data,
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
};

#pragma clang diagnostic pop
#endif // RADRPC_TEST_UNIT_UTILS_HPP