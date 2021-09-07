#!/usr/bin/env bash

pushd "$(dirname "$0")" >&/dev/null

set -v

/usr/bin/g++ -I./src -Wall -Wextra -Wpedantic -O0 -g -o ECTest.cpp.o -c ./src/test/ECTest.cpp

set +v

popd >&/dev/null
