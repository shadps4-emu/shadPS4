#include "libc.h"

#include <debug.h>

#include <cstdlib>
#include <cstring>

namespace Emulator::HLE::Libraries::LibC {

PS4_SYSV_ABI int printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

PS4_SYSV_ABI void exit(int code) { std::exit(code); }

PS4_SYSV_ABI int atexit(void (*func)()) {
    int rt = std::atexit(func);
    if (rt != 0) {
        BREAKPOINT();
    }
    return rt;
}

int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n) { return std::memcmp(s1, s2, n); }

void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n) { return std::memcpy(dest, src, n); }

void* PS4_SYSV_ABI memset(void* s, int c, size_t n) { return std::memset(s, c, n); }

};  // namespace Emulator::HLE::Libraries::LibC