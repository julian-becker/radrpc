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

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <radrpc/radrpc.hpp>

#include "ssl_test_files.hpp"




// User defined remote procedure calls
enum MyRpcCommands
{
    MY_RPC_ECHO_MSG,
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




bool on_verify_certificate(bool preverified,
                           boost::asio::ssl::verify_context &verify)
{
    const std::size_t max_subject_size = 256;
    char subject_name[max_subject_size];
    X509 *cert = X509_STORE_CTX_get_current_cert(verify.native_handle());
    X509_NAME_oneline(
        X509_get_subject_name(cert), subject_name, max_subject_size);
    X509_STORE_CTX *ctx = verify.native_handle();




    // See: openssl/x509_vfy.h
    int32_t depth = X509_STORE_CTX_get_error_depth(ctx);
    int32_t error = X509_STORE_CTX_get_error(ctx);
    LOG("on_verify_certificate:\n\t"
        << subject_name << "\n\t[OK:" << preverified << "]\n\t[DEPTH:" << depth
        << "]\n\t[ERROR:" << error << "]");




    // See:
    // https://www.openssl.org/docs/man1.1.0/man3/X509_STORE_CTX_get_error_depth.html
    switch (error)
    {
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            LOG("on_verify_certificate: Unable to get issuer\n");
            break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            LOG("on_verify_certificate: Certificate not yet valid\n");
            break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            LOG("on_verify_certificate: Certificate expired\n");
            break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            LOG("on_verify_certificate: Self signed certificate\n");
            break;
        default:
            break;
    }




    // 'preverified' may be changed to allow / disallow the connection.
    return preverified;
}




int main()
{
    using namespace radrpc;




    // This is just an example of many how to initialize
    // a ssl context.
    ssl::context ssl_ctx(ssl::context::sslv23);
    boost::system::error_code ec;
    // To load from a file, 'load_verify_file()' can be used.
    ssl_ctx.add_certificate_authority(
        boost::asio::buffer(ssl_test_files::client_certificate,
                            std::strlen(ssl_test_files::client_certificate)),
        ec);
    if (ec)
    {
        LOG("add_certificate_authority: " << ec.message());
        return 0;
    }
    // Optionally a callback handler can be passed to
    // inspect the verification procedure.
    ssl_ctx.set_verify_callback(std::bind(
        &on_verify_certificate, std::placeholders::_1, std::placeholders::_2));




    client_config cfg;
    cfg.host_address = "127.0.0.1";
    cfg.port = 3377;
    cfg.max_read_bytes = 0xFFFFFFFF;
    cfg.send_attempts = 0;
    cfg.send_attempt_delay = duration::zero();

    client_timeout timeout;
    timeout.handshake_timeout = std::chrono::seconds(5);
    timeout.send_timeout = std::chrono::seconds(1);
    timeout.response_timeout = std::chrono::seconds(1);

    // Let the client own ssl context.
    client::ssl cl(cfg, timeout, std::move(ssl_ctx));

    LOG("Press enter to connect.");
    std::cin.get();
    // Connect manually.
    // if not used, 'send()' / 'send_recv()'
    // will consume an attempt.
    cl.connect();

    LOG("Please enter your echo message:");
    std::string msg;
    std::getline(std::cin, msg);
    std::vector<char> msg_bytes(msg.begin(), msg.end());
    auto response_bytes = cl.send_recv(MY_RPC_ECHO_MSG, msg_bytes);
    if (!response_bytes.empty())
    {
        std::string response(response_bytes.data(),
                             response_bytes.data() + response_bytes.size());
        LOG("Received message from server: " << response);
    }

    LOG("Press enter to disconnect.");
    std::cin.get();
    cl.disconnect();
    return 0;
}