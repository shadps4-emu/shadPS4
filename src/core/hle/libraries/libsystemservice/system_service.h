#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSystemService {

using SceSystemServiceParamId = s32;
struct SceSystemServiceStatus {
    s32 eventNum;
    bool isSystemUiOverlaid;
    bool isInBackgroundExecution;
    bool isCpuMode7CpuNormal;
    bool isGameLiveStreamingOnAir;
    bool isOutOfVrPlayArea;
    u08 reserved[];
};

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen();
s32 PS4_SYSV_ABI sceSystemServiceGetStatus(SceSystemServiceStatus* status);
s32 PS4_SYSV_ABI sceSystemServiceParamGetInt(SceSystemServiceParamId paramId, s32* value);

void systemServiceSymbolsRegister(Loader::SymbolsResolver* sym);

};  // namespace Core::Libraries::LibSystemService
