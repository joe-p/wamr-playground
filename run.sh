#!/bin/bash

ITERATIONS=${1:-10000}

./build-zig.sh

echo "=== Running WAMR WASM ==="
zig-out/bin/wamr-zig -f out/wasm-apps/testapp.wasm -i $ITERATIONS

echo ""

echo "=== Running WAMR AOT ==="
zig-out/bin/wamr-zig -f out/wasm-apps/testapp.aot -i $ITERATIONS

echo ""

go run main.go -f out/wasm-apps/testapp.wasm -i $ITERATIONS
