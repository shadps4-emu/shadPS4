#include <core/PS4/HLE/ErrorCodes.h>
#include <core/PS4/HLE/Libs.h>
#include "common/log.h"
#include "system_service.h"

namespace Core::Libraries::LibSystemService {

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
    PRINT_DUMMY_FUNCTION_NAME();
    return SCE_OK;
}

void systemServiceSymbolsRegister(SymbolsResolver* sym) {
    LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1, "libSceSystemService", 1, 1, sceSystemServiceHideSplashScreen);
}
};  // namespace Emulator::HLE::Libraries::LibUserService