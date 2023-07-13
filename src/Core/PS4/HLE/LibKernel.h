#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibKernel {

void LibKernel_Register(SymbolsResolver* sym);
// functions
int sceKernelAllocateDirectMemory(off_t searchStart, off_t searchEnd, size_t len, size_t alignment, int memoryType, off_t* physAddrOut);
size_t sceKernelGetDirectMemorySize();
int sceKernelCreateEqueue(/* SceKernelEqueue* eq*/ int eq, const char* name);
int32_t sceKernelMapDirectMemory(void** addr, size_t len, int prot, int flags, off_t directMemoryStart, size_t alignment);
int32_t sceKernelReleaseDirectMemory(off_t start, size_t len);

};  // namespace HLE::Libs::LibKernel