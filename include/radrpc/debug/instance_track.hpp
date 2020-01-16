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

#ifndef RADRPC_DEBUG_INSTANCE_TRACK_HPP
#define RADRPC_DEBUG_INSTANCE_TRACK_HPP

#include <iostream>
#include <thread>

namespace radrpc {
namespace debug {

/**
 * @brief Tracks instances of desired classes by defining 'instance_track'
 * within a class & let a thread idle. These then can be tracked easier with a
 * debugger later.
 */
template <typename T> class instance_track
{
    /// Loop breaker.
    bool m_run;

    /// The loop thread.
    std::thread m_thread;

    /// The pointer of the instance.
    T *m_instance_ptr;

    void loop()
    {
        while (m_run)
        {
            if (m_instance_ptr == (void *)0x1)
                std::cout << " " << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

  public:
    instance_track(T *p_instance_ptr) :
        m_run(true),
        m_instance_ptr(p_instance_ptr)
    {
        m_thread = std::thread(&instance_track::loop, this);
    }

    ~instance_track()
    {
        m_run = false;
        if (m_thread.joinable())
            m_thread.join();
    }
};

} // namespace debug
} // namespace radrpc

#endif // RADRPC_DEBUG_INSTANCE_TRACK_HPP