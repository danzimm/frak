#!/usr/bin/env bash

set -ex

run_tests() {
  build_dir="$1"
  shift
  rm -rf "$build_dir" && mkdir "$build_dir" && pushd "$build_dir" && \
    cmake ../ -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release $@ && \
    make && make t && \
    popd
}

run_tests build
run_tests asan -DFRAK_SAN_TYPE=address
run_tests tsan -DFRAK_SAN_TYPE=thread
run_tests msan -DFRAK_SAN_TYPE=memory
