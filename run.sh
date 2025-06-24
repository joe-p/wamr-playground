#!/bin/bash

ITERATIONS=${1:-10000}

./build.sh

echo "=== Running WAMR WASM ==="
out/basic -f out/wasm-apps/testapp.wasm -i $ITERATIONS

echo ""

echo "=== Running WAMR AOT ==="
out/basic -f out/wasm-apps/testapp.aot -i $ITERATIONS

echo ""

go run main.go -f out/wasm-apps/testapp.wasm -i $ITERATIONS
