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

#ifndef RADRPC_TEST_FILES_HPP
#define RADRPC_TEST_FILES_HPP

namespace ssl_test_files {

constexpr auto client_certificate =
    R"(
-----BEGIN CERTIFICATE-----
MIIEJjCCAw6gAwIBAgIUDOECvBbjuITKug0kzF0OxnRx9xgwDQYJKoZIhvcNAQEL
BQAwgZsxCzAJBgNVBAYTAlVTMRUwEwYDVQQIDAxQZW5uc3lsdmFuaWExFTATBgNV
BAcMDFBoaWxhZGVscGhpYTESMBAGA1UECgwJcmFkcnBjIENBMRcwFQYDVQQLDA5T
ZXJ2ZXIgdGVzdGluZzEQMA4GA1UEAwwHVGVzdCBDQTEfMB0GCSqGSIb3DQEJARYQ
dGVzdEBleGFtcGxlLmNvbTAeFw0xOTA1MDEyMTU0MTRaFw0xOTA1MzEyMTU0MTRa
MIGbMQswCQYDVQQGEwJVUzEVMBMGA1UECAwMUGVubnN5bHZhbmlhMRUwEwYDVQQH
DAxQaGlsYWRlbHBoaWExEjAQBgNVBAoMCXJhZHJwYyBDQTEXMBUGA1UECwwOU2Vy
dmVyIHRlc3RpbmcxEDAOBgNVBAMMB1Rlc3QgQ0ExHzAdBgkqhkiG9w0BCQEWEHRl
c3RAZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCZ
FR0E8zwqLJNTC0HKEoirXHXbRBrfzfOWXZKeLAp4+JnqIKR1cKobRzM+iq/upKKh
f8bPLXGSBfeHlDN8RSRGqOFQPWgjTMR4wbmd2cFX8pN6MZ4+RHIK3/mq2EScgcm8
IPHG7LaHPjXhuSLP31mU05YOD3uIJEhb/bImzd8BC+oStqDVK8MG/6ni4F5RSLs7
VYj3ZhvlHRHmQ8oKOZGWcSnH0yGwysUb1Br5CPdVsr8OBFsKxCJfLZZEuztM5VWI
/flowD7C4BFrnkj2AgSpaiHd4q+oWYcdx4mBu2hg7GEfjieebXO1YHAwxbkwOAiq
/s6dVV+qqUgZLvzwMtcJAgMBAAGjYDBeMB0GA1UdDgQWBBRrwUzRjxPPTmuBlrZ4
EGYFlG1wXjAfBgNVHSMEGDAWgBRrwUzRjxPPTmuBlrZ4EGYFlG1wXjAPBgNVHRMB
Af8EBTADAQH/MAsGA1UdDwQEAwIBBjANBgkqhkiG9w0BAQsFAAOCAQEATHYstCRB
Q1xFxxYa5cHVj9RTV5p7Fjz12Vrd5+kt1VKwNwF0Qw8omgTVYAKpZGo5Kdc2/5rn
LH5rn3FBc9Tmyp7KEuXIITXjDfghhnIla3cFPuLUrHmLaC83KfqDrR4bTQlpWneC
PAppHmwYKRXWIQ5NeWmfplLDtNJPqB0juHiz050ti13+xI2xks6XlJIub+BEkdlJ
PDl0gOIi5h6ahK/1hUg2+FLywr5H8OlUfBoIABnmCxH8n5U5khwXGd8Fnye8uFU2
ZO5KhqHqShFsFBTu0p7gvyy9OAWmC+dwVJXH8vKAdps2evY18Ljg/YEX8K2xqTW8
7kWteuwe0ek1Kg==
-----END CERTIFICATE-----
)";

constexpr auto server_certificate =
    R"(
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 1 (0x1)
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: C=US, ST=Pennsylvania, L=Philadelphia, O=radrpc CA, OU=Server testing, CN=Test CA/emailAddress=test@example.com
        Validity
            Not Before: May  1 21:54:18 2019 GMT
            Not After : Apr 28 21:54:18 2029 GMT
        Subject: C=US, ST=PA, L=Philadelphia, O=radrpc CA, CN=Test Server
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:a3:5a:9c:84:a0:b4:01:a2:50:ad:88:f9:b9:e5:
                    f1:97:19:9b:2a:78:00:f3:76:7d:2b:d8:62:60:77:
                    33:28:f2:ad:77:75:a7:10:c9:0a:28:05:e2:bc:64:
                    d8:42:9d:52:0b:c6:74:38:01:70:7a:7e:26:69:01:
                    b2:9a:8e:ba:99:21:2a:6e:0d:6f:fa:3b:ab:65:5b:
                    be:dc:d6:6d:b6:aa:cd:7f:8d:a0:38:f8:0d:ea:39:
                    ad:3c:a0:ca:c9:f7:26:a1:64:c3:06:d1:f1:e2:91:
                    89:bd:58:b6:4f:4b:e1:1d:db:14:12:2f:28:2b:6d:
                    2b:a6:46:74:d7:83:7f:56:0f:f1:cc:dd:4a:32:26:
                    b6:24:da:b2:7a:87:b7:03:1b:4f:91:8e:cb:09:d4:
                    de:cf:7d:aa:33:8e:8c:d5:21:25:25:e6:0c:c7:75:
                    d1:76:b1:24:34:ec:33:3c:6e:bb:ed:ab:1b:ea:ed:
                    28:8f:0d:0c:e9:80:d5:40:40:f7:ee:b7:b7:a0:f4:
                    78:8c:97:5b:5e:d1:31:c8:be:92:f2:d2:80:7d:0f:
                    5b:33:19:8e:7a:9e:b2:9f:2b:0d:6c:19:a5:94:8c:
                    14:39:a9:11:1d:60:30:05:5a:35:87:fb:6c:a7:22:
                    e5:a8:5a:f9:83:ca:27:93:a7:ec:2e:cd:9e:bb:ef:
                    64:81
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Key Identifier: 
                42:8E:5B:30:E8:0C:4D:F5:0F:E5:8B:9C:C0:D3:E7:51:18:29:AB:4B
            X509v3 Authority Key Identifier: 
                keyid:6B:C1:4C:D1:8F:13:CF:4E:6B:81:96:B6:78:10:66:05:94:6D:70:5E

            X509v3 Basic Constraints: 
                CA:FALSE
            X509v3 Key Usage: 
                Digital Signature, Key Encipherment
            X509v3 Subject Alternative Name: 
                DNS:example.com, DNS:www.example.com, DNS:mail.example.com, DNS:ftp.example.com, IP Address:127.0.0.1, IP Address:0:0:0:0:0:0:0:1
            Netscape Comment: 
                OpenSSL Generated Certificate
    Signature Algorithm: sha256WithRSAEncryption
         5e:0f:b6:c4:ed:8f:94:0f:02:e8:3a:65:77:b0:32:d8:3c:1b:
         9a:19:9a:d6:47:aa:5d:57:13:a8:6c:93:b2:b9:77:aa:87:c6:
         40:f0:fd:dd:01:ed:9c:ef:0e:fc:c6:75:fa:a2:b5:04:84:17:
         f7:b8:be:5b:01:bb:d8:82:e1:31:15:54:89:d5:8a:ab:9d:93:
         7c:8d:02:e6:0f:52:a6:74:9e:2e:7a:96:9c:bd:ef:25:22:16:
         3d:e3:a0:16:9a:50:f3:dd:e1:c0:cd:6c:c2:38:b8:fc:75:8a:
         b1:4d:0d:f8:c5:d0:20:c3:1b:66:3f:49:6e:d4:36:49:38:72:
         b1:a7:55:09:32:16:36:5c:df:9c:2d:66:65:ad:24:cd:1a:48:
         4f:13:b9:cf:fb:34:5c:f0:01:7c:1b:02:d7:6e:e9:8d:a9:9e:
         68:70:5d:c4:d8:3d:1e:24:2b:f6:e2:3c:df:6a:8d:88:4d:17:
         3f:e7:c4:95:fa:16:24:a4:da:a8:2f:95:ee:fc:3b:9a:7e:70:
         65:88:0d:77:a3:03:ac:a8:d2:66:5e:7a:45:d8:c2:1a:77:e2:
         23:49:fb:ba:61:84:d0:f5:dc:eb:56:61:10:3c:45:c1:31:6b:
         6d:64:34:35:a0:b0:3d:87:6c:90:b5:ba:5a:37:c9:4e:69:86:
         4d:8d:ec:35
-----BEGIN CERTIFICATE-----
MIIEYDCCA0igAwIBAgIBATANBgkqhkiG9w0BAQsFADCBmzELMAkGA1UEBhMCVVMx
FTATBgNVBAgMDFBlbm5zeWx2YW5pYTEVMBMGA1UEBwwMUGhpbGFkZWxwaGlhMRIw
EAYDVQQKDAlyYWRycGMgQ0ExFzAVBgNVBAsMDlNlcnZlciB0ZXN0aW5nMRAwDgYD
VQQDDAdUZXN0IENBMR8wHQYJKoZIhvcNAQkBFhB0ZXN0QGV4YW1wbGUuY29tMB4X
DTE5MDUwMTIxNTQxOFoXDTI5MDQyODIxNTQxOFowWzELMAkGA1UEBhMCVVMxCzAJ
BgNVBAgMAlBBMRUwEwYDVQQHDAxQaGlsYWRlbHBoaWExEjAQBgNVBAoMCXJhZHJw
YyBDQTEUMBIGA1UEAwwLVGVzdCBTZXJ2ZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQCjWpyEoLQBolCtiPm55fGXGZsqeADzdn0r2GJgdzMo8q13dacQ
yQooBeK8ZNhCnVILxnQ4AXB6fiZpAbKajrqZISpuDW/6O6tlW77c1m22qs1/jaA4
+A3qOa08oMrJ9yahZMMG0fHikYm9WLZPS+Ed2xQSLygrbSumRnTXg39WD/HM3Uoy
JrYk2rJ6h7cDG0+RjssJ1N7PfaozjozVISUl5gzHddF2sSQ07DM8brvtqxvq7SiP
DQzpgNVAQPfut7eg9HiMl1te0THIvpLy0oB9D1szGY56nrKfKw1sGaWUjBQ5qREd
YDAFWjWH+2ynIuWoWvmDyieTp+wuzZ6772SBAgMBAAGjge0wgeowHQYDVR0OBBYE
FEKOWzDoDE31D+WLnMDT51EYKatLMB8GA1UdIwQYMBaAFGvBTNGPE89Oa4GWtngQ
ZgWUbXBeMAkGA1UdEwQCMAAwCwYDVR0PBAQDAgWgMGIGA1UdEQRbMFmCC2V4YW1w
bGUuY29tgg93d3cuZXhhbXBsZS5jb22CEG1haWwuZXhhbXBsZS5jb22CD2Z0cC5l
eGFtcGxlLmNvbYcEfwAAAYcQAAAAAAAAAAAAAAAAAAAAATAsBglghkgBhvhCAQ0E
HxYdT3BlblNTTCBHZW5lcmF0ZWQgQ2VydGlmaWNhdGUwDQYJKoZIhvcNAQELBQAD
ggEBAF4PtsTtj5QPAug6ZXewMtg8G5oZmtZHql1XE6hsk7K5d6qHxkDw/d0B7Zzv
DvzGdfqitQSEF/e4vlsBu9iC4TEVVInViqudk3yNAuYPUqZ0ni56lpy97yUiFj3j
oBaaUPPd4cDNbMI4uPx1irFNDfjF0CDDG2Y/SW7UNkk4crGnVQkyFjZc35wtZmWt
JM0aSE8Tuc/7NFzwAXwbAtdu6Y2pnmhwXcTYPR4kK/biPN9qjYhNFz/nxJX6FiSk
2qgvle78O5p+cGWIDXejA6yo0mZeekXYwhp34iNJ+7phhND13OtWYRA8RcExa21k
NDWgsD2HbJC1ulo3yU5phk2N7DU=
-----END CERTIFICATE-----

)";

constexpr auto server_key =
    R"(
-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCjWpyEoLQBolCt
iPm55fGXGZsqeADzdn0r2GJgdzMo8q13dacQyQooBeK8ZNhCnVILxnQ4AXB6fiZp
AbKajrqZISpuDW/6O6tlW77c1m22qs1/jaA4+A3qOa08oMrJ9yahZMMG0fHikYm9
WLZPS+Ed2xQSLygrbSumRnTXg39WD/HM3UoyJrYk2rJ6h7cDG0+RjssJ1N7Pfaoz
jozVISUl5gzHddF2sSQ07DM8brvtqxvq7SiPDQzpgNVAQPfut7eg9HiMl1te0THI
vpLy0oB9D1szGY56nrKfKw1sGaWUjBQ5qREdYDAFWjWH+2ynIuWoWvmDyieTp+wu
zZ6772SBAgMBAAECggEAetPhKKCleS0ROMrr62oI9DKex5ogLPCFQficUCiQ4VNI
N7DRg/+GkX2LdVjme4BLdusr7Ai5CIHeY8qQ/0Ie8JFNlB6qeh9vyvDnXKonKNjZ
V/Wn6e4qmFnx9sp4cNBTzbs0Ieau2EvSo/sWah2GEarvNpUawTx/O7FynbIUhyKW
db7XQ/M1kY8GvoaE3zQZL0ZJ9MKvgEdAHSuQb9ZexSO2IQ+ng6jyGs6T5JPaNz07
G5QprCifS5t31tPZNjYvy66Omfc7ei3MJNAnn26RAlxQPuX2bD4z4LD7ps36AU0R
zdOeVVpldDjmB+8pAFWWMxd52kjebQYjU622SRYcgQKBgQDQ/EEF9JQeJG3JyNNH
0qPUkcge7/4nzqd97dFAf6y1q8zJwvfha4hc2jOj8uGB2DuvRjnoJZeSBMCyIIkb
SstQH90SJZN4xAV7VEJOp2dnCeTqtvBZwMrdv06tKvclkkE0MFsOojJjhlvGWEih
D92mFKGkT/9Ztwf+KilV8C8xeQKBgQDIGmIeYrGZoQASYZyk8DsCKpNJg82nRItn
e6ettlkbRK5XfgZ4V/XD0AqoeiWs+z6u1YQwS5tYG0YBWCiSnxW3DRBn8qMlf655
Nocx4A6i377BAwhrJdbIHGDY3O9rHkxlkHtnJSfx7Zq1oFya5+Tjb0mtN69Jo4n5
StItZKtRSQKBgQCxVu3inNo3/dfAXI+VqhRmJNM6FZOJuh59iRqBbhZkD4Z3S+BH
RJXxnSQnIsrkAVccdC5dZdEKEshmH5/ZLg37Tx6s8IAFAepY7nZaAkqHsGi2GEk7
EJPVuiqMXiJdmo+ENuh+MXsq42/rjy0Kp8kBzw4fvdbFDcFVl/d9upSBmQKBgQCF
8QMUi8ONrjM6GpSb4yHchVsprkwOVtdpYWAP6ysiEYsRjY4PJ/GeLLe1pm1pqJOA
u/NL0JAfHcJyS+x8mHgUm24L5mJGS30iLdm5DN0/ML2ivMD51845DKXXA6xO62z/
3wJ7PGfMjNsuEDVsiySvCGMsy0VN6WYFtIuTTvtN4QKBgDJVM3sjXD5aOTSkpwOr
cYGC9T2r8n021cIq9HxcF3idO0anvHfSSmXAykZN0FMn2pPqBLaLptWLJwg2k64z
lpoNYL3tz8NlCPpNdeqOZa0V9JKgle2RmZ4IjjHnU0OCcd3Ds+X6yKa/DDNVfSgA
BLmWQSlmka6EG2EkKUKObt4F
-----END PRIVATE KEY-----
)";

constexpr auto server_key_pass = "";

constexpr auto dh_key =
    R"(
-----BEGIN DH PARAMETERS-----
MIIBCAKCAQEAjWLyepo8shXNRbL1Cp3clFJ1Tso07iVQFDnYodj7fZ/dBeuvC3FN
b1soXHMe1Nd6fl+2BwdxhbBgZz4NWw1fXaF+W6VHHCTPwjwJOP1cd8v28Ewbt8ee
sXV98dbYP4LzoKKb3xYpICtXd0nyzYzvut7HlxkKdPbgKmwX0CmdTD/V+/q+KM2v
z2rQ+SVJMSYc8f8pi50E0KuXNr18umC002vSO01EIU2bbwF9fiCcEWmtwB7H0HvI
1F5+hGBUcJQ/eCv/vpmI+P15c2ElbpEwqORrHTrZBq96LdtltERsH7xTR5BKDCQt
w2h4KUAAu0qxvVPut32alNDGM/cLPbTZwwIBAg==
-----END DH PARAMETERS-----
)";

} // namespace ssl_test_files

#endif // RADRPC_TEST_FILES_HPP