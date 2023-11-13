#include "core/hle/libraries/libkernel/time_management.h"

#include <core/hle/error_codes.h>
#include <pthread_time.h>

#include "common/debug.h"
#include "common/log.h"
#include "common/timer.h"
#include "core/hle/libraries/libs.h"
#include "emuTimer.h"

namespace Core::Libraries::LibKernel {

constexpr bool log_time_file = true;  // disable it to disable logging

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
    return static_cast<u64>(Emulator::emuTimer::getTimeMsec() * 1000.0);  // return time in microseconds
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter() { return Emulator::emuTimer::getTimeCounter(); }

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency() { return Emulator::emuTimer::getTimeFrequency(); }

u64 PS4_SYSV_ABI sceKernelReadTsc() { return Common::Timer::getQueryPerformanceCounter(); }

int PS4_SYSV_ABI sceKernelClockGettime(s32 clock_id, SceKernelTimespec* tp) {
    PRINT_FUNCTION_NAME();

    if (tp == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }

    clockid_t pclock_id = CLOCK_REALTIME;
    switch (clock_id) {
        case 0: pclock_id = CLOCK_REALTIME; break;
        case 13:
        case 4: pclock_id = CLOCK_MONOTONIC; break;
        default: LOG_TRACE_IF(log_time_file, "sceKernelClockGettime unknown clock_id: {}\n", clock_id); std::exit(0);
    }

    timespec t{};

    int result = clock_gettime(pclock_id, &t);

    tp->tv_sec = t.tv_sec;
    tp->tv_nsec = t.tv_nsec;

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}

void timeSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTime);
    LIB_FUNCTION("fgxnMeTNUtY", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTimeCounter);
    LIB_FUNCTION("BNowx2l588E", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTimeCounterFrequency);
    LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", 1, 1, sceKernelReadTsc);
    LIB_FUNCTION("QBi7HCK03hw", "libkernel", 1, "libkernel", 1, 1, sceKernelClockGettime);
}

}  // namespace Core::Libraries::LibKernel
