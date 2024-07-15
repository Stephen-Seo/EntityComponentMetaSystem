#!/usr/bin/env bash

cd "$(dirname "$0")"

set -ve

if ! [[ -e "${HOME}/.conan2/profiles/default" ]]; then
    conan profile detect
fi

if ! grep stephens_forgejo "${HOME}/.conan2/remotes.json" >&/dev/null; then
    conan remote add stephens_forgejo "https://git.seodisparate.com/api/packages/stephenseo/conan"
fi

conan install . -r stephens_forgejo --output-folder=buildConan
cmake -S . -B buildConan -DCMAKE_TOOLCHAIN_FILE=buildConan/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build buildConan
