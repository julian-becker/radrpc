##  SSL files generation
This little guide should just give a quickstart into generating and using ssl files with radrpc.
The provided information are gathered from various [sites](#sources) and is packaged into this tutorial.
I am myself not an expert in cryptography and if you find something is wrong, please report it to the [Issue-Tracker](https://github.com/reapler/radrpc/issues).


The goal is to initialize [boost::asio::ssl_context](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/overview/ssl.html) for client and server which will be later passed to the constructors.
There are a few ways to initialize it, for e.g.:
- Simple RSA encryption with private & public key only
- Self-signed certificate
- Certificate authority
- ...

We will focus on creating our own certificate authority to initialize the ssl context later.

## 
To configure the certificate authority & server certificate you may edit:
- [ssl/config/openssl-ca.cnf](https://github.com/reapler/radrpc/blob/master/tools/ssl/config/openssl-ca.cnf)
- [ssl/config/openssl-server.cnf](https://github.com/reapler/radrpc/blob/master/tools/ssl/config/openssl-server.cnf)

However you can also just execute the script & fill out the requested fields:

unix:
```
./_unix_gen_ssl.sh
```
windows:
```
_win_gen_ssl.bat
```
After the creation you will end up with these relevant certificates & keys located in folder "files":

- **cacert.pem** The certificate authority to use at clients.
- **dh.pem** The diffie-hellman key to use on server.
- **servercert.pem** The certificate to use on server.
- **serverkey.pem** The encryption key to use on server.

The other files are necessary, if you are going to create more certificates from your CA.
For initializing the ssl context it is not needed anymore.
Now you may use these files to initialize the context for client or server like this:
```cpp
// client
boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
boost::system::error_code ec;

ctx.load_verify_file("cacert.pem", ec);
if (ec)
{
    std::cout << "load_verify_file: " << ec.message() << std::endl;
    return 0;
}
```

```cpp
// server
boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
boost::system::error_code ec;

ctx.set_password_callback(
    [&](std::size_t, boost::asio::ssl::context_base::password_purpose) {
        return "your_password_for_the_serverkey";
    },
    ec);
if (ec)
{
    std::cout << "set_password_callback: " << ec.message() << std::endl;
    return 0;
}

ctx.use_certificate_chain_file("servercert.pem", ec);
if (ec)
{
    std::cout << "use_certificate_chain_file: " << ec.message()
                << std::endl;
    return 0;
}

ctx.use_private_key_file(
    "serverkey.pem", boost::asio::ssl::context::file_format::pem, ec);
if (ec)
{
    std::cout << "use_private_key_file: " << ec.message() << std::endl;
    return 0;
}

ctx.use_tmp_dh_file("dh.pem", ec);
if (ec)
{
    std::cout << "use_tmp_dh_file: " << ec.message() << std::endl;
    return 0;
}
```
At last you should make sure to keep the keys & the server related files in a secure place (restricted permissions).

##### Sources
- [https://stackoverflow.com/questions/21297139/how-do-you-sign-a-certificate-signing-request-with-your-certification-authority/21340898#21340898](https://stackoverflow.com/questions/21297139/how-do-you-sign-a-certificate-signing-request-with-your-certification-authority/21340898#21340898)
- [https://www.phildev.net/ssl/opensslconf.html](https://www.phildev.net/ssl/opensslconf.html)
- [https://stackoverflow.com/questions/18843466/whats-a-client-certificate-and-where-to-get-it-how-to-use-it-to-connect-to-ser](https://stackoverflow.com/questions/18843466/whats-a-client-certificate-and-where-to-get-it-how-to-use-it-to-connect-to-ser)
- [http://web.archive.org/web/20110704035103/http://www.tc.umn.edu/~brams006/selfsign.html](http://web.archive.org/web/20110704035103/http://www.tc.umn.edu/~brams006/selfsign.html)
- [https://datacenteroverlords.com/2012/03/01/creating-your-own-ssl-certificate-authority/](https://datacenteroverlords.com/2012/03/01/creating-your-own-ssl-certificate-authority/)
- [https://wiki.openssl.org/index.php/Command_Line_Utilities](https://wiki.openssl.org/index.php/Command_Line_Utilities)
