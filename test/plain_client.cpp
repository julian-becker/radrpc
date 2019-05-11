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

#include <test/defaults/default_config.hpp>
#include <test/dep/catch.hpp>




TEST_CASE("plain client implementation")
{
    ////////////////////////////////////////////////////////
    // connect() / disconnect()
    ////////////////////////////////////////////////////////

    SECTION("connecting / disconnecting")
    {
        auto srv = plain_create_server();
        auto cl = plain_create_client();
        cl->disconnect();
        REQUIRE_FALSE(cl->connect());
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        REQUIRE(cl->connect());
        cl->disconnect();
        srv->stop();
    }

    SECTION("connect attempts offline (timeout connect)")
    {
        auto cl = plain_create_client();
        REQUIRE_FALSE(cl->connect(3, std::chrono::milliseconds(100)));
    }

    SECTION("connect attempts offline (timeout handshake")
    {
        auto srv = plain_create_server();
        auto cl = plain_create_client();
        // The resulted wait time can also include the timeout from handshake
        REQUIRE_FALSE(cl->connect(3, std::chrono::milliseconds(100)));
        srv->stop();
    }

    SECTION("connect attempts")
    {
        auto srv = plain_create_server();
        auto cl = plain_create_client();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        REQUIRE(cl->connect(3, std::chrono::milliseconds(100)));
        cl->disconnect();
        sleep_ms(sleep_high_delay_ms);
        srv->stop();
    }

    SECTION("reconnect")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
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
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
            REQUIRE(cl->connect());
            REQUIRE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send offline")
    {
        auto cl = plain_create_client();
        REQUIRE_FALSE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
    }

    SECTION("send fill queue & process")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
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
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = plain_create_client(cfg, default_client_timeout());
            REQUIRE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send attempts offline")
    {
        auto srv = plain_create_server();
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = plain_create_client(cfg, default_client_timeout());
            REQUIRE_FALSE(cl->send(UNIT_RPC_SEND, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send exceed transfer")
    {
        auto session_cfg = default_session_config();
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
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
        auto session_cfg = default_session_config();
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.send_timeout = std::chrono::seconds(0);
            auto cl = plain_create_client(default_client_config(), timeout);
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
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
            REQUIRE(cl->connect());
            REQUIRE(
                (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send_recv offline")
    {
        auto cl = plain_create_client();
        REQUIRE_FALSE(
            (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
    }

    SECTION("send_recv fill queue & process")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
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
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = plain_create_client(cfg, default_client_timeout());
            REQUIRE(
                (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send_recv attempts offline")
    {
        auto srv = plain_create_server();
        {
            auto cfg = default_client_config();
            cfg.send_attempts = 3;
            cfg.send_attempt_delay = std::chrono::milliseconds(100);
            auto cl = plain_create_client(cfg, default_client_timeout());
            REQUIRE_FALSE(
                (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send_recv exceed transfer")
    {
        auto session_cfg = default_session_config();
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
            std::vector<char> bytes(session_cfg.max_transfer_bytes, 0x0);
            REQUIRE(cl->connect());
            REQUIRE((bool)cl->send_recv(UNIT_RPC_SEND_RECV, bytes));
            bytes.push_back(0x0);
            REQUIRE(cl->connect());
            REQUIRE_FALSE((bool)cl->send_recv(UNIT_RPC_SEND_RECV, bytes));
        }
        srv->stop();
    }

    SECTION("send_recv send timeout")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.send_timeout = std::chrono::seconds(0);
            auto cl = plain_create_client(default_client_config(), timeout);
            REQUIRE(cl->connect());
            REQUIRE_FALSE(
                (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
        }
        srv->stop();
    }

    SECTION("send_recv response timeout")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
            REQUIRE(cl->connect());
            REQUIRE_FALSE(
                (bool)cl->send_recv(UNIT_RPC_SEND, std::vector<char>()));
            REQUIRE(cl->connect());
            REQUIRE(
                (bool)cl->send_recv(UNIT_RPC_SEND_RECV, std::vector<char>()));
        }
        srv->stop();
    }

    ////////////////////////////////////////////////////////
    // ping()
    ////////////////////////////////////////////////////////

    SECTION("ping")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto cl = plain_create_client();
            REQUIRE(cl->ping());
        }
        srv->stop();
    }

    SECTION("ping timeout")
    {
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        {
            auto timeout = default_client_timeout();
            timeout.response_timeout = std::chrono::seconds(0);
            auto cl = plain_create_client(default_client_config(), timeout);
            REQUIRE_FALSE(cl->ping());
        }
        srv->stop();
    }

    SECTION("ping offline")
    {
        auto cl = plain_create_client();
        {
            auto srv = plain_create_server();
            REQUIRE_FALSE(cl->ping());
            srv->async_start();
            sleep_ms(sleep_high_delay_ms);
            REQUIRE(cl->ping());
            srv->stop();
        }
        REQUIRE_FALSE(cl->ping());
    }
}

TEST_CASE("plain client connect")
{
    SECTION("close io stopped")
    {
        // Special case for calling close in client_connector while io was
        // stopped. This happened while running the stress tests
        auto srv = plain_create_server();
        srv->async_start();
        sleep_ms(sleep_high_delay_ms);
        boost::asio::io_context io_ctx;
        std::thread io_worker;
        handshake_request req_handshake{};
        client_timeout client_timeout = default_client_timeout();
        boost::beast::websocket::stream_base::timeout timeout{};
        timeout.handshake_timeout = client_timeout.handshake_timeout;
        timeout.idle_timeout = duration::max();
        timeout.keep_alive_pings = false;
        auto cfg = default_client_config();
        std::unordered_map<uint32_t, std::function<void(receive_buffer &)>>
            bound_funcs;
        auto ptr = std::make_shared<radrpc::impl::client::session_connect<
            client_streams::plain_stream>>(io_ctx,
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