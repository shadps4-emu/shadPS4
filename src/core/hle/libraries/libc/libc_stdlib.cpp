#include <cstdlib>
#include "common/log.h"
#include "common/debug.h"
#include "core/hle/libraries/libc/libc_stdlib.h"

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

void PS4_SYSV_ABI ps4_exit(int code) {
    std::exit(code);
}

int PS4_SYSV_ABI ps4_atexit(void (*func)()) {
    int rt = std::atexit(func);
    if (rt != 0) {
        LOG_ERROR_IF(log_file_libc, "atexit returned {}\n", rt);
        BREAKPOINT();
    }
    return rt;
}

void* PS4_SYSV_ABI ps4_malloc(size_t size) {
    return std::malloc(size);
}

void PS4_SYSV_ABI ps4_free(void* ptr) {
    std::free(ptr);
}

typedef int(PS4_SYSV_ABI* pfunc_QsortCmp)(const void*, const void*);
thread_local static pfunc_QsortCmp compair_ps4;

int qsort_compair(const void* arg1, const void* arg2) {
    return compair_ps4(arg1, arg2);
}

void PS4_SYSV_ABI ps4_qsort(void* ptr, size_t count, size_t size, int(PS4_SYSV_ABI* comp)(const void*, const void*)) {
    compair_ps4 = comp;
    std::qsort(ptr, count, size, qsort_compair);
}

int PS4_SYSV_ABI ps4_rand() {
    return std::rand();
}

} // namespace Core::Libraries::LibC
