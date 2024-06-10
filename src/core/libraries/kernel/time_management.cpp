// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/native_clock.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/libs.h"
#include "core/libraries/error_codes.h"

namespace Libraries::Kernel {

static u64 initial_ptc;
static std::unique_ptr<Common::NativeClock> clock;

u64 PS4_SYSV_ABI sceKernelGetTscFrequency() {
    return clock->GetTscFrequency();
}

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
    return clock->GetProcessTimeUS();
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter() {
    return clock->GetUptime() - initial_ptc;
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency() {
    return clock->GetTscFrequency();
}

u64 PS4_SYSV_ABI sceKernelReadTsc() {
    return clock->GetUptime();
}

int PS4_SYSV_ABI sceKernelGettimeofday(OrbisKernelTimeval *tp) {
    if (!tp) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);

    tp->tv_sec = seconds.count();
    tp->tv_usec = microsecs.count();
    return ORBIS_OK;
}

int PS4_SYSV_ABI gettimeofday(OrbisKernelTimeval *tp, OrbisKernelTimezone* tz) {
    // FreeBSD docs mention that the kernel generally does not track these values
    // and they	are usually returned as	zero.
    if (tz) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }
    return sceKernelGettimeofday(tp);
}

void timeSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    clock = std::make_unique<Common::NativeClock>();
    initial_ptc = clock->GetUptime();
    LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTime);
    LIB_FUNCTION("fgxnMeTNUtY", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcessTimeCounter);
    LIB_FUNCTION("BNowx2l588E", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelGetProcessTimeCounterFrequency);
    LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", 1, 1, sceKernelReadTsc);
    LIB_FUNCTION("1j3S3n-tTW4", "libkernel", 1, "libkernel", 1, 1, sceKernelGetTscFrequency);
    LIB_FUNCTION("ejekcaNQNq0", "libkernel", 1, "libkernel", 1, 1, sceKernelGettimeofday);
    LIB_FUNCTION("n88vx3C5nW8", "libkernel", 1, "libkernel", 1, 1, gettimeofday);
    LIB_FUNCTION("n88vx3C5nW8", "libScePosix", 1, "libkernel", 1, 1, gettimeofday);
}

} // namespace Libraries::Kernel
