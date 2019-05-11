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

#ifndef RADRPC_DETAIL_DATA_HPP
#define RADRPC_DETAIL_DATA_HPP

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <boost/beast/core/flat_buffer.hpp>

#include <radrpc/config.hpp>
#include <radrpc/debug.hpp>

namespace radrpc {
namespace detail {

/**
 * Represent the states of the data.
 * @see radrpc::detail::data_cache
 * @see radrpc::detail::data_read
 */
enum class data_state
{
    queued,
    waiting,
    swapped,
};

#ifdef _WIN32
#pragma pack(push, 1)
/**
 * A header which is prepend to all data transferred between
 * two hosts & allows to identify these.
 */
struct io_header
#else
/**
 * A header which is prepend to all data transferred between
 * two hosts & allows to identify these.
 */
struct __attribute__((packed)) io_header
#endif
{
    uint32_t call_id;   ///< The id to call on the remote host.
    uint32_t pad0;      ///< Reserved field used as padding.
    uint64_t result_id; ///< The id which will be sent back to caller.

    io_header(uint32_t p_call_id, uint64_t p_result_id) :
        call_id(p_call_id),
        pad0(0),
        result_id(p_result_id)
    {
    }
};
#ifdef _WIN32
#pragma pack(pop)
#endif

/**
 * A container to hold all needed information
 * for a received packet from the server.
 * @see radrpc::detail::data_cache
 */
class data_read : public std::enable_shared_from_this<data_read>
{
  public:
    using time_point = std::chrono::steady_clock::time_point;
    time_point timestamp; ///< Creation timestamp.
    int64_t valid_ms;     ///< Used to check whether data is still valid.
    data_state state;     ///< The state of the data.
    boost::beast::flat_buffer read_buffer; ///< The data buffer.
    std::mutex mtx; ///< Used to lock & change data.
    std::condition_variable cv; ///< Used in 'data_cache' to wait on.

    /**
     * @param p_timestamp The creation timestamp.
     * @param p_valid The validity duration.
     */
    data_read(time_point p_timestamp, duration p_valid) :
        timestamp(p_timestamp),
        valid_ms(std::chrono::duration_cast<std::chrono::milliseconds>(p_valid)
                     .count()),
        state(data_state::queued)
    {
    }

    /**
     * Check whether this data is still valid.
     * @return True if data is valid, false if not.
     */
    bool valid()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - timestamp)
                   .count() < valid_ms;
    }
};

/**
 * A container for send data to clients.
 */
class data_push : public std::enable_shared_from_this<data_push>
{
  public:
    // duration timeout;
    io_header header;       ///<
    std::vector<char> body; ///<

    explicit data_push(const io_header &p_io_header) : header(p_io_header) {}
};

/**
 * A container for write data to server with an
 * option to notify the caller.
 */
class data_send
{
  public:
    std::weak_ptr<std::promise<bool>>
        result_callback;   ///< The callback to notify waiting functions.
    const char *body_ptr;  ///< The pointer of the body.
    std::size_t body_size; ///< The size of the body.
    io_header header;      ///<

    /**
     * @param p_call_id The id to call on remote host.
     * @param p_result_id The id which will be sent back to the caller
     * @param p_result_callback The weak callback to notify waiting functions.
     * @param p_body_ptr The pointer of the body to send.
     * @param p_body_size The size of the body to send.
     */
    data_send(uint32_t p_call_id,
              uint64_t p_result_id,
              std::weak_ptr<std::promise<bool>> p_result_callback,
              const char *p_body_ptr,
              std::size_t p_body_size) :
        result_callback(std::move(p_result_callback)),
        body_ptr(p_body_ptr),
        body_size(p_body_size),
        header(p_call_id, p_result_id)
    {
    }
};

/**
 * Provides a solution to hold, queue, wait & notify data.
 */
class data_cache
{
    const std::size_t m_max_entries; ///< Maximum entries allowed in queue.
    std::mutex m_mtx; ///< Lock, since it will be used out of the io handlers.
    uint64_t m_id_counter; ///< Id counter which increments on each sent data.
    std::map<uint64_t, std::shared_ptr<data_read>>
        m_entries; ///< Holds all queued data.

  public:
    /**
     * @param p_max_entries The maximum entries allowed in queue.
     */
    explicit data_cache(std::size_t p_max_entries) :
        m_max_entries(p_max_entries),
        m_id_counter(0)
    {
    }

    /**
     * Queue an empty 'data_read' which will be
     * assigned later in 'swap_notify()'.
     * @param valid The duration how long this data may remain in the cache.
     * @see radrpc::detail::data_cache::wait
     * @see radrpc::detail::data_cache::swap_notify
     * @return The id of the data which can be used to wait for the data.
     */
    uint64_t queue(duration valid)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_entries.size() >= m_max_entries)
            return 0;
        auto ptr = new data_read(std::chrono::steady_clock::now(), valid);
        uint64_t id = ++m_id_counter;
        m_entries[id] = std::shared_ptr<data_read>(ptr, [&, id](data_read *p) {
            delete p;
            RADRPC_LOG("data_cache::queue: [" << id << "] Deleted shared_ptr");
        });
        RADRPC_LOG("data_cache::queue: [" << id << "] Created shared_ptr");
        return id;
    }

    /**
     * Queue an empty 'data_read' which will be
     * assigned later in 'swap_notify()'.
     * @param valid The duration how long this data may remain in the cache.
     * @param delete_callback The handler to call if the data is going to be
     * deleted.
     * @see radrpc::detail::data_cache::wait
     * @see radrpc::detail::data_cache::swap_notify
     * @return The id of the data which can be used to wait for the data.
     */
    uint64_t queue(duration valid, const std::function<void()> &delete_callback)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_entries.size() >= m_max_entries)
            return 0;
        auto ptr = new data_read(std::chrono::steady_clock::now(), valid);
        m_entries[++m_id_counter] =
            std::shared_ptr<data_read>(ptr, [delete_callback](data_read *p) {
                delete p;
                if (delete_callback)
                    delete_callback();
            });
        return m_id_counter;
    }

    /**
     * Wait for the data which was queued by 'queue()'.
     * @param id The id to wait on.
     * @param timeout The timeout.
     * @param buffer The empty buffer to swap the received data in queue.
     * @return True if data received, false if timeout or invalid id.
     */
    bool wait(uint64_t id, duration timeout, boost::beast::flat_buffer &buffer)
    {
        std::shared_ptr<data_read> row;
        auto success = wait(id, timeout, row);
        if (row)
        {
            std::unique_lock<std::mutex> row_lock(row->mtx);
            std::swap(buffer, row->read_buffer);
        }
        return success;
    }

    /**
     * Wait for the data which was queued by 'queue()'.
     * @param id The id to wait on.
     * @param timeout The timeout.
     * @param row The empty ptr to assign ownership of the received data.
     * @return True if data received, false if timeout or invalid id.
     */
    bool wait(uint64_t id, duration timeout, std::shared_ptr<data_read> &row)
    {
        m_mtx.lock(); // Lock table
        auto cache_itr = m_entries.find(id);
        if (cache_itr != m_entries.end())
        {
            row = cache_itr->second->shared_from_this();
        }
        else
        {
            m_mtx.unlock(); // Unlock table
            return false;
        }
        std::unique_lock<std::mutex> row_lock(row->mtx); // Row is now locked
        // If the data is already swapped, skip wait & erase it
        if (row->state == data_state::swapped)
        {
            m_entries.erase(cache_itr);
            m_mtx.unlock(); // Unlock table
            return true;
        }
        row->state = data_state::waiting;
        m_mtx.unlock(); // Unlock table
        auto success = row->cv.wait_for(row_lock, timeout, [row] {
            return row->state == data_state::swapped;
        });
        return success;
    }

    /**
     * Swaps the received data from the server with the queued buffer & notify
     * all. This function will usually used for receiving data from the server.
     * @param id The id of the data.
     * @param buffer The buffer with the received data.
     */
    void swap_notify(uint64_t id, boost::beast::flat_buffer &buffer)
    {
        std::unique_lock<std::mutex> table_lock(m_mtx);
        auto cache_itr = m_entries.find(id);
        if (cache_itr != m_entries.end())
        {
            data_state state;
            auto row = cache_itr->second->shared_from_this();
            {
                std::unique_lock<std::mutex> row_lock(row->mtx);
                state = row->state;
                std::swap(buffer, row->read_buffer);
                buffer = boost::beast::flat_buffer();
                row->state = data_state::swapped;
            }
            if (state == data_state::waiting)
            {
                row->cv.notify_all();
                m_entries.erase(cache_itr);
            }
        }
    }

    /**
     * Remove obsolete entries which was queued & not being waited on or
     * swapped.
     * Note: 
     * It may not be needed now since queued data will be removed
     * anyway with 'wait()'. This was generally implemented for a feature to
     * allow receive all data from the server and lookup them later.
     */
    void remove_obsolete()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        for (auto cache_itr = m_entries.begin(); cache_itr != m_entries.end();)
        {
            auto row = cache_itr->second->shared_from_this();
            row->mtx.lock();
            if (!row->valid())
            {
                m_entries.erase(cache_itr++);
                row->mtx.unlock();
            }
            else
            {
                row->mtx.unlock();
                ++cache_itr;
            }
        }
    };

    /**
     * Returns the current size of the queue.
     * @return The size of the queue.
     */
    std::size_t size()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return m_entries.size();
    }

    /**
     * Clears all data, release its ownership & 
     * notify all waiting functions.
     * Caution:
     * This function should be carefully used, since
     * it can fully release the referenced buffer used in
     * writing or reading operations.
     */
    void clear()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        for (auto cache_itr = m_entries.begin(); cache_itr != m_entries.end();)
        {
            auto row = cache_itr->second->shared_from_this();
            {
                std::unique_lock<std::mutex> row_lock(row->mtx);
                row->state = data_state::swapped;
            }
            row->cv.notify_all();
            m_entries.erase(cache_itr++);
        }
    }
};

/**
 * Provides a solution to hold the data which needs to be
 * written to the server & notify whether it was written.
 * This will be usually used in tandem with the websocket's 'async_write()'
 * function within the IO context/thread. 
 * Using it external will result in data races.
 */
class data_queue
{
    const std::size_t m_max_entries; ///< Maximum entries allowed in queue.
    std::deque<data_send> m_entries; ///< Queued data. Using deque to allow
                                     ///< a future option to prioritize data.

  public:
    /**
     * @param p_max_entries The maximum entries allowed in queue.
     */
    explicit data_queue(std::size_t p_max_entries) :
        m_max_entries(p_max_entries)
    {
    }

    /**
     * Queue the data which should be written to the server.
     * @param call_id The id to call serverside.
     * @param result_id The id assign later the received data at the server
     * @param callback The callback to check whether the data was written.
     * @param send_bytes The bytes to send.
     * @return True if data was queued, false if queue is full.
     */
    bool queue(uint32_t call_id,
               uint64_t result_id,
               std::weak_ptr<std::promise<bool>> &callback,
               const char *p_body_ptr,
               std::size_t p_body_size)
    {
        if (m_entries.size() >= m_max_entries)
            return false;
        m_entries.emplace_back(
            call_id, result_id, callback, p_body_ptr, p_body_size);
        return true;
    }

    /**
     * Checks whether the queue processed with 'async_write()'.
     * @return True if writing, false if not.
     */
    bool is_writing() { return m_entries.size() > 1; }

    /**
     * Returns the first element in the queue.
     * @return The first element in the queue.
     */
    data_send *front() { return &m_entries.front(); }

    /**
     * Checks whether to write the next entry
     * @return True if should write next entry, false if not.
     */
    bool write_next()
    {
        try
        {
            if (auto active = m_entries.front().result_callback.lock())
                active->set_value(true);
        }
        catch (...)
        {
        }
        m_entries.pop_front();
        return !m_entries.empty();
    }

    /**
     * Clears all entries in the queue.
     * Caution:
     * This function should be carefully used, since
     * it can fully release the referenced buffer used in
     * writing or reading operations.
     */
    void clear()
    {
        for (auto &msg : m_entries)
        {
            try
            {
                if (auto active = msg.result_callback.lock())
                    active->set_value(false);
            }
            catch (...)
            {
            }
        }
        m_entries.clear();
    }
};

} // namespace detail
} // namespace radrpc

#endif // RADRPC_DETAIL_DATA_HPP