#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibKernel {

void LibKernel_Register(SymbolsResolver* sym);

// functions


int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);
};  // namespace HLE::Libs::LibKernel