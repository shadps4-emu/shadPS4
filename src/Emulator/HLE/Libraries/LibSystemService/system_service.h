#pragma once
#include "Core/PS4/Loader/SymbolsResolver.h"

namespace Emulator::HLE::Libraries::LibSystemService {

//HLE functions
s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen();

void libSystemService_Register(SymbolsResolver* sym);

};  // namespace Emulator::HLE::Libraries::LibUserService