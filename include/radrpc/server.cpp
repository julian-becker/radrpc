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

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <radrpc/server.hpp>
#include <radrpc/impl/server/listener.hpp>

namespace radrpc {

void server::run_async_workers(const std::function<void()> &io_stopped_handler)
{
    if (m_io_ctx.stopped())
        m_io_ctx.restart();
    m_workers = std::vector<std::thread>();
    {
        std::unique_lock<std::mutex> worker_lock(m_stop_mtx);
        m_workers_done = 0;
    }
    auto workers =
        m_async_start ? m_server_cfg.workers : m_server_cfg.workers - 1;
    for (auto i = workers; i > 0; --i)
    {
        m_workers.emplace_back([this, i, workers, io_stopped_handler] {
            RADRPC_LOG("server::run_async_workers: Worker " << i << " started");
            m_io_ctx.run();
            RADRPC_LOG("server::run_async_workers: Worker " << i << " done");
            bool notify;
            {
                std::unique_lock<std::mutex> worker_lock(m_stop_mtx);
                notify = ++m_workers_done == workers;
            }
            if (notify)
            {
                RADRPC_LOG("server::run_async_workers: IO has been stopped "
                           "on workers");
                m_cv_stop.notify_all();
                if (io_stopped_handler)
                    io_stopped_handler();
            }
        });
    }
}

void server::on_signal(const boost::system::error_code &ec, int signal_code)
{
    RADRPC_LOG("server::on_signal: " << signal_code);
    m_io_ctx.stop();
}

server::server(const server_config &p_server_cfg,
               const server_timeout &p_server_timeout,
               const session_config &p_session_cfg) :
#ifdef RADRPC_SSL_SUPPORT
    m_ssl_ctx(ssl::context::sslv23),
#endif
    m_running(false),
    m_async_start(false),
    m_workers_done(0),
    m_server_cfg(p_server_cfg),
    m_server_timeout(p_server_timeout),
    m_session_cfg(p_session_cfg),
    m_manager(std::make_shared<impl::server::session_manager>(p_server_cfg)),
    m_io_ctx(static_cast<int>(p_server_cfg.workers)),
    m_listener(std::make_shared<impl::server::listener>(
        m_io_ctx,
#ifdef RADRPC_SSL_SUPPORT
        nullptr,
#endif
        tcp::endpoint{boost::asio::ip::make_address(p_server_cfg.host_address),
                      p_server_cfg.port},
        m_manager,
        m_server_cfg,
        m_server_timeout,
        m_session_cfg)),
    m_signals(m_io_ctx, SIGINT, SIGTERM)
{
    RADRPC_LOG("+server");
    m_listener->run();
    m_signals.async_wait(std::bind(&server::on_signal,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
}

#ifdef RADRPC_SSL_SUPPORT

server::server(const server_config &p_server_cfg,
               const server_timeout &p_server_timeout,
               const session_config &p_session_cfg,
               ssl::context &&p_ssl_ctx) :
    m_ssl_ctx(std::move(p_ssl_ctx)),
    m_running(false),
    m_async_start(false),
    m_workers_done(0),
    m_server_cfg(p_server_cfg),
    m_server_timeout(p_server_timeout),
    m_session_cfg(p_session_cfg),
    m_manager(std::make_shared<impl::server::session_manager>(p_server_cfg)),
    m_io_ctx(static_cast<int>(p_server_cfg.workers)),
    m_listener(std::make_shared<impl::server::listener>(
        m_io_ctx,
        &m_ssl_ctx,
        tcp::endpoint{boost::asio::ip::make_address(p_server_cfg.host_address),
                      p_server_cfg.port},
        m_manager,
        m_server_cfg,
        m_server_timeout,
        m_session_cfg)),
    m_signals(m_io_ctx, SIGINT, SIGTERM)
{
    RADRPC_LOG("+server");
    m_listener->run();
    m_signals.async_wait(std::bind(&server::on_signal,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
}
#endif

server::~server()
{
    RADRPC_LOG("~server");
    stop();
}

void server::start()
{
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_running)
            return;
        RADRPC_LOG("server::start");
        m_running = true;
        m_async_start = false;
        run_async_workers();
    }

    RADRPC_LOG("server::start: Run blocking IO context");
    m_io_ctx.run();

    // SIGINT, SIGTERM, m_io_ctx.stop()
    RADRPC_LOG("server::start: Blocking IO context has been stopped");
}

void server::async_start(const std::function<void()> &io_stopped_handler)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_running)
        return;
    RADRPC_LOG("server::async_start");
    m_running = true;
    m_async_start = true;
    run_async_workers(io_stopped_handler);
}

void server::stop()
{
    if (m_io_ctx.get_executor().running_in_this_thread())
    {
        RADRPC_LOG("server::stop: Called from IO worker");
        std::thread([&] { m_io_ctx.stop(); }).join();
    }
    else
    {
        RADRPC_LOG("server::stop");
        m_io_ctx.stop();
        std::unique_lock<std::mutex> lock(m_mtx);
        if (!m_running)
            return;
        auto workers =
            m_async_start ? m_server_cfg.workers : m_server_cfg.workers - 1;
        std::unique_lock<std::mutex> worker_lock(m_stop_mtx);
        m_cv_stop.wait(worker_lock, [&] { return workers == m_workers_done; });
        RADRPC_LOG("server::stop: Workers done");
        for (auto &worker : m_workers)
        {
            if (worker.joinable())
                worker.join();
        }
        RADRPC_LOG("server::stop: Workers joined");
        m_running = false;
    }
}

void server::broadcast(uint32_t call_id, const std::vector<char> &send_bytes)
{
    m_manager->broadcast(call_id, send_bytes);
}

void server::broadcast(uint32_t call_id,
                       const std::vector<char> &send_bytes,
                       const std::vector<uint64_t> &session_ids)
{
    m_manager->broadcast(call_id, send_bytes, session_ids);
}

void server::broadcast(uint32_t call_id,
                       const std::vector<char> &send_bytes,
                       const std::set<uint64_t> &session_ids)
{
    m_manager->broadcast(call_id, send_bytes, session_ids);
}

void server::broadcast(uint32_t call_id,
                       const std::vector<char> &send_bytes,
                       const std::unordered_set<uint64_t> &session_ids)
{
    m_manager->broadcast(call_id, send_bytes, session_ids);
}

long server::connections() { return m_manager->connections(); }

std::vector<uint64_t> server::get_session_ids()
{
    return m_manager->get_session_ids();
}

bool server::bind(uint32_t bind_id,
                  std::function<void(session_context *)> handler)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_running || m_manager->connections() != 0)
        return false;
    auto func_itr = m_manager->bound_funcs.find(bind_id);
    if (func_itr != m_manager->bound_funcs.end())
        RADRPC_THROW("server::bind: The given id '" + std::to_string(bind_id) +
                     "' was already bound to a function.");
    m_manager->bound_funcs[bind_id] = std::move(handler);
    return true;
}

bool server::bind_listen(std::function<bool(const std::string &)> handler)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_running || m_manager->connections() != 0)
        return false;
    m_manager->on_listen = std::move(handler);
    return true;
}

bool server::bind_accept(std::function<bool(session_info &)> handler)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_running || m_manager->connections() != 0)
        return false;
    m_manager->on_accept = std::move(handler);
    return true;
}

bool server::bind_disconnect(std::function<void(const session_info &)> handler)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_running || m_manager->connections() != 0)
        return false;
    m_manager->on_disconnect = std::move(handler);
    return true;
}

} // namespace radrpc