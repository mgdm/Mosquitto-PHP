#!/bin/sh -x
set -e
$HOME/bin/phpize
./configure --with-php-config=$HOME/bin/php-config --with-mosquitto=$HOME
make -j2 --quiet
make install
echo "extension=mosquitto.so" > $HOME/php.d/mosquitto.ini
