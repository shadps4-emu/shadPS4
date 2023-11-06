#pragma once

#include <sys/types.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);

void LibKernel_Register(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibKernel
