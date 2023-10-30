#pragma once

#include "types.h"
#include "Core/PS4/Loader/SymbolsResolver.h"

namespace Core::Libraries {
u64 sceKernelGetProcessTime();

void timeSymbolsRegister(SymbolsResolver* sym);
}