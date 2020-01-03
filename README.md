# radrpc ![MIT](https://img.shields.io/badge/license-MIT-blue.svg)
- [Introduction](#introduction)
- [Design goals](#design-goals)
- [Features](#features)
- [Examples](#examples)
- [Supported compilers](#supported-compilers)
- [Requirements](#requirements)
- [Project integration](#project-integration)
- [Building tests & examples](#building-tests-and-examples)
- [License](#license)
- [Help](#help)
- [Thanks](#thanks)

## Introduction
Radrpc is a high-level remote procedure call library build upon Asio & Beast.
It is header-only C++14 and represents a asynchronous server & synchronous client.

## Design goals
Radrpc aims for usability & a simple client-server architecture without sacrificing the advanced aspect of a remote procedure call library.
- **Simple** Just a few types are needed to run the server or client. Since the public exposed part of the library is straightforward, the user doesn't need to care about the little details behind the scenes.
- **Intuitive usage** The user relevant functions do what you would except, doesn't matter in which context it was called from.
- **Robust testing** Functionality of the library was unit- & stress tested with [Valgrind](http://www.valgrind.org/) and [Sanitizers](https://clang.llvm.org/docs/index.html). Unit tests ensures everything is working as expected, stress tests will cover different execution paths to ensure thread & memory safety.

## Features
The following features are available:
- **Adjustable timeouts** Different timeouts can be set for handshake, send or response.
- **Serialize yourself** A 'bring your own serializer' design for sending & receiving bytes.
- **Keep alive** Server-side keep alive checks the activity & disconnects inactive sessions.
- **Reconnect** Adjustable reconnect attempts if disconnected.
- **Session based** Each connection represents a session with an id which allows to operate on.
- **Config friendly** Advanced configurations for client, sessions and server.
- **Customizable handshake** Adjust & inspect the handshake for validating the session or passing additional data for e.g. a key
- **SSL** Allows to use an encrypted stream with client and server. The server supports a dual mode for accepting only plain, ssl or both streams on the same port.
- **Broadcast** Send messages to a single session or to all sessions specified with an id.

## Examples
For advanced examples and usage of all features, please take a look at [examples/advanced](https://github.com/reapler/radrpc/tree/master/examples/advanced) and [examples/ssl](https://github.com/reapler/radrpc/tree/master/examples/ssl)

Minimal client:
```cpp
#include  "radrpc.hpp"

// All available remote procedure calls
enum RpcCommands
{
    RPC_ECHO_MSG,
};

int main()
{
    using  namespace  radrpc;

    client_config cfg;
    cfg.host_address  =  "127.0.0.1";
    cfg.port  =  3377;
    cfg.max_read_bytes  =  0xFFFFFFFF;
    cfg.send_attempts  =  0;
    cfg.send_attempt_delay  = duration::zero();

    client_timeout timeout;
    timeout.handshake_timeout  = std::chrono::seconds(5);
    timeout.send_timeout  = std::chrono::seconds(1);
    timeout.response_timeout  = std::chrono::seconds(1);
    
    client<client_mode::plain> cl(cfg, timeout);

    // Connect manually.
    // if not used, 'send()' / 'send_recv()'
    // will consume an attempt.
    cl.connect();

    // Of course you would use a serializer
    // instead of using raw strings.
    std::string msg;
    std::vector<char> msg_bytes(msg.begin(), msg.end());
    auto response_bytes = cl.send_recv(RPC_ECHO_MSG, msg_bytes);
    if  (!response_bytes.empty())
    {
        std::string response(
                response_bytes.data(), 
                response_bytes.data()  + response_bytes.size());
        printf("Received message from server: %s\n", response.c_str());
    }
    else
    {
        printf("No response received from server\n");
    }
  
    cl.disconnect();
    return  0;
}
```
Minimal server:
```cpp
#include  "radrpc.hpp"

// All available remote procedure calls
enum RpcCommands
{
    RPC_ECHO_MSG,
};

int main()
{
    using  namespace  radrpc;
    server_config cfg;
    cfg.host_address  =  "0.0.0.0";
    cfg.port  =  3377;
    cfg.workers  =  2;
    cfg.max_sessions  =  1000;
    cfg.max_handshake_bytes  =  0xFFFF;
    cfg.mode = server_mode::plain;
    
    server_timeout timeout;
    timeout.handshake_or_close_timeout  = std::chrono::seconds(5);
    
    session_config default_session_cfg;
    default_session_cfg.max_transfer_bytes  =  0xFFFFFFFF;
    default_session_cfg.ping_delay  = std::chrono::seconds(30);

    server srv(cfg, timeout, default_session_cfg);

    srv.bind(RPC_ECHO_MSG, [&](session_context *ctx) {
        // Of course you would use a serializer
        // instead of using raw strings.
        std::string received(ctx->data(), ctx->data()  + ctx->size());
        printf("Received message: %s\n", received.c_str());
        
        // Assign the response.
        ctx->response = std::vector<char>(
                ctx->data(), 
                ctx->data()  + ctx->size());
    });

    srv.start();    // Blocks until stopped, or got a signal with SIGINT or
                    // SIGTERM
    srv.stop();
}
```

## Supported compilers
The following compilers are tested:
- GCC 7.3
- Clang 6.0
- Microsoft Visual C++ 2017 / Build Tools 15.5.180.51428

Other versions or compilers might also work flawless as long it supports C++14.

## Requirements
Radrpc requires the following libraries:
- [Boost 1.70.0](https://www.boost.org/users/download/) (Asio+Beast)
- [OpenSSL 1.1.1](https://www.openssl.org/source/) (if using SSL)

## Project integration
You can clone this repository and add the include folder to your project & use it like this:
```cpp
// You can configure radrpc like this:
//#define RADRPC_SSL_SUPPORT    // Enables SSL support.
//#define RADRPC_LOGGING        // Enables logging facility.
#include "radrpc.hpp"
```
It is also necessary to include / link Boost and OpenSSL if ssl support is desired.
An example on how configure it with CMake can be found [here](https://github.com/reapler/radrpc/tree/master/CMakeLists.txt).

## Building tests and examples
To build the tests yourself Boost, OpenSSL, CMake & Git are required. 
Please make sure the environment variables of the used libraries are also set correctly to work with CMake.
```
git clone --recursive https://github.com/reapler/radrpc.git
cd radrpc
mkdir build
cd build
cmake ..
make
```

This will create the following binaries:
```
// Examples
client
server
ssl_client
ssl_server
advanced_client
advanced_server

// Tests
stress_client*
stress_server*
unit_common*
unit_utils*
```
Each test binary is created with a different postfix for e.g. (_thread, _valgrind, _address, etc...).
Now you can test these binaries with the created shell scripts.
For creating a coverage report for example (only supports GCC & Clang), you may execute "s_unit_common_cover.sh".

## License
```
MIT License 

Copyright (c) 2020 reapler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
```
## Help
Your opinion matters. To improve radrpc robustness, usage, features & overall code quality 
please post your suggestions on the [Issue-Tracker](https://github.com/reapler/radrpc/issues).

## Thanks
Radrpc would not be here without the effort of these libraries & the people who maintain it:
- [Boost](https://www.boost.org/) Must have library for all common tasks.
- [Asio](https://think-async.com/Asio/) Providing the foundation for networking.
- [Beast](https://github.com/boostorg/beast)  Offers the used websocket protocol plus some great features & examples.
- [rpclib](https://github.com/rpclib/rpclib) Influenced the design of radrpc.