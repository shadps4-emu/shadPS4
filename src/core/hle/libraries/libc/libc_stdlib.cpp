#include "libc_stdlib.h"

#include <Util/log.h>
#include <debug.h>

#include <cstdlib>

namespace Core::Libraries::LibC::stdlib {
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

}  // namespace Core::Libraries::LibC::stdlib
