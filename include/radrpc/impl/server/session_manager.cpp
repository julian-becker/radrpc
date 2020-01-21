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

#include <set>
#include <unordered_set>

#include <boost/beast/core/tcp_stream.hpp>

#include <radrpc/debug/log.hpp>
#include <radrpc/core/data/push.hpp>
#include <radrpc/impl/server/session_manager.hpp>
#include <radrpc/impl/server/server_session.hpp>

namespace radrpc {
namespace impl {
namespace server {

using namespace radrpc::common;
using namespace radrpc::core;

template <typename StlContainer>
void session_manager::broadcast(uint32_t call_id,
                                const std::vector<char> &send_bytes,
                                StlContainer *session_ids)
{
    if (m_msg_queued >= config::queue_send_max_bytes)
    {
        RADRPC_LOG("session_manager::broadcast: Queue is full");
        return;
    }
    if (connections() == 0)
        return;
        
    // Convert to network byte order (big endian)
    const auto push = new data::push(io_header(htonl(call_id), 0), send_bytes);
    auto push_ptr = std::shared_ptr<const data::push>(
        push,
        std::bind(&session_manager::on_msg_sent,
                  shared_from_this(),
                  std::placeholders::_1));
    m_msg_queued++;

    // Copy sessions to avoid long locking.
    plain_sessions plain_sessions_tmp;
    {
        std::unique_lock<std::mutex> lock(m_plain_mtx);
        if (session_ids)
        {
            for (const auto &id : *session_ids)
            {
                auto session_itr =
                    m_plain_sessions.find(static_cast<uint64_t>(id));
                if (session_itr != m_plain_sessions.end())
                    plain_sessions_tmp.emplace(session_itr->first,
                                               session_itr->second);
            }
        }
        else
        {
            plain_sessions_tmp = m_plain_sessions;
        }
    }
    for (auto const &s : plain_sessions_tmp)
        if (auto session = s.second.lock())
            session->send(push_ptr);



#ifdef RADRPC_SSL_SUPPORT
    // Copy sessions to avoid long locking.
    ssl_sessions ssl_sessions_tmp;
    {
        std::unique_lock<std::mutex> lock(m_ssl_mtx);
        if (session_ids)
        {
            for (const auto &id : *session_ids)
            {
                auto session_itr =
                    m_ssl_sessions.find(static_cast<uint64_t>(id));
                if (session_itr != m_ssl_sessions.end())
                    ssl_sessions_tmp.emplace(session_itr->first,
                                             session_itr->second);
            }
        }
        else
        {
            ssl_sessions_tmp = m_ssl_sessions;
        }
    }
    for (auto const &s : ssl_sessions_tmp)
        if (auto session = s.second.lock())
            session->send(push_ptr);
#endif
}

void session_manager::on_msg_sent(const data::push *p)
{
    m_msg_queued--;
    RADRPC_LOG("session_manager::on_msg_sent: " << m_msg_queued);
    delete p;
}

session_manager::session_manager(server_config p_server_cfg) :
    m_sub_refs(0),
    m_msg_queued(0),
    server_cfg(std::move(p_server_cfg)),
    on_accept(nullptr),
    on_listen(nullptr),
    on_disconnect(nullptr)
{
}

void session_manager::set_subtract_refs()
{
    // This can be only called once
    RADRPC_ASSERT(m_sub_refs == 0);
    m_sub_refs = shared_from_this().use_count();
    RADRPC_LOG("session_manager::set_subtract_refs: " << m_sub_refs);
}

#pragma warning(suppress : 4018)
bool session_manager::is_full()
{
    return connections() >= server_cfg.max_sessions;
}

long session_manager::connections()
{
    // Use max just in case...
    return std::max(shared_from_this().use_count() - m_sub_refs, 0l);
}

void session_manager::add_plain_session(
    const std::shared_ptr<server_session<session_accept<streams::plain>>>
        &session)
{
    std::unique_lock<std::mutex> lock(m_plain_mtx);
    m_plain_sessions[session->id] =
        std::weak_ptr<server_session<session_accept<streams::plain>>>(session);
}

void session_manager::remove_plain_session(uint64_t id)
{
    std::unique_lock<std::mutex> lock(m_plain_mtx);
    auto session_itr = m_plain_sessions.find(id);
    if (session_itr != m_plain_sessions.end())
    {
        m_plain_sessions.erase(session_itr);
    }
}

#ifdef RADRPC_SSL_SUPPORT

void session_manager::add_ssl_session(
    const std::shared_ptr<server_session<session_accept<streams::ssl>>>
        &session)
{
    std::unique_lock<std::mutex> lock(m_ssl_mtx);
    m_ssl_sessions[session->id] =
        std::weak_ptr<server_session<session_accept<streams::ssl>>>(session);
}

void session_manager::remove_ssl_session(uint64_t id)
{
    std::unique_lock<std::mutex> lock(m_ssl_mtx);
    auto session_itr = m_ssl_sessions.find(id);
    if (session_itr != m_ssl_sessions.end())
    {
        m_ssl_sessions.erase(session_itr);
    }
}

#endif

void session_manager::broadcast(uint32_t call_id,
                                const std::vector<char> &send_bytes)
{
    broadcast(call_id, send_bytes, static_cast<std::vector<char> *>(nullptr));
}

template <typename StlContainer>
void session_manager::broadcast(uint32_t call_id,
                                const std::vector<char> &send_bytes,
                                const StlContainer &session_ids)
{
    broadcast(call_id, send_bytes, &session_ids);
}

template void session_manager::broadcast<std::vector<uint64_t>>(
    uint32_t,
    const std::vector<char> &,
    const std::vector<uint64_t> &);

template void session_manager::broadcast<std::set<uint64_t>>(
    uint32_t,
    const std::vector<char> &,
    const std::set<uint64_t> &);

template void session_manager::broadcast<std::unordered_set<uint64_t>>(
    uint32_t,
    const std::vector<char> &,
    const std::unordered_set<uint64_t> &);

std::vector<uint64_t> session_manager::get_session_ids()
{
    std::vector<uint64_t> ids;
    std::unique_lock<std::mutex> plain_lock(m_plain_mtx);
#ifdef RADRPC_SSL_SUPPORT
    std::unique_lock<std::mutex> ssl_lock(m_ssl_mtx);
    ids.reserve(m_plain_sessions.size() + m_ssl_sessions.size());
    for (const auto &id : m_plain_sessions)
        ids.push_back(id.first);
    for (const auto &id : m_ssl_sessions)
        ids.push_back(id.first);
#else
    ids.reserve(m_plain_sessions.size());
    for (const auto &id : m_plain_sessions)
        ids.push_back(id.first);
#endif
    return ids;
}

} // namespace server
} // namespace impl
} // namespace radrpc