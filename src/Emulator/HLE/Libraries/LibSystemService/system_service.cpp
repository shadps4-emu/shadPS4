#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>

#include "system_service.h"

namespace Emulator::HLE::Libraries::LibSystemService {

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
    // dummy
    return SCE_OK;
}

void libSystemService_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1, "libSceSystemService", 1, 1, sceSystemServiceHideSplashScreen);
}
};  // namespace Emulator::HLE::Libraries::LibUserService