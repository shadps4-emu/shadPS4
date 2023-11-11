#include "core/hle/libraries/libsystemservice/system_service.h"

#include "common/log.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibSystemService {

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
    PRINT_DUMMY_FUNCTION_NAME();
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceSystemServiceGetStatus(SceSystemServiceStatus* status) {
    SceSystemServiceStatus st = {};
    st.eventNum = 0;
    st.isSystemUiOverlaid = false;
    st.isInBackgroundExecution = false;
    st.isCpuMode7CpuNormal = true;
    st.isGameLiveStreamingOnAir = false;
    st.isOutOfVrPlayArea = false;
    *status = st;
    return SCE_OK;
}

void systemServiceSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1, "libSceSystemService", 1, 1, sceSystemServiceHideSplashScreen);
    LIB_FUNCTION("rPo6tV8D9bM", "libSceSystemService", 1, "libSceSystemService", 1, 1, sceSystemServiceGetStatus);
}

};  // namespace Core::Libraries::LibSystemService
