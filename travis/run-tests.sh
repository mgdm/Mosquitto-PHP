#!/bin/sh

set -e

cd $TRAVIS_BUILD_DIR

REPORT_EXIT_STATUS=1 TEST_PHP_ARGS="-q --show-diff" make test
