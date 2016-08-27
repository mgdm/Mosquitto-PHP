#!/usr/bin/env sh
set -e
mkdir -p $HOME/php
git clone https://github.com/php/php-src $HOME/php/src
cd $HOME/php/src
git checkout PHP-7.0
./buildconf --force
./configure \
    --prefix=$HOME \
    --disable-all \
    --enable-debug \
    --with-config-file-path=$HOME \
    --with-config-file-scan-dir=$HOME/php.d \
    --enable-json \
    --enable-phar \
    --with-openssl

make -j2 --quiet install
cp php.ini-development $HOME
mkdir $HOME/php.d

