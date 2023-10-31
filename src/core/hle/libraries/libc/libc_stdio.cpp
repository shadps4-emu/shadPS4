#include "libc_stdio.h"

namespace Core::Libraries::LibC::stdio {
int PS4_SYSV_ABI printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg) { return vsnprintf_ctx(s, n, format, arg); }
}  // namespace Core::Libraries::LibC::stdio