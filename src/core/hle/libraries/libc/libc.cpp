#include "libc.h"

#include <debug.h>

#include <cstdlib>


namespace Core::Libraries::LibC {

void PS4_SYSV_ABI exit(int code) { std::exit(code); }

int PS4_SYSV_ABI atexit(void (*func)()) {
    int rt = std::atexit(func);
    if (rt != 0) {
        BREAKPOINT();
    }
    return rt;
}



void* PS4_SYSV_ABI malloc(size_t size) { return std::malloc(size); }

void PS4_SYSV_ABI free(void* ptr) { std::free(ptr); }







// math

};  // namespace Core::Libraries::LibC