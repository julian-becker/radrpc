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

#include <radrpc/client.hpp>
#include <test/core/defaults.hpp>
#include <test/core/random.hpp>
#include <test/core/sanitizer.hpp>
#include <test/core/sleep.hpp>
#include <test/core/ssl_context.hpp>
#include <test/core/test_data.hpp>
#include <test/core/throw.hpp>
#include <test/core/log.hpp>
#include <test/common/ssl_test_files.hpp>
#include <test/stress/broadcaster.hpp>
#include <test/stress/config_file.hpp>
#include <test/stress/test_suite/rpc_command.hpp>

using namespace test::common;
using namespace test::core;
using namespace test::stress;




int main()
{
    sanitizer_info();
    auto data = std::make_unique<test_data>();
    std::unique_ptr<broadcaster> broadcast = nullptr;
    test_suite::server_set set;


    // Got false positive data race on 'set' with
    // these variables, but since the modification and the access
    // can't happen at the same time, it is very unlikely a data race.
    // Still i would like to soothe sanitizer,
    // so atomic variables will be used for this case.
    std::atomic<bool> send_broadcast = ATOMIC_VAR_INIT(false);
    std::atomic<int> connect_chance = ATOMIC_VAR_INIT(0);


    // Set control server
    auto ctrl_cfg = default_server_config();
    ctrl_cfg.workers = 1;
    ctrl_cfg.port = 3378;
    ctrl_cfg.mode = stream_mode::plain;
    auto ctrl_timeout = default_server_timeout();
    auto ctrl_session_cfg = default_session_config();
    server control_server(ctrl_cfg, ctrl_timeout, ctrl_session_cfg);
    control_server.bind((uint32_t)rpc_command::init, [&](session_context *ctx) {
        if (ctx->size() == sizeof(test_suite::server_set))
        {
            // Copy message
            auto srv_set =
                *reinterpret_cast<const test_suite::server_set *>(ctx->data());
            srv_set.to_host();

            set = srv_set;
            connect_chance = srv_set.connect_chance;
            send_broadcast = srv_set.broadcast_delay_ms != 0;

            // Restart broadcaster
            broadcast->stop();
            data->set_limit(srv_set.min_send_bytes, srv_set.max_send_bytes);
            broadcast->set_cfg(srv_set);
            broadcast->run();
            TEST_INFO("rpc_command::init: Done\n" << srv_set);
            ctx->response.push_back(0x0);
        }
        else
        {
            TEST_INFO("rpc_command::init: Invalid test_suite::server_set size "
                     << ctx->size() << "/" << sizeof(test_suite::server_set));
        }
    });


    // Set test server
    std::atomic<bool> restart_server = ATOMIC_VAR_INIT(false);
    auto cfg = default_server_config();
    auto timeout = default_server_timeout();
    auto session_cfg = default_session_config();
#ifdef RADRPC_SSL_SUPPORT
    ssl::context ssl_ctx = server_ssl_context();
    boost::system::error_code ec;
    server srv(cfg, timeout, session_cfg, std::move(ssl_ctx));
#else
    server srv(cfg, timeout, session_cfg);
#endif
    std::function<void()> stop_server = [&]() { srv.stop(); };


    // Set broadcaster
    broadcast = std::make_unique<broadcaster>(
        srv, *data.get(), rpc_command::server_msg);


    // Start control server
    control_server.bind((uint32_t)rpc_command::shutdown, [&](session_context *ctx) { srv.stop(); });
    control_server.async_start();


    srv.bind_accept([&](session_info &info) {
        auto field_itr = info.request_handshake.find("y");
        if (field_itr != info.request_handshake.end() &&
            field_itr->value() == "yyyyyyyyyy")
        {
            info.response_handshake.insert("x", "xxxxxxxxxx");
            if (send_broadcast)
            {
                if (rnd_bool(50))
                    srv.broadcast((uint32_t)rpc_command::server_msg,
                                  data->get_random_data());
                else
                    srv.broadcast((uint32_t)rpc_command::server_msg,
                                  data->get_random_data(),
                                  std::unordered_set<uint64_t>{info.id});
            }
            return rnd_bool(set.accept_chance);
        }
        else
        {
            TEST_INFO("on_accept: invalid handshake request:\n"
                  << info.request_handshake);
            stop_server();
            return false;
        }
    });

    srv.bind_listen([&](const std::string &ip) {
        return rnd_bool(connect_chance);
    });

    srv.bind_disconnect([&](const session_info &info) {
        auto ids = srv.get_session_ids();
        for (const auto &id : ids)
        {
            if (id == 0)
                stop_server();
        }
    });

    srv.bind((uint32_t)rpc_command::restart, [&](session_context *ctx) {
        restart_server = true;
        srv.stop();
    });

    srv.bind((uint32_t)rpc_command::echo, [&](session_context *ctx) {
        auto ec = data->data_valid(ctx->data(), ctx->size());
        if (ec != data_state::valid)
        {
            TEST_INFO("STRESS_RPC_ECHO: invalid " << (int)ec);
            stop_server();
            return;
        }
        sleep_ms_rnd(set.min_delay_ms, set.max_delay_ms);
        if (rnd_bool(set.response_chance))
            ctx->response =
                std::vector<char>(ctx->data(), ctx->data() + ctx->size());
        if (rnd_bool(set.close_chance))
            ctx->close();
    });

    srv.bind((uint32_t)rpc_command::send, [&](session_context *ctx) {
        auto ec = data->data_valid(ctx->data(), ctx->size());
        if (ec != data_state::valid)
        {
            TEST_INFO("rpc_command::send: invalid " << (int)ec);
            stop_server();
            return;
        }
        sleep_ms_rnd(set.min_delay_ms, set.max_delay_ms);
        if (rnd_bool(set.close_chance))
            ctx->close();
    });

    srv.bind((uint32_t)rpc_command::send_recv, [&](session_context *ctx) {
        auto ec = data->data_valid(ctx->data(), ctx->size());
        if (ec != data_state::valid)
        {
            TEST_INFO("rpc_command::send_recv: invalid " << (int)ec);
            stop_server();
            return;
        }
        sleep_ms_rnd(set.min_delay_ms, set.max_delay_ms);
        if (rnd_bool(set.response_chance))
            ctx->response = data->get_random_data();
        if (rnd_bool(set.close_chance))
            ctx->close();
    });

    // A bit heavy for the server to handle with many clients
    srv.bind((uint32_t)rpc_command::send_broadcast, [&](session_context *ctx) {
        auto ec = data->data_valid(ctx->data(), ctx->size());
        if (ec != data_state::valid)
        {
            TEST_INFO("rpc_command::send_broadcast: invalid " << (int)ec);
            stop_server();
            return;
        }

        if (send_broadcast)
        {
            if (rnd_bool(50))
                srv.broadcast((uint32_t)rpc_command::server_msg,
                              data->get_random_data());
            else
                srv.broadcast((uint32_t)rpc_command::server_msg,
                              data->get_random_data(),
                              std::unordered_set<uint64_t>{ctx->id});
        }

        sleep_ms_rnd(set.min_delay_ms, set.max_delay_ms);
        if (rnd_bool(set.response_chance))
            ctx->response = data->get_random_data();
        if (rnd_bool(set.close_chance))
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




    control_server.stop();
    return 0;
}