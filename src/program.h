#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdint.h>
#include <stddef.h>

#define ERROR_SIZE 128

struct ProgramReturn {
    uint64_t return_value;
    char error_message[ERROR_SIZE];
};

// When calling from go, ensure to pin the heap_buf and wasm_binary
// ```go
// pinner.Pin(unsafe.SliceData(data))
// defer pinner.Unpin()
// ```
struct ProgramReturn run_program(uint8_t *wasm_binary, size_t binary_size, char *heap_buf, size_t heap_size, int iterations);

#endif // PROGRAM_H 
