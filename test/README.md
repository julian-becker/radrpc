##  Stress tests
If you are going to use the stress tests, you may adjust these functions to your needs:
```cpp
// stress_client.cpp
test_suite::client_set default_client_set();
test_suite::server_set default_server_set();
client_config default_client_config();
client_timeout default_client_timeout();
run_time();
```
Hint: If you are going to use many clients or test entries, do this incrementally since a few Sanitizers consumes a lot of memory & processing power.
If you would like, you may also add your own tests for a specific network traffic.
After you have compiled this, you will end up with "stress_client*" and "stress_server*".
One binary can be also run on a extra server, if you are changing the host.
This is overall not perfect, but if you got ideas or see improvements, i would be glad to hear from you.

##  Unit tests
The tests for "unit_common*" are splitted into: 
- defaults/default_plain_client.cpp
- defaults/default_plain_server.cpp
- defaults/default_ssl_client.cpp (if SSL enabled)
- defaults/default_ssl_server.cpp (if SSL enabled)
- _main.cpp
- misc.cpp
- plain_client.cpp
- plain_server.cpp
- ssl_client.cpp (if SSL enabled)
- ssl_server.cpp (if SSL enabled)

The ssl files are actually almost the same as the plain, except a few sections.
The tests for "unit_utils*" are located in "unit_utils.cpp".