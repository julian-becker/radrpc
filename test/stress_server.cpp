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

#define BRPC_TEST_BINARY
#include <test/dep/unit_utils.hpp>




server_config default_server_config()
{
    server_config cfg;
    cfg.host_address = "0.0.0.0"; 
    cfg.port = 3377;
    cfg.workers = 3;
    cfg.max_sessions = 10000;
    cfg.max_handshake_bytes = 1024 * 200;
    cfg.mode = server_mode::plain | server_mode::ssl;
    return cfg;
}

server_timeout default_server_timeout()
{
    server_timeout cfg;
    cfg.handshake_or_close_timeout = std::chrono::milliseconds(500);
    return cfg;
}

session_config default_session_config()
{
    session_config cfg;
    cfg.max_transfer_bytes = 1024 * 200;
    cfg.ping_delay = std::chrono::milliseconds(1000);
    return cfg;
}




class broadcaster
{
    std::thread m_worker;
    std::atomic<bool> m_broadcast_run;
    radrpc::server *m_srv;
    test_data *m_data;
    server_settings m_set;

  public:
    broadcaster(radrpc::server *p_srv, test_data *p_data) :
        m_broadcast_run(false),
        m_srv(p_srv),
        m_data(p_data),
        m_set({})
    {
    }

    ~broadcaster() { stop(); }

    void set(const server_settings &p_set) { m_set = p_set; }

    void run()
    {
        if (m_broadcast_run)
            return;
        m_broadcast_run = true;
        m_worker = std::thread([&] {
            while (m_broadcast_run)
            {
                m_srv->broadcast(STRESS_RPC_SERVER_MSG,
                                 m_data->get_random_data());
                sleep_ms(m_set.broadcast_delay_ms);
            }
        });
    }

    void stop()
    {
        m_broadcast_run = false;
        if (m_worker.joinable())
            m_worker.join();
    }
};




int main()
{
    sanitizer_info();
    broadcaster *broadcast = nullptr;
    auto data = std::make_unique<test_data>();
    auto set = std::make_unique<server_settings>();
    auto ctrl_cfg = default_server_config();
    ctrl_cfg.workers = 1;
    ctrl_cfg.port = 3378;
    ctrl_cfg.mode = server_mode::plain;
    auto ctrl_timeout = default_server_timeout();
    auto ctrl_session_cfg = default_session_config();
    server ctrl_srv(ctrl_cfg, ctrl_timeout, ctrl_session_cfg);
    ctrl_srv.bind(STRESS_RPC_INIT, [&](session_context *ctx) {
        if (ctx->size() == sizeof(server_settings))
        {
            auto srv_set =
                reinterpret_cast<const server_settings *>(ctx->data());
            memcpy(set.get(), srv_set, sizeof(server_settings));

            broadcast->stop();
            data->init_test_data(srv_set->test_entries);
            broadcast->set(*srv_set);
            broadcast->run();
            CINFO("RPC_INIT: Done");

            ctx->response.push_back(0x0);
        }
        else
        {
            CINFO("RPC_INIT: Invalid server_settings size "
                  << ctx->size() << "/" << sizeof(server_settings));
        }
    });




    std::atomic<bool> restart_server = ATOMIC_VAR_INIT(false);
    auto cfg = default_server_config();
    auto timeout = default_server_timeout();
    auto session_cfg = default_session_config();
#ifdef RADRPC_SSL_SUPPORT
    ssl::context ssl_ctx(ssl::context::sslv23);
    boost::system::error_code ec;

    ssl_ctx.set_password_callback(
        [&](std::size_t, boost::asio::ssl::context_base::password_purpose) {
            return ssl_test_files::server_key_pass;
        },
        ec);
    if (ec)
        UNIT_THROW("set_password_callback: " << ec.message());

    ssl_ctx.use_certificate_chain(
        boost::asio::buffer(ssl_test_files::server_certificate,
                            std::strlen(ssl_test_files::server_certificate)),
        ec);
    if (ec)
        UNIT_THROW("use_certificate_chain: " << ec.message());

    ssl_ctx.use_private_key(
        boost::asio::buffer(ssl_test_files::server_key,
                            std::strlen(ssl_test_files::server_key)),
        boost::asio::ssl::context::file_format::pem,
        ec);
    if (ec)
        UNIT_THROW("use_private_key: " << ec.message());

    ssl_ctx.use_tmp_dh(boost::asio::buffer(ssl_test_files::dh_key,
                                           std::strlen(ssl_test_files::dh_key)),
                       ec);
    if (ec)
        UNIT_THROW("use_tmp_dh: " << ec.message());

    server srv(cfg, timeout, session_cfg, std::move(ssl_ctx));
#else
    server srv(cfg, timeout, session_cfg);
#endif
    ctrl_srv.bind(STRESS_RPC_SHUTDOWN, [&](session_context *ctx) {
        srv.stop();
    });
    broadcast = new broadcaster(&srv, data.get());
    ctrl_srv.async_start();




    std::function<void()> abort_server = [&]() { srv.stop(); };

    srv.bind_accept([&](session_info &info) {
        auto field_itr = info.request_handshake.find("y");
        if (field_itr != info.request_handshake.end() &&
            field_itr->value() == "yyyyyyyyyy")
        {
            info.response_handshake.insert("x", "xxxxxxxxxx");
            if (rnd_bool(50))
                srv.broadcast(STRESS_RPC_SERVER_MSG, data->get_random_data());
            else
                srv.broadcast(STRESS_RPC_SERVER_MSG,
                              data->get_random_data(),
                              std::unordered_set<uint64_t>{info.id});
            return rnd_bool(set->accept_chance);
        }
        else
        {
            CINFO("on_accept: invalid handshake request:\n"
                  << info.request_handshake);
            abort_server();
            return false;
        }
    });

    srv.bind_listen([&](const std::string &ip) {
        if (srv.connections() < 0)
            abort_server();
        return rnd_bool(set->connect_chance);
    });

    srv.bind_disconnect([&](const session_info &info) {
        auto ids = srv.get_session_ids();
        for (const auto &id : ids)
        {
            if (id == 0)
                abort_server();
        }
    });

    srv.bind(STRESS_RPC_RESTART, [&](session_context *ctx) {
        restart_server = true;
        srv.stop();
    });

    srv.bind(STRESS_RPC_ECHO, [&](session_context *ctx) {
        auto ec = data->data_entry_valid(ctx->data(), ctx->size());
        if (ec != DATA_VALID)
        {
            CINFO("STRESS_RPC_ECHO: invalid " << ec);
            abort_server();
            return;
        }
        rnd_sleep_ms(set->min_delay_ms, set->max_delay_ms);
        if (rnd_bool(set->response_chance))
            ctx->response =
                std::vector<char>(ctx->data(), ctx->data() + ctx->size());
        if (rnd_bool(set->close_chance))
            ctx->close();
    });

    srv.bind(STRESS_RPC_SEND, [&](session_context *ctx) {
        auto ec = data->data_entry_valid(ctx->data(), ctx->size());
        if (ec != DATA_VALID)
        {
            CINFO("STRESS_RPC_SEND: invalid " << ec);
            abort_server();
            return;
        }
        rnd_sleep_ms(set->min_delay_ms, set->max_delay_ms);
        if (rnd_bool(set->close_chance))
            ctx->close();
    });

    srv.bind(STRESS_RPC_SEND_RECV, [&](session_context *ctx) {
        auto ec = data->data_entry_valid(ctx->data(), ctx->size());
        if (ec != DATA_VALID)
        {
            CINFO("RPC_SEND_RECV: invalid " << ec);
            abort_server();
            return;
        }
        rnd_sleep_ms(set->min_delay_ms, set->max_delay_ms);
        if (rnd_bool(set->response_chance))
            ctx->response = data->get_random_data();
        if (rnd_bool(set->close_chance))
            ctx->close();
    });

    // A bit heavy for the server to handle with many clients
    srv.bind(STRESS_RPC_SEND_BROADCAST, [&](session_context *ctx) {
        auto ec = data->data_entry_valid(ctx->data(), ctx->size());
        if (ec != DATA_VALID)
        {
            CINFO("STRESS_RPC_SEND_BROADCAST: invalid " << ec);
            abort_server();
            return;
        }
        if (rnd_bool(50))
            srv.broadcast(STRESS_RPC_SERVER_MSG, data->get_random_data());
        else
            srv.broadcast(STRESS_RPC_SERVER_MSG,
                          data->get_random_data(),
                          std::unordered_set<uint64_t>{ctx->id});

        rnd_sleep_ms(set->min_delay_ms, set->max_delay_ms);
        if (rnd_bool(set->response_chance))
            ctx->response = data->get_random_data();
        if (rnd_bool(set->close_chance))
            ctx->close();
    });




restart:
    if (restart_server)
        broadcast->run();
    srv.start();
    broadcast->stop();
    srv.stop();

    if (restart_server)
    {
        restart_server = false;
        goto restart;
    }




    ctrl_srv.stop();
    delete broadcast;
    return 0;
}