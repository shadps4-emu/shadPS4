#include "time_management.h"

#include <Core/PS4/HLE/Libs.h>

#include "emuTimer.h"

namespace Core::Libraries::LibKernel {
u64 sceKernelGetProcessTime() {
    return static_cast<u64>(Emulator::emuTimer::getTimeMsec() * 1000.0);  // return time in microseconds
}

void timeSymbolsRegister(SymbolsResolver* sym) { LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTime); }

}  // namespace Core::Libraries
