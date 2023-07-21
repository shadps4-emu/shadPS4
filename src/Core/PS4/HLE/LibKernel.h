#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibKernel {

void LibKernel_Register(SymbolsResolver* sym);

// functions
int PS4_SYSV_ABI sceKernelAllocateDirectMemory(off_t searchStart, off_t searchEnd, size_t len, size_t alignment, int memoryType, off_t* physAddrOut);
size_t PS4_SYSV_ABI sceKernelGetDirectMemorySize();
int PS4_SYSV_ABI sceKernelCreateEqueue(/* SceKernelEqueue* eq*/ int eq, const char* name);
int32_t PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, size_t len, int prot, int flags, off_t directMemoryStart, size_t alignment);
int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);
int PS4_SYSV_ABI sceKernelIsNeoMode();
int PS4_SYSV_ABI sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/);
};  // namespace HLE::Libs::LibKernel