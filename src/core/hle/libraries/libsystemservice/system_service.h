#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSystemService {

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

void systemServiceSymbolsRegister(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibSystemService
