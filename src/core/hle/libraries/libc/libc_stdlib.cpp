#include "core/hle/libraries/libc/libc_stdlib.h"

#include <cstdlib>

#include "common/debug.h"
#include "common/log.h"
#include <core/hle/libraries/libs.h>

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

void PS4_SYSV_ABI exit(int code) { std::exit(code); }

int PS4_SYSV_ABI atexit(void (*func)()) {
    PRINT_FUNCTION_NAME();
    int rt = std::atexit(func);
    if (rt != 0) {
        LOG_ERROR_IF(log_file_libc, "atexit returned {}\n", rt);
        BREAKPOINT();
    }
    return rt;
}

void* PS4_SYSV_ABI malloc(size_t size) {
    PRINT_FUNCTION_NAME();
    return std::malloc(size);
}

void PS4_SYSV_ABI free(void* ptr) {
    PRINT_FUNCTION_NAME();
    std::free(ptr);
}

typedef int(PS4_SYSV_ABI* pfunc_QsortCmp)(const void*, const void*);
thread_local static pfunc_QsortCmp compair_ps4;

int qsort_compair(const void* arg1, const void* arg2) { PRINT_FUNCTION_NAME();return compair_ps4(arg1, arg2); }

void PS4_SYSV_ABI qsort(void* ptr, size_t count, size_t size, int(PS4_SYSV_ABI* comp)(const void*, const void*)) {
    PRINT_FUNCTION_NAME();
    compair_ps4 = comp;
    std::qsort(ptr, count, size, qsort_compair);
}

int PS4_SYSV_ABI rand() {
    PRINT_FUNCTION_NAME();
    return std::rand();
}

unsigned long int PS4_SYSV_ABI _Stoul(const char* str, char** endptr, int base) {
    PRINT_FUNCTION_NAME();
    return std::strtoul(str, endptr, base);
}

void PS4_SYSV_ABI srand(unsigned int seed) {
    PRINT_FUNCTION_NAME();
    return std::srand(seed);
}

s64 PS4_SYSV_ABI time(s64* pt) {
    PRINT_FUNCTION_NAME();
    return std::time(pt);
}
}  // namespace Core::Libraries::LibC
