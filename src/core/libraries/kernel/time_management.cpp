// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#include "common/assert.h"
#include "common/debug.h"
#include "common/native_clock.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/libs.h"

#ifdef _WIN64
#include <pthread_time.h>
#include <windows.h>

#include "common/ntapi.h"

#else
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#endif

namespace Libraries::Kernel {

static u64 initial_ptc;
static std::unique_ptr<Common::NativeClock> clock;

u64 PS4_SYSV_ABI sceKernelGetTscFrequency() {
    return clock->GetTscFrequency();
}

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
    // TODO: this timer should support suspends, so initial ptc needs to be updated on wake up
    return clock->GetTimeUS(initial_ptc);
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

int PS4_SYSV_ABI sceKernelUsleep(u32 microseconds) {
#ifdef _WIN64
    if (microseconds < 1000u) {
        LARGE_INTEGER interval{
            .QuadPart = -1 * (microseconds * 10u),
        };
        NtDelayExecution(FALSE, &interval);
    } else {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    }
    return 0;
#else
    timespec start;
    timespec remain;
    start.tv_sec = microseconds / 1000000;
    start.tv_nsec = (microseconds % 1000000) * 1000;
    timespec* requested = &start;
    int ret = 0;
    do {
        ret = nanosleep(requested, &remain);
        requested = &remain;
    } while (ret != 0);
    return ret;
#endif
}

int PS4_SYSV_ABI posix_usleep(u32 microseconds) {
    return sceKernelUsleep(microseconds);
}

u32 PS4_SYSV_ABI sceKernelSleep(u32 seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return 0;
}

int PS4_SYSV_ABI sceKernelClockGettime(s32 clock_id, OrbisKernelTimespec* tp) {
    if (tp == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    clockid_t pclock_id = CLOCK_REALTIME;
    switch (clock_id) {
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE:
    case ORBIS_CLOCK_REALTIME_FAST:
        pclock_id = CLOCK_REALTIME;
        break;
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
    case ORBIS_CLOCK_MONOTONIC_FAST:
        pclock_id = CLOCK_MONOTONIC;
        break;
    default:
        LOG_ERROR(Lib_Kernel, "unsupported = {} using CLOCK_REALTIME", clock_id);
        break;
    }

    timespec t{};
    int result = clock_gettime(pclock_id, &t);
    tp->tv_sec = t.tv_sec;
    tp->tv_nsec = t.tv_nsec;
    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI posix_clock_gettime(s32 clock_id, OrbisKernelTimespec* time) {
    int result = sceKernelClockGettime(clock_id, time);
    if (result < 0) {
        UNREACHABLE(); // TODO return posix error code
    }
    return result;
}

int PS4_SYSV_ABI posix_nanosleep(const OrbisKernelTimespec* rqtp, OrbisKernelTimespec* rmtp) {
    const auto* request = reinterpret_cast<const timespec*>(rqtp);
    auto* remain = reinterpret_cast<timespec*>(rmtp);
    return nanosleep(request, remain);
}

int PS4_SYSV_ABI sceKernelNanosleep(const OrbisKernelTimespec* rqtp, OrbisKernelTimespec* rmtp) {
    if (!rqtp || !rmtp) {
        return SCE_KERNEL_ERROR_EFAULT;
    }

    if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    return posix_nanosleep(rqtp, rmtp);
}

int PS4_SYSV_ABI sceKernelGettimeofday(OrbisKernelTimeval* tp) {
    if (!tp) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

#ifdef _WIN64
    FILETIME filetime;
    GetSystemTimeAsFileTime(&filetime);

    constexpr u64 UNIX_TIME_START = 0x295E9648864000;
    constexpr u64 TICKS_PER_SECOND = 1000000;

    u64 ticks = filetime.dwHighDateTime;
    ticks <<= 32;
    ticks |= filetime.dwLowDateTime;
    ticks /= 10;
    ticks -= UNIX_TIME_START;

    tp->tv_sec = ticks / TICKS_PER_SECOND;
    tp->tv_usec = ticks % TICKS_PER_SECOND;
#else
    timeval tv;
    gettimeofday(&tv, nullptr);
    tp->tv_sec = tv.tv_sec;
    tp->tv_usec = tv.tv_usec;
#endif
    return ORBIS_OK;
}

int PS4_SYSV_ABI gettimeofday(OrbisKernelTimeval* tp, OrbisKernelTimezone* tz) {
    // FreeBSD docs mention that the kernel generally does not track these values
    // and they	are usually returned as	zero.
    if (tz) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }
    return sceKernelGettimeofday(tp);
}

s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone* tz) {
#ifdef _WIN64
    ASSERT(tz);
    static int tzflag = 0;
    if (!tzflag) {
        _tzset();
        tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
#else
    struct timezone tzz;
    struct timeval tv;
    gettimeofday(&tv, &tzz);
    tz->tz_dsttime = tzz.tz_dsttime;
    tz->tz_minuteswest = tzz.tz_minuteswest;
#endif
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_clock_getres(u32 clock_id, OrbisKernelTimespec* res) {
    if (res == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    clockid_t pclock_id = CLOCK_REALTIME;
    switch (clock_id) {
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE:
    case ORBIS_CLOCK_REALTIME_FAST:
        pclock_id = CLOCK_REALTIME;
        break;
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
    case ORBIS_CLOCK_MONOTONIC_FAST:
        pclock_id = CLOCK_MONOTONIC;
        break;
    default:
        UNREACHABLE();
    }

    timespec t{};
    int result = clock_getres(pclock_id, &t);
    res->tv_sec = t.tv_sec;
    res->tv_nsec = t.tv_nsec;
    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI sceKernelConvertLocaltimeToUtc(time_t param_1, int64_t param_2, time_t* seconds,
                                                OrbisKernelTimezone* timezone, int* dst_seconds) {
    LOG_INFO(Kernel, "called");
    if (timezone) {
        sceKernelGettimezone(timezone);
        param_1 -= (timezone->tz_minuteswest + timezone->tz_dsttime) * 60;
        if (seconds)
            *seconds = param_1;
        if (dst_seconds)
            *dst_seconds = timezone->tz_dsttime * 60;
    } else {
        return SCE_KERNEL_ERROR_EINVAL;
    }
    return SCE_OK;
}

namespace Dev {
u64& GetInitialPtc() {
    return initial_ptc;
}

Common::NativeClock* GetClock() {
    return clock.get();
}

} // namespace Dev

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
    LIB_FUNCTION("QvsZxomvUHs", "libkernel", 1, "libkernel", 1, 1, sceKernelNanosleep);
    LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, sceKernelUsleep);
    LIB_FUNCTION("QcteRwbsnV0", "libkernel", 1, "libkernel", 1, 1, posix_usleep);
    LIB_FUNCTION("QcteRwbsnV0", "libScePosix", 1, "libkernel", 1, 1, posix_usleep);
    LIB_FUNCTION("-ZR+hG7aDHw", "libkernel", 1, "libkernel", 1, 1, sceKernelSleep);
    LIB_FUNCTION("0wu33hunNdE", "libScePosix", 1, "libkernel", 1, 1, sceKernelSleep);
    LIB_FUNCTION("yS8U2TGCe1A", "libkernel", 1, "libkernel", 1, 1, posix_nanosleep);
    LIB_FUNCTION("yS8U2TGCe1A", "libScePosix", 1, "libkernel", 1, 1, posix_nanosleep);
    LIB_FUNCTION("QBi7HCK03hw", "libkernel", 1, "libkernel", 1, 1, sceKernelClockGettime);
    LIB_FUNCTION("kOcnerypnQA", "libkernel", 1, "libkernel", 1, 1, sceKernelGettimezone);
    LIB_FUNCTION("lLMT9vJAck0", "libkernel", 1, "libkernel", 1, 1, posix_clock_gettime);
    LIB_FUNCTION("lLMT9vJAck0", "libScePosix", 1, "libkernel", 1, 1, posix_clock_gettime);
    LIB_FUNCTION("smIj7eqzZE8", "libScePosix", 1, "libkernel", 1, 1, posix_clock_getres);
    LIB_FUNCTION("0NTHN1NKONI", "libkernel", 1, "libkernel", 1, 1, sceKernelConvertLocaltimeToUtc);
}

} // namespace Libraries::Kernel
