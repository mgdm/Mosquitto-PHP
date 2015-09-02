#!/bin/sh

set -e

cd $TRAVIS_BUILD_DIR

cd tests
./makeTestCerts.sh
mosquitto -c mosquitto.conf -d
cd ..

USE_ZEND_ALLOC=1 REPORT_EXIT_STATUS=1 TEST_PHP_ARGS="-q --show-diff" make test
