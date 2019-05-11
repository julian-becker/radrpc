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

#ifndef RADRPC_DETAIL_OPERATIONS_HPP
#define RADRPC_DETAIL_OPERATIONS_HPP

#include <chrono>
#include <functional>
#include <future>
#include <memory>

#include "boost/asio/post.hpp"

#include <radrpc/config.hpp>
#include <radrpc/debug.hpp>

namespace radrpc {
namespace detail {

/**
 * Allows to post safely to the executor context
 * without sharing ownership with the help of 'weak_ptr'.
 * @param sp The shared pointer to check whether 'this' is still valid.
 * @param executor The executor to execute the handler.
 * @param handler The handler to call.
 */
template <class Type, typename Executor, typename SharedPointer>
inline void weak_post(const SharedPointer &sp,
                      const Executor &executor,
                      const std::function<void()> &handler)
{
    boost::asio::post(executor,
                      [handler{handler}, weak_write{std::weak_ptr<Type>(sp)}] {
                          if (auto active_write = weak_write.lock())
                          {
                              if (handler)
                                  handler();
                          }
                      });
}

/**
 * Allows to post safely to the executor context
 * without sharing ownership with the help of 'weak_ptr'
 * & wait til the handler was executed.
 * @param sp The shared pointer to check whether 'this' is still valid.
 * @param executor The executor to execute the handler.
 * @param handler The handler to call.
 * @param timeout The maximum time to wait for the execution.
 * @return true if it was executed, false if timeout
 */
template <class Type, typename Executor, typename SharedPointer>
inline bool wait_weak_post(const SharedPointer &sp,
                           const Executor &executor,
                           const std::function<void()> &handler,
                           const duration timeout)
{
    auto post_callback = std::make_shared<std::promise<bool>>();
    std::weak_ptr<std::promise<bool>> weak_post_callback = post_callback;
    auto posted = post_callback->get_future();

    boost::asio::post(executor,
                      [handler{handler},
                       weak_write{std::weak_ptr<Type>(sp)},
                       weak_post{std::move(weak_post_callback)}] {
                          if (auto active_write = weak_write.lock())
                          {
                              if (handler)
                                  handler();
                          }
                          if (auto active_post = weak_post.lock())
                          {
                              active_post->set_value(true);
                          }
                      });

    // Wait til the post is actually triggered in IO
    // and wait long enough to ensure it is stopped
    std::future_status post_status = posted.wait_for(timeout);
    if (post_status != std::future_status::ready)
    {
        return false;
    }
    return true;
}

/**
 * Set or get the timeout for sending data to the server for the calling thread.
 * @param timeout The timeout to set
 * @see radrpc::impl::client::client_session
 * @return The timeout if parameter 'timeout' was not set, otherwise zero.
 */
inline std::chrono::steady_clock::duration
send_timeout(std::chrono::steady_clock::duration timeout =
                 std::chrono::steady_clock::duration::max())
{
    thread_local duration tls_send_timeout = duration::zero();
    if (timeout == duration::max())
        return tls_send_timeout;
    tls_send_timeout = timeout;
    return duration::zero();
}

/**
 * Set or get the timeout for receiving data from the server for the calling
 * thread.
 * @param timeout The timeout to set
 * @see radrpc::impl::client::client_session
 * @return The timeout if parameter 'timeout' was not set, otherwise zero.
 */
inline std::chrono::steady_clock::duration
response_timeout(std::chrono::steady_clock::duration timeout =
                     std::chrono::steady_clock::duration::max())
{
    thread_local duration tls_response_timeout = duration::zero();
    if (timeout == duration::max())
        return tls_response_timeout;
    tls_response_timeout = timeout;
    return duration::zero();
}

} // namespace detail
} // namespace radrpc

#endif // RADRPC_DETAIL_OPERATIONS_HPP