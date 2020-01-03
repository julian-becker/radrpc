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

#include "radrpc.hpp"




// User defined remote procedure calls
enum MyRpcCommands
{
    MY_RPC_SEND,
    MY_RPC_ECHO_MSG,
    MY_RPC_BROADCAST_ALL,
    MY_RPC_BROADCAST_SINGLE,
    MY_RPC_LISTEN_BROADCAST,
    MY_RPC_SERVER_SHUTDOWN,
};

inline std::mutex &log_mtx()
{
    static std::mutex m_log_mtx;
    return m_log_mtx;
}

// Small logger utility
#define LOG(str)                                                               \
    do                                                                         \
    {                                                                          \
        log_mtx().lock();                                                      \
        std::cout << std::fixed << std::setprecision(5) << str << std::endl;   \
        log_mtx().unlock();                                                    \
    } while (false)




std::thread broadcast_worker;
std::atomic<bool> broadcast_run = ATOMIC_VAR_INIT(true);
void broadcast(radrpc::server *p_srv)
{
    std::string msg("radrpc got a broadcast function");
    std::vector<char> msg_bytes(msg.begin(), msg.end());
    while (broadcast_run)
    {
        p_srv->broadcast(MY_RPC_LISTEN_BROADCAST, msg_bytes);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}




bool on_listen(const std::string &remote_host)
{
    LOG("on_listen: [IP:" << remote_host << "]");
    return true; // True if request should be processed, false if not
}

bool on_accept(radrpc::session_info &info)
{
    LOG("on_accept: [IP:" << info.remote_host << "] [SID:" << info.id << "]"
                          << "\nHandshake from client:\n"
                          << info.request_handshake);




    ////////////////////////////////////////////////////////
    // How to inspect the client's handshake
    //
    // Note:
    // -With SSL this handshake will also
    //  be transferred encrypted.
    ////////////////////////////////////////////////////////

    // Find default field 'user_agent'.
    auto agent_itr =
        info.request_handshake.find(boost::beast::http::field::user_agent);
    if (agent_itr != info.request_handshake.end())
    {
        LOG("on_accept: user_agent found:"
            << "\nField name: " << agent_itr->name_string() //
            << "\nField value: " << agent_itr->value()      //
            << "\nField enum id: "
            << agent_itr->name() // This Enum will be printed out as string
        );
    }
    // If this field doesn't exist, you may reject this session
    else
    {
        // Field not found, reject this session.
        return false;
    }

    // Find custom field 'key'.
    auto key_itr =
        info.request_handshake.find("key");
    if (key_itr != info.request_handshake.end())
    {
        LOG("on_accept: key found:"
            << "\nField name: " << key_itr->name_string() //
            << "\nField value: " << key_itr->value()      //
            << "\nField enum id: "
            << key_itr->name() // This Enum will be printed out as string
        );
        // For example you can check the value and
        // reject the session with returning false.
        if (key_itr->value() != "xxxxxxxxxx")
        {
            LOG("on_accept: Reject session from [IP:" << info.remote_host
                                                      << "]");
            return false;
        }
    }
    // If this field doesn't exist, you may reject this session
    else
    {
        // Field not found, reject this session.
        return false;
    }




    ////////////////////////////////////////////////////////
    // How to set response handshake for client
    //
    // Warning:
    // Undefined behavior results when the fields specific to perform the
    // WebSocket Upgrade, such as the 'Upgrade' or 'Connection' are
    // modified.
    //
    // Note:
    // -With SSL this handshake will also
    //  be transferred encrypted.
    // -No 'set()' should be used, since 'response_handshake' is empty
    //  and will later be append to the original handshake
    ////////////////////////////////////////////////////////

    // Insert default field user_agent.
    info.response_handshake.insert(boost::beast::http::field::user_agent,
                                   "radrpc-server");

    // Insert custom field user-level.
    info.response_handshake.insert("user-level", "1");




    ////////////////////////////////////////////////////////
    // How to set configuration for this session
    //
    // Note:
    // If this is not set, the default session configuration
    // will be used (which was passed to the server constructor).
    ////////////////////////////////////////////////////////

    // Set ping delay.
    // This will check if the client is still connected.
    info.config.ping_delay = std::chrono::seconds(30);

    // Set maximum transfer bytes.
    // This will limit the transferred
    // bytes on each request known as
    // 'send()' / 'send_recv()' from client.
    info.config.max_transfer_bytes = 0xFFFF;




    return true; // True if session should be accepted, false if not.
}




int main()
{
    using namespace radrpc;

    server_config cfg;
    cfg.host_address = "0.0.0.0";
    cfg.port = 3377;
    cfg.workers = 3;
    cfg.max_sessions = 1000;
    cfg.max_handshake_bytes = 0xFFFF;
    cfg.mode = server_mode::plain;

    server_timeout timeout;
    timeout.handshake_or_close_timeout = std::chrono::seconds(5);

    session_config default_session_cfg;
    default_session_cfg.max_transfer_bytes = 0xFFFFFFFF;
    default_session_cfg.ping_delay = std::chrono::seconds(30);

    server srv(cfg, timeout, default_session_cfg);




    // Bind the handlers to specific events.
    // A lambda can also be used.

    srv.bind_listen(on_listen);

    srv.bind_accept(on_accept);

    srv.bind_disconnect([&](const session_info &info) {
        LOG("disconnect: [IP:" << info.remote_host << "]");
    });

    srv.bind(MY_RPC_SEND, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        LOG("RPC_SEND: " << received);
    });

    srv.bind(MY_RPC_ECHO_MSG, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        LOG("RPC_ECHO_MSG: " << received);
        ctx->response =
            std::vector<char>(ctx->data(), ctx->data() + ctx->size());
    });

    srv.bind(MY_RPC_BROADCAST_ALL, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        std::string msg("client " + std::to_string(ctx->id) + " writes: " +
                        received);
        LOG("RPC_BROADCAST_ALL: Broadcast this message:\n" << msg);
        std::vector<char> msg_bytes(msg.begin(), msg.end());
        srv.broadcast(MY_RPC_LISTEN_BROADCAST, msg_bytes);
        ctx->response.push_back(0x0);
    });

    srv.bind(MY_RPC_BROADCAST_SINGLE, [&](session_context *ctx) {
        std::string received(ctx->data(), ctx->data() + ctx->size());
        std::string msg("client " + std::to_string(ctx->id) + " write:\n" +
                        received);
        LOG("MY_RPC_BROADCAST_SINGLE: Broadcast this message:\n" << msg);
        std::vector<char> msg_bytes(msg.begin(), msg.end());
        // In a real-world case you would keep a 
        // list of session ids to send later
        std::vector<uint64_t> send_to = {ctx->id};
        srv.broadcast(MY_RPC_LISTEN_BROADCAST, msg_bytes, send_to);
        ctx->response.push_back(0x0);
    });

    srv.bind(MY_RPC_SERVER_SHUTDOWN, [&](session_context *ctx) {
        // This handler should only contain the shutdown procedure.
        LOG("RPC_SERVER_SHUTDOWN");
        broadcast_run = false;
        srv.stop();
    });




    // 'async_start()' allows to run in non-blocking mode
    // & can be used with other blocking threads.
    // Also it allows to call a handler,
    // if io has been stopped on all workers.
    srv.async_start([&] { broadcast_run = false; });

    // Create a blocking worker to broadcast messages to clients.
    // The 'broadcast()' function can be called
    // from another thread (must ensure the server is still valid)
    // or from a server executed handler.
    broadcast_worker = std::thread(broadcast, &srv);
    broadcast_worker.join();
    LOG("broacast thread joined");

    // It is possible to manually stop the server.
    // However the server will call this anyway in the destructor.
    srv.stop();
}