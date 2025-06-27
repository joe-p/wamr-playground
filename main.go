package main

import (
	"context"
	"flag"
	"fmt"
	"io"
	"os"
	"time"

	"github.com/tetratelabs/wazero"
)

type ProgramReturn struct {
	ReturnValue  int64
	ErrorMessage string
}

func readFile(path string) ([]byte, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("failed to open file: %w", err)
	}
	defer file.Close()

	return io.ReadAll(file)
}

func runProgramWithRuntime(ctx context.Context, wasmBinary []byte, runtimeConfig wazero.RuntimeConfig, runtimeName string, iterations int) ProgramReturn {
	result := ProgramReturn{ReturnValue: 0, ErrorMessage: ""}

	runtime := wazero.NewRuntimeWithConfig(ctx, runtimeConfig)
	defer runtime.Close(ctx)

	start := time.Now()

	compiledModule, err := runtime.CompileModule(ctx, wasmBinary)
	if err != nil {
		result.ErrorMessage = fmt.Sprintf("Failed to compile module: %v", err)
		return result
	}
	defer compiledModule.Close(ctx)

	module, err := runtime.InstantiateModule(ctx, compiledModule, wazero.NewModuleConfig())
	if err != nil {
		result.ErrorMessage = fmt.Sprintf("Failed to instantiate module: %v", err)
		return result
	}
	defer module.Close(ctx)

	programFunc := module.ExportedFunction("program")
	if programFunc == nil {
		result.ErrorMessage = "The program wasm function is not found"
		return result
	}

	loadTime := time.Since(start)
	fmt.Printf("[%s] Load to lookup time: %v\n", runtimeName, loadTime)

	start = time.Now()

	for range iterations {
		results, err := programFunc.Call(ctx)
		if err != nil {
			result.ErrorMessage = fmt.Sprintf("Failed to call program function: %v", err)
			return result
		}

		if len(results) > 0 {
			result.ReturnValue = int64(results[0])
		}
	}

	elapsed := time.Since(start)
	timePerOp := elapsed / time.Duration(iterations)
	fmt.Printf("[%s] Call time: %v/iter (%v total for %d iters)\n", runtimeName, timePerOp, elapsed, iterations)

	return result
}

func main() {
	var wasmPath string
	var iterations int
	flag.StringVar(&wasmPath, "f", "", "path of wasm file")
	flag.IntVar(&iterations, "i", 10000, "number of iterations")
	flag.Parse()

	if wasmPath == "" {
		fmt.Println("Options:")
		fmt.Println("  -f [path of wasm file]")
		fmt.Println("  -i [number of iterations] (default: 10000)")
		return
	}

	wasmBinary, err := readFile(wasmPath)
	if err != nil {
		fmt.Printf("Error reading file: %v\n", err)
		return
	}

	ctx := context.Background()

	fmt.Println("\n=== Running with Interpreter Runtime ===")
	interpreterConfig := wazero.NewRuntimeConfigInterpreter().WithMemoryCapacityFromMax(true).WithMemoryLimitPages(62)
	interpreterResult := runProgramWithRuntime(ctx, wasmBinary, interpreterConfig, "Default", iterations)

	fmt.Printf("Program return value: %d\n", interpreterResult.ReturnValue)
	if interpreterResult.ErrorMessage != "" {
		fmt.Printf("Program error message: %s\n", interpreterResult.ErrorMessage)
	}

	fmt.Println("\n=== Running with Compiler (AOT) Runtime ===")
	// cache, err := wazero.NewCompilationCacheWithDir("wazero-cache")
	// if err != nil {
	// 	panic(fmt.Sprintf("Error creating compilation cache: %v\n", err))
	// }
	cache := wazero.NewCompilationCache()
	compilerConfig := wazero.NewRuntimeConfigCompiler().WithMemoryCapacityFromMax(true).WithMemoryLimitPages(62).WithCompilationCache(cache)
	compilerResult := runProgramWithRuntime(ctx, wasmBinary, compilerConfig, "Cached", iterations)

	fmt.Printf("Program return value: %d\n", compilerResult.ReturnValue)
	if compilerResult.ErrorMessage != "" {
		fmt.Printf("Program error message: %s\n", compilerResult.ErrorMessage)
	}

	fmt.Println("\n=== Performance Comparison ===")
	if interpreterResult.ErrorMessage == "" && compilerResult.ErrorMessage == "" {
		fmt.Println("Both runtimes completed successfully")
		fmt.Printf("Return values match: %t\n", interpreterResult.ReturnValue == compilerResult.ReturnValue)
	}
}
