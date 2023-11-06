#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSystemService {

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen();

void systemServiceSymbolsRegister(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibSystemService
