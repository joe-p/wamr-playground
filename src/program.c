#include "program.h"
#include "platform_common.h"
#include "wasm_export.h"
#include <stddef.h>
#include <string.h>

struct ProgramReturn run_program(uint8 *wasm_binary, size_t binary_size, char *heap_buf, size_t heap_size) {

    struct ProgramReturn result = {0, ""};
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    wasm_module_t module = NULL;
    RuntimeInitArgs init_args;
    wasm_function_inst_t program_func = NULL;

    struct timespec start, end;
    long long elapsed_ns;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = heap_buf;
    init_args.mem_alloc_option.pool.heap_size = heap_size;

    init_args.running_mode = Mode_Interp;
    init_args.native_module_name = "avm";

    if (!wasm_runtime_full_init(&init_args)) {
        strcpy(result.error_message, "Init runtime environment failed.");
        goto fail;
    }

    uint32 stack_size = 8092;
    char error_buf[ERROR_SIZE];

    clock_gettime(CLOCK_REALTIME, &start);

    module = wasm_runtime_load(wasm_binary, binary_size, error_buf, sizeof(error_buf));

    if (!module) {
        strcpy(result.error_message, error_buf);
        goto fail;
    }

    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));

    if (!module_inst) {
        strcpy(result.error_message, error_buf);
        goto fail;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    if (!exec_env) {
        strcpy(result.error_message, "Create wasm execution environment failed.");
        goto fail;
    }

    if (!(program_func = wasm_runtime_lookup_function(module_inst, "program"))) {
        strcpy(result.error_message, "The program wasm function is not found.");
        goto fail;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Load to lookup time: %lld nanoseconds (%.3f ms)\n", elapsed_ns, elapsed_ns / 1000000.0);

    // Measure call time
    clock_gettime(CLOCK_REALTIME, &start);

    wasm_val_t results[1] = {{.kind = WASM_I64, .of.i64 = 0}};

    if (!wasm_runtime_call_wasm_a(exec_env, program_func, 1, results, 0, NULL)) {
        strcpy(result.error_message, wasm_runtime_get_exception(module_inst));
        goto fail;
    }

    result.return_value = results[0].of.i64;

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Call time: %lld nanoseconds (%.3f ms)\n", elapsed_ns, elapsed_ns / 1000000.0);

fail:
    if (exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst) {
        wasm_runtime_deinstantiate(module_inst);
    }
    if (module)
        wasm_runtime_unload(module);
    wasm_runtime_destroy();
    return result;
}
