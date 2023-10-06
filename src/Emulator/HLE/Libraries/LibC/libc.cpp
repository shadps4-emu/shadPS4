#include "libc.h"

namespace Emulator::HLE::Libraries::LibC {

PS4_SYSV_ABI int printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}
};  // namespace Emulator::HLE::Libraries::LibC