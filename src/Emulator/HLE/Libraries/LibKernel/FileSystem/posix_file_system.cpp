#include "posix_file_system.h"

#include <debug.h>

#include "file_system.h"

namespace Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX {
int PS4_SYSV_ABI open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    int result = sceKernelOpen(path, flags, mode);
    if (result < 0) {
        BREAKPOINT();  // posix calls different only for their return values
    }
    return result;
}
}  // namespace Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX
