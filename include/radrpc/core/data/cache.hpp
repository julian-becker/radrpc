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

#ifndef RADRPC_CORE_DATA_CACHE_HPP
#define RADRPC_CORE_DATA_CACHE_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include <boost/beast/core/flat_buffer.hpp>

#include <radrpc/config.hpp>
#include <radrpc/core/data/read.hpp>
#include <radrpc/core/data/state.hpp>
#include <radrpc/debug/log.hpp>

namespace radrpc {
namespace core {
namespace data {

/**
 * Provides a solution to hold, queue, wait & notify data.
 */
class cache
{
    /// Maximum entries allowed in queue.
    const std::size_t m_max_entries;

    /// Lock, since it will be used out of the io handlers.
    std::mutex m_mtx;

    /// Id counter which increments on each sent data.
    uint64_t m_id_counter;

    /// Holds all queued data.
    std::map<uint64_t, std::shared_ptr<read>> m_entries;

  public:
    /**
     * @param p_max_entries The maximum entries allowed in queue.
     */
    explicit cache(std::size_t p_max_entries) :
        m_max_entries(p_max_entries),
        m_id_counter(0)
    {
    }

    /**
     * Queue an empty 'read' which will be
     * assigned later in 'swap_notify()'.
     * @param valid The duration how long this data may remain in the cache.
     * @see radrpc::core::data::cache::wait
     * @see radrpc::core::data::cache::swap_notify
     * @return The id of the data which can be used to wait for the data.
     */
    uint64_t queue(duration valid)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_entries.size() >= m_max_entries)
            return 0;
        auto ptr = new read(std::chrono::steady_clock::now(), valid);
        uint64_t id = ++m_id_counter;
        m_entries[id] = std::shared_ptr<read>(ptr, [&, id](read *p) {
            delete p;
            RADRPC_LOG("cache::queue: [" << id << "] Deleted shared_ptr");
        });
        RADRPC_LOG("cache::queue: [" << id << "] Created shared_ptr");
        return id;
    }

    /**
     * Queue an empty 'read' which will be
     * assigned later in 'swap_notify()'.
     * @param valid The duration how long this data may remain in the cache.
     * @param delete_callback The handler to call if the data is going to be
     * deleted.
     * @see radrpc::core::data::cache::wait
     * @see radrpc::core::data::cache::swap_notify
     * @return The id of the data which can be used to wait for the data.
     */
    uint64_t queue(duration valid, const std::function<void()> &delete_callback)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_entries.size() >= m_max_entries)
            return 0;
        auto ptr = new read(std::chrono::steady_clock::now(), valid);
        m_entries[++m_id_counter] =
            std::shared_ptr<read>(ptr, [delete_callback](read *p) {
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
        std::shared_ptr<read> row;
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
    bool wait(uint64_t id, duration timeout, std::shared_ptr<read> &row)
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
        if (row->state == state::swapped)
        {
            m_entries.erase(cache_itr);
            m_mtx.unlock(); // Unlock table
            return true;
        }
        row->state = state::waiting;
        m_mtx.unlock(); // Unlock table
        auto success = row->cv.wait_for(
            row_lock, timeout, [row] { return row->state == state::swapped; });
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
            state state;
            auto row = cache_itr->second->shared_from_this();
            {
                std::unique_lock<std::mutex> row_lock(row->mtx);
                state = row->state;
                std::swap(buffer, row->read_buffer);
                buffer = boost::beast::flat_buffer();
                row->state = state::swapped;
            }
            if (state == state::waiting)
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
                row->state = state::swapped;
            }
            row->cv.notify_all();
            m_entries.erase(cache_itr++);
        }
    }
};

} // namespace data
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_DATA_CACHE_HPP