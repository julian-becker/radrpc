#!/bin/bash
OUTPUT_DIR="$PWD/files"
cd $PWD


read -p "Certificate validity in days: " VALID_DAYS
read -p "Password for encrypt CA- and server private key: " -s PASS
echo -e ""
read -p "Please confirm the password: " -s PASS_CONFIRM
echo -e ""
if [ "$PASS" != "$PASS_CONFIRM" ]; then
    echo "Password is wrong"
    exit 1
fi


echo "Create certificate authority and its key"
openssl req -x509 -config $PWD/config/openssl-ca.cnf -newkey rsa:2048 -sha256 -passout pass:$PASS -days $VALID_DAYS -out $OUTPUT_DIR/cacert.pem -outform PEM
# [Out] cakey.pem:
# - used for signing certificate requests and should stay private
# [Out] cacert.pem:
# - used at clients to check whether 
#   they're connecting to the correct server


echo "Create certificate signing request and its key"
openssl req -config $PWD/config/openssl-server.cnf -newkey rsa:2048 -sha256 -passout pass:$PASS -days $VALID_DAYS -out $OUTPUT_DIR/servercert.csr -outform PEM
# [Out] servercert.csr:
# - this certficate request will be used later to generate the signed "servercert.pem" certficate
# [Out] serverkey.pem:
# - used at the server for crypto operations on the ssl stream


touch $OUTPUT_DIR/index.txt
echo '01' > $OUTPUT_DIR/serial.txt


echo "Sign server certificate signing request with certificate authority and its key"
openssl ca -passin pass:$PASS -days $VALID_DAYS -config $PWD/config/openssl-ca.cnf -policy signing_policy -extensions signing_req -out $OUTPUT_DIR/servercert.pem -infiles $OUTPUT_DIR/servercert.csr
# [In]  cacert.pem, cakey.pem, servercert.csr
# [Out] servercert.pem:
# - this certficate will be used at the server and is also sent to the client at ssl handshake,
#   so the client may check it with the deployed certficate (cacert.pem)


echo "Create diffie-hellman key"
openssl dhparam -out $OUTPUT_DIR/dh.pem 2048


echo "Done"