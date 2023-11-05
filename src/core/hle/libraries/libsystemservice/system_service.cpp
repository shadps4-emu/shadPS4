#include "common/log.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libsystemservice/system_service.h"

namespace Core::Libraries::LibSystemService {

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
    PRINT_DUMMY_FUNCTION_NAME();
    return SCE_OK;
}

void systemServiceSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1, "libSceSystemService", 1, 1,
                 sceSystemServiceHideSplashScreen);
}

}; // namespace Core::Libraries::LibSystemService
