#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibKernel {

void LibKernel_Register(SymbolsResolver* sym);

// functions


int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);

int PS4_SYSV_ABI sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/);
};  // namespace HLE::Libs::LibKernel