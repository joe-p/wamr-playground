/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_getopt.h"
#include "platform_common.h"
#include "program.h"
#include "wasm_export.h"
#include <stdio.h>
#include <stdlib.h>

// Function to read a file into a buffer and return the size
uint8_t *read_file_as_bytes(const char *path, uint32_t *size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", path);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        printf("Failed to get file size\n");
        fclose(file);
        return NULL;
    }

    // Allocate buffer
    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (!buffer) {
        printf("Failed to allocate memory for file\n");
        fclose(file);
        return NULL;
    }

    // Read file into buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);

    if (read_size != file_size) {
        printf("Failed to read entire file\n");
        free(buffer);
        return NULL;
    }

    *size = (uint32_t)file_size;
    return buffer;
}

void my_log(uint32 log_level, const char *file, int line, const char *fmt, ...) {
    char buf[200];
    snprintf(buf, 200, log_level == WASM_LOG_LEVEL_VERBOSE ? "[WamrLogger - VERBOSE] %s" : "[WamrLogger] %s", fmt);

    va_list ap;
    va_start(ap, fmt);
    vprintf(buf, ap);
    va_end(ap);
}

int my_vprintf(const char *format, va_list ap) {
    /* Print in blue */
    char buf[200];
    snprintf(buf, 200, "\x1b[34m%s\x1b[0m", format);
    return vprintf(buf, ap);
}

void print_usage(void) {
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -f [path of wasm file] \n");
}

int main(int argc, char *argv_main[]) {
    char *buffer;
    int opt;
    char *wasm_path = NULL;

    uint32 buf_size;

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    while ((opt = getopt(argc, argv_main, "hf:")) != -1) {
        switch (opt) {
        case 'f':
            wasm_path = optarg;
            break;
        case 'h':
            print_usage();
            return 0;
        case '?':
            print_usage();
            return 0;
        }
    }
    if (optind == 1) {
        print_usage();
        return 0;
    }

    buffer = read_file_as_bytes(wasm_path, &buf_size);
    static char new_heap[512 * 1024];

    struct ProgramReturn program_result = {0, ""};

    program_result = run_program((uint8 *)buffer, buf_size, new_heap, sizeof(new_heap));

    printf("\n\nProgram return value: %lld\n", program_result.return_value);
    printf("Program error message: %s\n", program_result.error_message);

    return 0;
}
