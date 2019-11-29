#!/usr/bin/env bash

set -ex
mkdir -p build && pushd build && rm -r * && \
  cmake ../ -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release && make && make test && \
  popd

mkdir -p asan && pushd asan && rm -r * && \
  cmake ../ -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release -DFRAK_SAN_TYPE=address && make && make test && \
  popd
