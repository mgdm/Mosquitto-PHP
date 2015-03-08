#!/bin/bash

dest_dir=certs

if [ ! -d "$dest_dir" ]; then
    mkdir "$dest_dir"
fi

if [ ! -d "$dest_dir" ]; then
    echo "Unable to create directory: $dest_dir" >&2
    exit 3
fi

cd "$dest_dir"

echo "Making CA key and certificate"
openssl req \
    -new \
    -x509 \
    -days 3650 \
    -extensions v3_ca \
    -subj "/C=GB/ST=Scotland/L=Glasgow/O=Mosquitto-PHP/CN=Mosquitto-PHP Test CA" \
    -keyout ca.key \
    -out ca.crt \
    -nodes

echo "Making server key"

openssl genrsa \
    -out server.key \
    2048

echo "Making server signing request"
openssl req \
    -subj "/C=GB/ST=Scotland/L=Glasgow/O=Mosquitto-PHP/CN=localhost" \
    -out server.csr \
    -key server.key \
    -new

echo "Signing server certificate"
openssl x509 \
    -req \
    -in server.csr \
    -CA ca.crt \
    -CAkey ca.key \
    -CAcreateserial \
    -out server.crt \
    -days 3650

echo "Making encrypted client key"
openssl genrsa \
    -out client-enc.key \
    -des3 \
    -passout pass:Mosquitto-PHP \
    2048

echo "Making clear-text client key"
openssl rsa \
    -in client-enc.key \
    -passin pass:Mosquitto-PHP \
    -out client.key

echo "Making client signing request"
openssl req \
    -subj "/C=GB/ST=Scotland/L=Glasgow/O=Mosquitto-PHP/CN=Mosquitto-PHP Client" \
    -out client.csr \
    -key client.key \
    -new

echo "Signing client certificate"
openssl x509 \
    -req \
    -in client.csr \
    -CA ca.crt \
    -CAkey ca.key \
    -CAcreateserial \
    -out client.crt \
    -days 3650

echo "Created all certificates. Now run mosquitto with the following command line, from the current directory:"
echo "mosquitto -c mosquitto.conf -d"
