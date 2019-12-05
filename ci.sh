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

get_build_dir() {
  case "$1" in
    address) echo "asan" ;;
    thread) echo "tsan" ;;
    memory) echo "msan" ;;
    *) echo "build" ;;
  esac
}

if [ -n "$FRAK_SAN_TYPE" ]; then
  run_tests "$(get_build_dir "$FRAK_SAN_TYPE")" "-DFRAK_SAN_TYPE=$FRAK_SAN_TYPE"
else
  run_tests build
  if [ "$1" = "all" ]; then
    run_tests asan -DFRAK_SAN_TYPE=address
    run_tests tsan -DFRAK_SAN_TYPE=thread
    if [ "$(uname)" = "Linux" ]; then
      run_tests msan -DFRAK_SAN_TYPE=memory
    fi
  fi
fi
