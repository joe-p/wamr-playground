#!/bin/bash

ITERATIONS=${1:-10000}

./build-zig.sh

echo "=== Running with WAMR (Interpreter) ==="
zig-out/bin/wamr-zig -f out/wasm-apps/testapp.wasm -i $ITERATIONS

echo ""

echo "=== Running with WAMR (AOT) ==="
zig-out/bin/wamr-zig -f out/wasm-apps/testapp.aot -i $ITERATIONS

go run main.go -f out/wasm-apps/testapp.wasm -i $ITERATIONS
