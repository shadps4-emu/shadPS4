#include "common/timer.h"
#include "core/hle/libraries/libkernel/time_management.h"
#include "core/hle/libraries/libs.h"
#include "emuTimer.h"

namespace Core::Libraries::LibKernel {

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
    return static_cast<u64>(Emulator::emuTimer::getTimeMsec() * 1000.0);  // return time in microseconds
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter() {
    return Emulator::emuTimer::getTimeCounter();
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency() {
    return Emulator::emuTimer::getTimeFrequency();
}

u64 PS4_SYSV_ABI sceKernelReadTsc() {
    return Common::Timer::getQueryPerformanceCounter();
}

void timeSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTime); 
    LIB_FUNCTION("fgxnMeTNUtY", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTimeCounter);
    LIB_FUNCTION("BNowx2l588E", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTimeCounterFrequency);
    LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", 1, 1, sceKernelReadTsc);
}

}  // namespace Core::Libraries::LibKernel
