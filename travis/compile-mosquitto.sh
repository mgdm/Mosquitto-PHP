#!/bin/bash

wget http://mosquitto.org/files/source/mosquitto-1.4.tar.gz
tar xvzf mosquitto-1.4.tar.gz
cd mosquitto-1.4
make
sudo make install

