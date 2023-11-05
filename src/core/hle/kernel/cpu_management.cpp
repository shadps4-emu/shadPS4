#include "common/log.h"
#include "core/hle/kernel/cpu_management.h"
#include "core/hle/libraries/libs.h"
#include "Util/config.h"

namespace Core::Kernel {

int PS4_SYSV_ABI sceKernelIsNeoMode() {
    PRINT_FUNCTION_NAME();
    return Config::isNeoMode();
}

} // namespace Core::Kernel
