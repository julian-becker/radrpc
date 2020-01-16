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

#ifndef RADRPC_CORE_DATA_QUEUE_HPP
#define RADRPC_CORE_DATA_QUEUE_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>

#include <radrpc/core/data/send.hpp>

namespace radrpc {
namespace core {
namespace data {

/**
 * Provides a solution to hold the data which needs to be
 * written to the server & notify whether it was written.
 * This will be usually used in tandem with the websocket's 'async_write()'
 * function within the IO context/thread.
 * Using it external will result in data races.
 */
class queue
{
    /// Maximum entries allowed in queue.
    const std::size_t m_max_entries;

    /// Queued data. Using deque to allow a future option to prioritize data.
    std::deque<send> m_entries;

  public:
    /**
     * @param p_max_entries The maximum entries allowed in queue.
     */
    explicit queue(std::size_t p_max_entries) : m_max_entries(p_max_entries) {}

    /**
     * Queue the data which should be written to the server.
     * @param call_id The id to call serverside.
     * @param result_id The id assign later the received data at the server
     * @param callback The callback to check whether the data was written.
     * @param send_bytes The bytes to send.
     * @return True if data was queued, false if queue is full.
     */
    bool queue_data(uint32_t call_id,
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
    send *front() { return &m_entries.front(); }

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

} // namespace data
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_DATA_QUEUE_HPP