#pragma once

#include "types.h"
#include "core/PS4/Loader/SymbolsResolver.h"

namespace Core::Libraries::LibKernel {
u64 PS4_SYSV_ABI sceKernelGetProcessTime();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency();
u64 PS4_SYSV_ABI sceKernelReadTsc();

void timeSymbolsRegister(SymbolsResolver* sym);
}