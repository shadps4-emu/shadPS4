#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSceGnmDriver {

int32_t sceGnmSubmitDone();
void sceGnmFlushGarlic();

void LibSceGnmDriver_Register(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibSceGnmDriver
