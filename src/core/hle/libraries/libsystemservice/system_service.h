#pragma once
#include "core/PS4/Loader/SymbolsResolver.h"

namespace Core::Libraries::LibSystemService {

//HLE functions
s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen();

void systemServiceSymbolsRegister(SymbolsResolver* sym);

};  // namespace Emulator::HLE::Libraries::LibUserService