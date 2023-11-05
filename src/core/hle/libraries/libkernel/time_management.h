#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

u64 PS4_SYSV_ABI sceKernelGetProcessTime();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency();
u64 PS4_SYSV_ABI sceKernelReadTsc();

void timeSymbolsRegister(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibKernel
