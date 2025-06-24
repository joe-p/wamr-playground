#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// uint64_t fibonacci(uint64_t n) {
//     if (n <= 1)
//         return n;
//     return fibonacci(n - 1) + fibonacci(n - 2);
// }

__attribute__((export_name("program"))) uint64_t program() { return 7; }
