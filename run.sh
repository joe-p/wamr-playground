#!/bin/bash

./build.sh

echo "*** Running WASM ***"
out/basic -f out/wasm-apps/testapp.wasm

echo ""

echo "*** Running AOT ***"
out/basic -f out/wasm-apps/testapp.aot
