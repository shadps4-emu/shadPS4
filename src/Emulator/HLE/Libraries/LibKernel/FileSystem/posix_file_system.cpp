#include "posix_file_system.h"

#include <debug.h>
#include <Util/log.h>
#include "file_system.h"

namespace Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX {
constexpr bool log_file_pfs = true;  // disable it to disable logging

int PS4_SYSV_ABI open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO_IF(log_file_pfs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    if (result < 0) {
        BREAKPOINT();  // posix calls different only for their return values
    }
    return result;
}
}  // namespace Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX
