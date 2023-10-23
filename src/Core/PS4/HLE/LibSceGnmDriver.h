#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibSceGnmDriver {

void LibSceGnmDriver_Register(SymbolsResolver* sym);
int32_t sceGnmSubmitDone();
void sceGnmFlushGarlic();
};  // namespace HLE::Libs::LibSceGnmDriver