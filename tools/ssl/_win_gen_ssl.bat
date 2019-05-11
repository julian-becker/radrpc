@echo off
set OUTPUT_DIR="%cd%\files"


@echo off
set /p VALID_DAYS=Certificate validity in days: 

@echo off
powershell -Command $pword = read-host "Password for encrypt CA- and server private key" -AsSecureString ; ^
    $BSTR=[System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($pword) ; ^
        [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR) > .tmp.txt 
set /p PASS=<.tmp.txt & del .tmp.txt

@echo off
powershell -Command $pword = read-host "Please confirm the password" -AsSecureString ; ^
    $BSTR=[System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($pword) ; ^
        [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR) > .tmp.txt 
set /p PASS_CONFIRM=<.tmp.txt & del .tmp.txt

if "%PASS%"=="%PASS_CONFIRM%" (
 @echo off
) else (
 echo Password is wrong
 goto eof
)


echo Create certificate authority and its key
openssl req -x509 -config %cd%\config\openssl-ca.cnf -newkey rsa:2048 -sha256 -passout pass:%PASS% -days %VALID_DAYS% -out %OUTPUT_DIR%\cacert.pem -outform PEM
:: [Out] cakey.pem:
:: - used for signing certificate requests & should stay private
:: [Out] cacert.pem:
:: - used at clients to check whether 
::   they're connecting to the correct server


echo Create certificate signing request and its key
openssl req -config %cd%\config\openssl-server.cnf -newkey rsa:2048 -sha256 -passout pass:%PASS% -days %VALID_DAYS% -out %OUTPUT_DIR%\servercert.csr -outform PEM
:: [Out] servercert.csr:
:: - this certficate request will be used later to generate the signed "servercert.pem" certficate
:: [Out] serverkey.pem:
:: - used at the server for crypto operations on the ssl stream


@echo off
  break>%OUTPUT_DIR%\index.txt
@echo 01> %OUTPUT_DIR%\serial.txt


echo Sign server certificate signing request with certificate authority and its key
openssl ca -passin pass:%PASS% -days %VALID_DAYS% -config %cd%\config\openssl-ca.cnf -policy signing_policy -extensions signing_req -out %OUTPUT_DIR%\servercert.pem -infiles %OUTPUT_DIR%\servercert.csr
:: [In]  cacert.pem, cakey.pem, servercert.csr
:: [Out] servercert.pem:
:: - this certficate will be used at the server and is also sent to the client at ssl handshake,
::   so the client may check it with the deployed certficate (cacert.pem)


echo Create diffie-hellman key
openssl dhparam -out %OUTPUT_DIR%\dh.pem 2048


echo Done
:eof