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

#include <csignal>

#include <radrpc/client.hpp>
#include <radrpc/impl/client/connector.hpp>
#include "radrpc/common/connection_state.hpp"

#define CATCH_CONFIG_CONSOLE_WIDTH 300
#include "catch.hpp"

#include <test/core/bytes.hpp>
#include <test/core/defaults.hpp>
#include <test/core/log.hpp>
#include <test/core/sleep.hpp>
#include <test/unit/construct/rpc_command.hpp>
#include <test/unit/construct/client.hpp>
#include <test/unit/construct/server.hpp>

using namespace test::unit::construct;

bool plain_connect_send_recv()
{
    auto cl = create_plain_client();
    cl->connect();
    return !cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()).empty();
}




TEST_CASE("plain server implementation")
{
    ////////////////////////////////////////////////////////
    // start() / stop()
    ////////////////////////////////////////////////////////

    SECTION("start & stop")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::thread t1([&] {
            sleep_ms(defaults::sleep_high_delay_ms);
            srv->stop();
        });
        srv->start();
        t1.join();
        srv->stop();
    }

    SECTION("start & remote stop")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        std::thread t1([&] {
            sleep_ms(defaults::sleep_high_delay_ms);
            REQUIRE(cl->connect());
            // This will timeout 'async_close', since the server doesn't respond
            // back
            REQUIRE(cl->send(UNIT_RPC_STOP, std::vector<char>()));
        });
        srv->start();
        t1.join();
        srv->stop();
    }

    SECTION("restart & remote stop")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        std::thread t1([&] {
            sleep_ms(defaults::sleep_high_delay_ms);
            REQUIRE(cl->connect());
            // This will timeout 'async_close', since the server doesn't respond
            // back
            REQUIRE(cl->send(UNIT_RPC_STOP, std::vector<char>()));
        });
        srv->start();
        t1.join();
        srv->stop();
        t1 = std::thread([&] {
            sleep_ms(defaults::sleep_high_delay_ms);
            REQUIRE(cl->connect());
            // This will timeout 'async_close', since the server doesn't respond
            // back
            REQUIRE(cl->send(UNIT_RPC_STOP, std::vector<char>()));
        });
        srv->start();
        t1.join();
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // async_start() / stop()
    ////////////////////////////////////////////////////////

    SECTION("async start & SIGTERM")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::thread t1([&] {
            sleep_ms(defaults::sleep_high_delay_ms);
            std::raise(SIGTERM);
        });
        srv->start();
        t1.join();
        srv->stop();
    }

    SECTION("async start & stop")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        srv->stop();
    }

    SECTION("async start & remote stop")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            // This will timeout 'async_close', since the server doesn't respond
            // back
            REQUIRE(cl->send(UNIT_RPC_STOP, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("async start & SIGTERM")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        std::raise(SIGTERM);
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // broadcast()
    ////////////////////////////////////////////////////////

    SECTION("broadcast all")
    {
        TEST_DINFO("");
        std::vector<char> msg(10, 0x1);
        std::atomic<int> received = ATOMIC_VAR_INIT(0);
        std::atomic<bool> msg_ok = ATOMIC_VAR_INIT(true);
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        auto listen_handler = [&](receive_buffer &p_data) {
            std::vector<char> recv_msg(p_data.data(),
                                       p_data.data() + p_data.size());
            if (recv_msg != msg)
                msg_ok = false;
            received++;
        };
        auto cl = create_plain_client();
        auto clients = vector_of_object(cl);
        for (auto i = 0; i < 3; ++i)
        {
            clients.emplace_back(create_plain_client());
            clients[i]->listen_broadcast(UNIT_RPC_SERVER_MSG, listen_handler);
            REQUIRE(clients[i]->connect());
        }
        sleep_ms(defaults::sleep_high_delay_ms);
        srv->broadcast(UNIT_RPC_SERVER_MSG, msg);
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(received == 3);
        REQUIRE(msg_ok);
        srv->stop();
    }

    SECTION("broadcast to session id")
    {
        TEST_DINFO("");
        std::vector<char> msg(sizeof(uint64_t), 0x0);
        std::atomic<int> received = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> session_id = ATOMIC_VAR_INIT(0);
        std::atomic<bool> msg_ok = ATOMIC_VAR_INIT(true);
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        auto listen_handler = [&](receive_buffer &p_data) {
            if (p_data.size() > 0)
            {
                std::vector<char> recv_msg(p_data.data(),
                                           p_data.data() + p_data.size());
                if (recv_msg != msg)
                    msg_ok = false;
                session_id = *reinterpret_cast<const uint64_t *>(p_data.data());
                received++;
            }
        };
        auto cl = create_plain_client();
        auto clients = vector_of_object(cl);
        for (auto i = 0; i < 3; ++i)
        {
            clients.emplace_back(create_plain_client());
            clients[i]->listen_broadcast(UNIT_RPC_SERVER_MSG, listen_handler);
            REQUIRE(clients[i]->connect());
        }
        sleep_ms(defaults::sleep_high_delay_ms);
        auto ids = srv->get_session_ids();
        REQUIRE(ids.size() == 3);
        std::memcpy(msg.data(), &ids[2], sizeof(uint64_t));
        srv->broadcast(UNIT_RPC_SERVER_MSG, msg, std::vector<uint64_t>{ids[2]});
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(received == 1);
        REQUIRE(msg_ok);
        REQUIRE(session_id == ids[2]);
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // bind()
    ////////////////////////////////////////////////////////

    SECTION("rebind function")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        REQUIRE_THROWS(srv->bind(UNIT_RPC_SEND_RECV,
                                 [&](radrpc::session_context *ctx) {}));
        srv->stop();
    }

    SECTION("bind while running")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE_FALSE(srv->bind(UNIT_RPC_SEND_RECV,
                                [&](radrpc::session_context *ctx) {}));
        REQUIRE_FALSE(srv->bind_accept(
            [&](radrpc::session_info &info) { return false; }));
        REQUIRE_FALSE(
            srv->bind_disconnect([&](const radrpc::session_info &info) {}));
        REQUIRE_FALSE(
            srv->bind_listen([&](const std::string &ip) { return false; }));
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // bind_disconnect() / bind_accept() / bind_listen()
    ////////////////////////////////////////////////////////

    SECTION("bind disconnect")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::atomic<bool> disconnected = ATOMIC_VAR_INIT(false);
        REQUIRE(srv->bind_disconnect(
            [&](const radrpc::session_info &info) { disconnected = true; }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(plain_connect_send_recv());
        sleep_ms(defaults::sleep_high_delay_ms * 2); // Wait for result
        srv->stop();
        REQUIRE(disconnected);
    }

    SECTION("bind accept session_info")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::atomic<bool> is_ok = ATOMIC_VAR_INIT(false);
        REQUIRE(srv->bind_accept([&](radrpc::session_info &info) {
            if (info.id != 0 && info.remote_host == "127.0.0.1")
                is_ok = true;
            return true;
        }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(plain_connect_send_recv());
        sleep_ms(defaults::sleep_high_delay_ms); // Wait for result
        srv->stop();
        REQUIRE(is_ok);
    }

    SECTION("bind accept customized handshake")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        handshake_request req;
        req.set(boost::beast::http::field::user_agent, "radrpc-client");
        req.insert("key", "xxxxxxxxxx");
        cl->set_handshake_request(req);
        auto request_handshake_ok = false;
        REQUIRE(srv->bind_accept([&](radrpc::session_info &info) {
            auto agent_itr = info.request_handshake.find(
                boost::beast::http::field::user_agent);
            auto key_itr = info.request_handshake.find("key");
            if (agent_itr != info.request_handshake.end() &&
                agent_itr->value() == "radrpc-client" &&
                key_itr != info.request_handshake.end() &&
                key_itr->value() == "xxxxxxxxxx")
            {
                request_handshake_ok = true;
                info.response_handshake.set(
                    boost::beast::http::field::user_agent, "radrpc-server");
                info.response_handshake.insert("level", "0");
            }
            return true;
        }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect());
        sleep_ms(defaults::sleep_high_delay_ms); // Wait for result
        srv->stop();
        REQUIRE(request_handshake_ok);
        auto response_handshake_ok = false;
        auto res = cl->get_handshake_response();
        auto agent_itr = res.find(boost::beast::http::field::user_agent);
        auto key_itr = res.find("level");
        if (agent_itr != res.end() && agent_itr->value() == "radrpc-server" &&
            key_itr != res.end() && key_itr->value() == "0")
        {
            response_handshake_ok = true;
        }
        REQUIRE(response_handshake_ok);
    }

    SECTION("reject accept")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::atomic<bool> rejected = ATOMIC_VAR_INIT(false);
        REQUIRE(srv->bind_accept([&](const radrpc::session_info &info) {
            rejected = true;
            return false;
        }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE_FALSE(plain_connect_send_recv());
        sleep_ms(defaults::sleep_high_delay_ms); // Wait for result
        srv->stop();
        REQUIRE(rejected);
    }

    SECTION("reject listen")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        std::atomic<bool> rejected = ATOMIC_VAR_INIT(false);
        REQUIRE(srv->bind_listen([&](const std::string &ip) {
            rejected = true;
            return false;
        }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        // connect_send_recv() could trigger timeout on send
        REQUIRE_FALSE(plain_connect_send_recv());
        sleep_ms(defaults::sleep_high_delay_ms); // Wait for result
        REQUIRE(rejected);
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // Misc
    ////////////////////////////////////////////////////////

    SECTION("close session in bound function")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            // This will timeout 'async_close', since the server doesn't respond
            // back
            REQUIRE(cl->send(UNIT_RPC_CLOSE, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("transfer limit")
    {
        TEST_DINFO("");
        auto session_cfg = default_session_config();
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            std::vector<char> bytes(session_cfg.max_transfer_bytes, 0x0);
            REQUIRE(cl->connect());
            REQUIRE((bool)cl->send_recv(UNIT_RPC_SEND_RECV, bytes));
            bytes.push_back(0x0);
            REQUIRE_FALSE((bool)cl->send_recv(UNIT_RPC_SEND_RECV, bytes));
        }
        srv->stop();
    }

    SECTION("handshake transfer limit")
    {
        TEST_DINFO("");
        auto cfg = default_server_config();
        // This limit will affect the whole request, not just the custom fields
        cfg.max_handshake_bytes = 100;
        auto srv = create_plain_server(cfg);
        auto cl = create_plain_client();
        handshake_request req;
        for (auto i = 0; i < 10; ++i)
            req.insert("field" + std::to_string(i), "xxxxxxxxxx");
        cl->set_handshake_request(req);
        auto got_request = false;
        REQUIRE(srv->bind_accept([&](radrpc::session_info &info) {
            got_request = true;
            return true;
        }));
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE_FALSE(cl->connect());
        sleep_ms(defaults::sleep_high_delay_ms); // Wait for result
        REQUIRE_FALSE(got_request);
        srv->stop();
        auto res = cl->get_handshake_response();
        REQUIRE(res.begin() == res.end());
    }

    SECTION("sessions limit")
    {
        TEST_DINFO("");
        auto srv_cfg = default_server_config();
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            auto clients = vector_of_object(cl);
            for (unsigned int i = 0; i < srv_cfg.max_sessions; ++i)
            {
                clients.emplace_back(create_plain_client());
                REQUIRE(clients[i]->connect());
            }
            REQUIRE_FALSE(cl->connect());
        }
        srv->stop();
    }

    SECTION("connections count")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect());
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(srv->connections() == 1);
        REQUIRE(srv->get_session_ids().size() == 1);
        cl->disconnect();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(srv->connections() == 0);
        REQUIRE(srv->get_session_ids().empty());
        srv->stop();
    }

    SECTION("server mode")
    {
        TEST_DINFO("");
        auto cfg = default_server_config();
        cfg.mode = stream_mode::plain;
        auto srv = create_plain_server(cfg);
        auto cl = create_plain_client();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect());
        srv->stop();
    }
}