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

#ifndef RADRPC_CORE_DATA_READ_HPP
#define RADRPC_CORE_DATA_READ_HPP

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <memory>

#include <boost/beast/core/flat_buffer.hpp>

#include <radrpc/config.hpp>
#include <radrpc/core/data/state.hpp>

namespace radrpc {
namespace core {
namespace data {

/**
 * A container to hold all needed information
 * for a received packet from the server.
 * @see radrpc::core::data::cache
 */
class read : public std::enable_shared_from_this<read>
{
  public:
    /// Creation timestamp.
    time_point timestamp;

    /// Used to check whether data is still valid.
    int64_t valid_ms;

    /// The state of the data.
    state state;

    /// The data buffer.
    boost::beast::flat_buffer read_buffer;

    /// Used to lock & change data.
    std::mutex mtx;

    /// Used in 'cache' to wait on.
    std::condition_variable cv;

    /**
     * @param p_timestamp The creation timestamp.
     * @param p_valid The validity duration.
     */
    read(time_point p_timestamp, duration p_valid) :
        timestamp(p_timestamp),
        valid_ms(std::chrono::duration_cast<std::chrono::milliseconds>(p_valid)
                     .count()),
        state(state::queued)
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

} // namespace data
} // namespace core
} // namespace radrpc

#endif // RADRPC_CORE_DATA_READ_HPP