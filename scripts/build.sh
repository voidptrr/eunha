#!/usr/bin/env bash

# MIT License
#
# Copyright (c) 2026 Tommaso Bruno
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set -eu

usage() {
    echo "usage: build [debug|release|tests]" >&2
    exit 1
}

(( $# <= 1 )) || usage

common_flags=(
    -std=c23
    -Isrc
    -Wall
    -Wextra
    -Wpedantic
    -Walloca
    -Warray-bounds
    -Wcast-align
    -Wcast-qual
    -Wcomma
    -Wconversion
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wmissing-prototypes
    -Wnull-dereference
    -Wpointer-arith
    -Wshadow
    -Wsign-conversion
    -Wstrict-prototypes
    -Wswitch-enum
    -Wundef
    -Wunreachable-code
    -Wvla
    -Wwrite-strings
)

debug_flags=(
    -g
    -Og
    -Werror
    '-fsanitize=address,undefined'
    -fno-omit-frame-pointer
)

release_flags=(
    -O2
    -UNDEBUG
    -D_FORTIFY_SOURCE=3
    -fstack-protector-strong
    -fPIE
    -pie
    '-Wl,-z,relro'
    '-Wl,-z,now'
)

shopt -s globstar nullglob
app_sources=(src/**/*.c)
test_sources=(tests/**/*_test.c)
module_sources=()

for source in "${app_sources[@]}"; do
    [[ "$source" == "src/main.c" ]] || module_sources+=("$source")
done

build_app() {
    local mode=$1
    shift

    mkdir -p "build/$mode"
    clang "${common_flags[@]}" "$@" "${app_sources[@]}" -o "build/$mode/eunha"
}

build_tests() {
    local source target

    for source in "${test_sources[@]}"; do
        target="build/${source%.c}"
        mkdir -p "${target%/*}"
        clang "${common_flags[@]}" "${debug_flags[@]}" \
            "${module_sources[@]}" "$source" -o "$target"
    done
}

case "${1:-debug}" in
    debug)
        build_app debug "${debug_flags[@]}"
        ;;
    release)
        build_app release "${release_flags[@]}"
        ;;
    tests)
        build_tests
        ;;
    *)
        usage
        ;;
esac
