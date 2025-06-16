#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((export_name("program")))
uint64_t program() { return 1337; }
