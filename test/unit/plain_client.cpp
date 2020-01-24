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
#include <radrpc/impl/client/connector.hpp>
#include "radrpc/common/connection_state.hpp"

#define CATCH_CONFIG_CONSOLE_WIDTH 300
#include "catch.hpp"

#include <test/core/defaults.hpp>
#include <test/core/log.hpp>
#include <test/core/sleep.hpp>
#include <test/unit/construct/rpc_command.hpp>
#include <test/unit/construct/client.hpp>
#include <test/unit/construct/server.hpp>

using namespace test::unit::construct;




TEST_CASE("plain client implementation")
{
    ////////////////////////////////////////////////////////
    // connect() / disconnect()
    ////////////////////////////////////////////////////////

    SECTION("connecting / disconnecting")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        cl->disconnect();
        REQUIRE_FALSE(cl->connect());
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect());
        cl->disconnect();
        srv->stop();
    }

    SECTION("connect attempts offline (timeout connect)")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        REQUIRE_FALSE(cl->connect(3, std::chrono::milliseconds(100)));
    }

    SECTION("connect attempts offline (timeout handshake")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        // The resulted wait time can also include the timeout from handshake
        REQUIRE_FALSE(cl->connect(3, std::chrono::milliseconds(100)));
        srv->stop();
    }

    SECTION("connect attempts")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        auto cl = create_plain_client();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect(3, std::chrono::milliseconds(100)));
        cl->disconnect();
        sleep_ms(defaults::sleep_high_delay_ms);
        srv->stop();
    }

    SECTION("reconnect")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            cl->disconnect();
            REQUIRE(cl->connect());
        }
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // send()
    ////////////////////////////////////////////////////////

    SECTION("send")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            REQUIRE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send offline")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        REQUIRE_FALSE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
    }

    SECTION("send fill queue & process")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            std::vector<std::thread> workers;
            workers.reserve(10);
            REQUIRE(cl->connect());
            for (auto i = 0; i < 10; ++i)
            {
                workers.emplace_back(
                    [&] { cl->send(UNIT_RPC_SEND, std::vector<char>()); });
            }
            for (auto &worker : workers)
                worker.join();
        }
        srv->stop();
    }

    SECTION("send attempts")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = create_plain_client(cfg, default_client_timeout());
            REQUIRE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send attempts offline")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = create_plain_client(cfg, default_client_timeout());
            REQUIRE_FALSE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send exceed transfer")
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
            REQUIRE(cl->send(UNIT_RPC_SEND, bytes));
            bytes.push_back(0x0);
            REQUIRE(cl->connect());
            REQUIRE(cl->send(UNIT_RPC_SEND, bytes));
        }
        srv->stop();
    }

    SECTION("send timeout")
    {
        TEST_DINFO("");
        auto session_cfg = default_session_config();
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.send_timeout = std::chrono::seconds(0);
            auto cl = create_plain_client(default_client_config(), timeout);
            std::vector<char> bytes(session_cfg.max_transfer_bytes, 0x0);
            REQUIRE(cl->connect());
            REQUIRE_FALSE(cl->send(UNIT_RPC_SEND, bytes));
        }
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // send_recv()
    ////////////////////////////////////////////////////////

    SECTION("send_recv")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            REQUIRE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
        }
        srv->stop();
    }

    SECTION("send_recv offline")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        REQUIRE_FALSE(
            static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
    }

    SECTION("send_recv fill queue & process")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            std::vector<std::thread> workers;
            workers.reserve(10);
            REQUIRE(cl->connect());
            for (auto i = 0; i < 10; ++i)
            {
                workers.emplace_back([&] {
                    cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>());
                });
            }
            for (auto &worker : workers)
                worker.join();
        }
        srv->stop();
    }

    SECTION("send_recv attempts")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = create_plain_client(cfg, default_client_timeout());
            REQUIRE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
        }
        srv->stop();
    }

    SECTION("send_recv attempts offline")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = create_plain_client(cfg, default_client_timeout());
            REQUIRE_FALSE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
        }
        srv->stop();
    }

    SECTION("send_recv exceed transfer")
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
            REQUIRE(static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, bytes)));
            bytes.push_back(0x0);
            REQUIRE(cl->connect());
            REQUIRE_FALSE(static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, bytes)));
        }
        srv->stop();
    }

    SECTION("send_recv send timeout")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.send_timeout = std::chrono::seconds(0);
            auto cl = create_plain_client(default_client_config(), timeout);
            REQUIRE(cl->connect());
            REQUIRE_FALSE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
        }
        srv->stop();
    }

    SECTION("send_recv response timeout")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->connect());
            REQUIRE_FALSE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND, std::vector<char>())));
            REQUIRE(cl->connect());
            REQUIRE(
                static_cast<bool>(cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>())));
        }
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // ping()
    ////////////////////////////////////////////////////////

    SECTION("ping")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto cl = create_plain_client();
            REQUIRE(cl->ping());
        }
        srv->stop();
    }

    SECTION("ping timeout")
    {
        TEST_DINFO("");
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.response_timeout = std::chrono::seconds(0);
            auto cl = create_plain_client(default_client_config(), timeout);
            REQUIRE_FALSE(cl->ping());
        }
        srv->stop();
    }

    SECTION("ping offline")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        {
            auto srv = create_plain_server();
            REQUIRE_FALSE(cl->ping());
            srv->async_start();
            sleep_ms(defaults::sleep_high_delay_ms);
            REQUIRE(cl->ping());
            srv->stop();
        }
        REQUIRE_FALSE(cl->ping());
    }

    ////////////////////////////////////////////////////////
    // listen_broadcast()
    ////////////////////////////////////////////////////////

    SECTION("bind max call_id")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        REQUIRE_FALSE(cl->listen_broadcast(config::max_call_id + 1,
                                           [&](receive_buffer &p_data) {}));
        REQUIRE_FALSE(cl->listen_broadcast(config::max_call_id,
                                           [&](receive_buffer &p_data) {}));
        REQUIRE(cl->listen_broadcast(config::max_call_id - 1,
                                           [&](receive_buffer &p_data) {}));
    }

    SECTION("listen broadcast min max call_id")
    {
        TEST_DINFO("");
        auto cl = create_plain_client();
        std::vector<char> bytes(1, 0x0);
        std::atomic<int> received = ATOMIC_VAR_INIT(0);
        REQUIRE(cl->listen_broadcast(config::max_call_id - 1,
                                     [&](receive_buffer &p_data) {
                                         received++;
                                     }));
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(cl->connect());
        sleep_ms(defaults::sleep_high_delay_ms);
        srv->broadcast(config::max_call_id - 1, bytes);
        srv->broadcast(0, bytes);
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(received == 1);
        cl->disconnect();
        REQUIRE(cl->listen_broadcast(
            0, [&](receive_buffer &p_data) { received++; }));
        REQUIRE(cl->connect());
        sleep_ms(defaults::sleep_high_delay_ms);
        srv->broadcast(config::max_call_id - 1, bytes);
        srv->broadcast(0, bytes);
        sleep_ms(defaults::sleep_high_delay_ms);
        REQUIRE(received == 3);
    }
}

TEST_CASE("plain client connect")
{
    SECTION("close io stopped")
    {
        TEST_DINFO("");
        // Special case for calling close in client_connector while io was
        // stopped. This happened while running the stress tests
        auto srv = create_plain_server();
        srv->async_start();
        sleep_ms(defaults::sleep_high_delay_ms);
        boost::asio::io_context io_ctx;
        std::thread io_worker;
        handshake_request req_handshake{};
        client_timeout client_timeout = default_client_timeout();
        boost::beast::websocket::stream_base::timeout timeout{};
        timeout.handshake_timeout = client_timeout.handshake_timeout;
        timeout.idle_timeout = duration::max();
        timeout.keep_alive_pings = false;
        auto cfg = default_client_config();
        std::function<void(receive_buffer &)>
            bound_funcs[config::max_call_id]{};

            
        auto ptr = std::make_shared<
            radrpc::impl::client::connector<streams::plain>>(
            io_ctx,
            io_worker,
            cfg,
            client_timeout,
            timeout,
            req_handshake,
            bound_funcs);
        REQUIRE(ptr->run() == connection_state::established);
        io_ctx.stop();
        ptr->close();


        if (io_worker.joinable())
            io_worker.join();
        srv->stop();
    }
}