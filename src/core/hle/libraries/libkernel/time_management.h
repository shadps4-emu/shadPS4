#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Core::Libraries::LibKernel {

struct SceKernelTimespec {
    s64 tv_sec;
    s64 tv_nsec;
};

u64 PS4_SYSV_ABI sceKernelGetProcessTime();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency();
u64 PS4_SYSV_ABI sceKernelReadTsc();
int PS4_SYSV_ABI sceKernelClockGettime(s32 clock_id, SceKernelTimespec* tp);

void timeSymbolsRegister(Loader::SymbolsResolver* sym);

}  // namespace Core::Libraries::LibKernel
