#!/bin/bash

./build.sh

echo "=== Running WAMR WASM ==="
out/basic -f out/wasm-apps/testapp.wasm

echo ""

echo "=== Running WAMR AOT ==="
out/basic -f out/wasm-apps/testapp.aot

echo ""

go run main.go -f out/wasm-apps/testapp.wasm
