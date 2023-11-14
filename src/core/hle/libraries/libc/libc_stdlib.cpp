#include "core/hle/libraries/libc/libc_stdlib.h"

#include <cstdlib>

#include "common/debug.h"
#include "common/log.h"

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

void PS4_SYSV_ABI exit(int code) { std::exit(code); }

int PS4_SYSV_ABI atexit(void (*func)()) {
    int rt = std::atexit(func);
    if (rt != 0) {
        LOG_ERROR_IF(log_file_libc, "atexit returned {}\n", rt);
        BREAKPOINT();
    }
    return rt;
}

void* PS4_SYSV_ABI malloc(size_t size) { return std::malloc(size); }

void PS4_SYSV_ABI free(void* ptr) { std::free(ptr); }

typedef int(PS4_SYSV_ABI* pfunc_QsortCmp)(const void*, const void*);
thread_local static pfunc_QsortCmp compair_ps4;

int qsort_compair(const void* arg1, const void* arg2) { return compair_ps4(arg1, arg2); }

void PS4_SYSV_ABI qsort(void* ptr, size_t count, size_t size, int(PS4_SYSV_ABI* comp)(const void*, const void*)) {
    compair_ps4 = comp;
    std::qsort(ptr, count, size, qsort_compair);
}

int PS4_SYSV_ABI rand() { return std::rand(); }

unsigned long int PS4_SYSV_ABI _Stoul(const char* str, char** endptr, int base) { return std::strtoul(str, endptr, base); }

void PS4_SYSV_ABI srand(unsigned int seed) { return std::srand(seed); }

s64 PS4_SYSV_ABI time(s64* pt) { return std::time(pt); }
}  // namespace Core::Libraries::LibC
