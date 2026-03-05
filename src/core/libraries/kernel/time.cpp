// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ctime>
#include <thread>

#include "common/assert.h"
#include "common/native_clock.h"
#include "common/thread.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"

#ifdef _WIN64
#include <windows.h>
#include "common/ntapi.h"
#else
#ifdef __APPLE__
#include <date/tz.h>
#endif
#include <ctime>
#include <sys/resource.h>
#include <sys/time.h>
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

static s32 posix_nanosleep_impl(const OrbisKernelTimespec* rqtp, OrbisKernelTimespec* rmtp,
                                const bool interruptible) {
    if (!rqtp || rqtp->tv_sec < 0 || rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1'000'000'000) {
        SetPosixErrno(EINVAL);
        return -1;
    }
    const auto duration = std::chrono::nanoseconds(rqtp->tv_sec * 1'000'000'000 + rqtp->tv_nsec);
    std::chrono::nanoseconds remain;
    const auto uninterrupted = Common::AccurateSleep(duration, &remain, interruptible);
    if (rmtp) {
        rmtp->tv_sec = remain.count() / 1'000'000'000;
        rmtp->tv_nsec = remain.count() % 1'000'000'000;
    }
    if (!uninterrupted) {
        SetPosixErrno(EINTR);
        return -1;
    }
    return 0;
}

s32 PS4_SYSV_ABI posix_nanosleep(const OrbisKernelTimespec* rqtp, OrbisKernelTimespec* rmtp) {
    return posix_nanosleep_impl(rqtp, rmtp, true);
}

s32 PS4_SYSV_ABI sceKernelNanosleep(const OrbisKernelTimespec* rqtp, OrbisKernelTimespec* rmtp) {
    if (const auto ret = posix_nanosleep_impl(rqtp, rmtp, false); ret < 0) {
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_usleep(u32 microseconds) {
    const OrbisKernelTimespec ts = {
        .tv_sec = microseconds / 1'000'000,
        .tv_nsec = (microseconds % 1'000'000) * 1'000,
    };
    return posix_nanosleep(&ts, nullptr);
}

s32 PS4_SYSV_ABI sceKernelUsleep(u32 microseconds) {
    const OrbisKernelTimespec ts = {
        .tv_sec = microseconds / 1'000'000,
        .tv_nsec = (microseconds % 1'000'000) * 1'000,
    };
    return sceKernelNanosleep(&ts, nullptr);
}

u32 PS4_SYSV_ABI posix_sleep(u32 seconds) {
    const OrbisKernelTimespec ts = {
        .tv_sec = seconds,
        .tv_nsec = 0,
    };
    OrbisKernelTimespec rm;
    if (const auto ret = posix_nanosleep(&ts, &rm); ret < 0) {
        return *__Error() == POSIX_EINTR ? rm.tv_sec + (rm.tv_nsec == 0 ? 0 : 1) : seconds;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceKernelSleep(u32 seconds) {
    return sceKernelUsleep(seconds * 1'000'000);
}

s32 PS4_SYSV_ABI posix_clock_gettime(u32 clock_id, OrbisKernelTimespec* ts) {
    if (ts == nullptr) {
        SetPosixErrno(EFAULT);
        return -1;
    }

    if (clock_id == ORBIS_CLOCK_PROCTIME) {
        const auto us = sceKernelGetProcessTime();
        ts->tv_sec = static_cast<s64>(us / 1'000'000);
        ts->tv_nsec = static_cast<s64>((us % 1'000'000) * 1000);
        return 0;
    }
    if (clock_id == ORBIS_CLOCK_EXT_NETWORK || clock_id == ORBIS_CLOCK_EXT_DEBUG_NETWORK ||
        clock_id == ORBIS_CLOCK_EXT_AD_NETWORK || clock_id == ORBIS_CLOCK_EXT_RAW_NETWORK) {
        LOG_ERROR(Lib_Kernel, "Unsupported clock type {}, using CLOCK_MONOTONIC", clock_id);
        clock_id = ORBIS_CLOCK_MONOTONIC;
    }

#ifdef _WIN32
    static const auto FileTimeTo100Ns = [](FILETIME& ft) { return *reinterpret_cast<u64*>(&ft); };
    switch (clock_id) {
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE: {
        FILETIME ft;
        GetSystemTimePreciseAsFileTime(&ft);
        static constexpr u64 DeltaEpochIn100ns = 116444736000000000ULL;
        const u64 ns = FileTimeTo100Ns(ft) - DeltaEpochIn100ns;
        ts->tv_sec = ns / 10'000'000;
        ts->tv_nsec = (ns % 10'000'000) * 100;
        return 0;
    }
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_REALTIME_FAST: {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        static constexpr u64 DeltaEpochIn100ns = 116444736000000000ULL;
        const u64 ns = FileTimeTo100Ns(ft) - DeltaEpochIn100ns;
        ts->tv_sec = ns / 10'000'000;
        ts->tv_nsec = (ns % 10'000'000) * 100;
        return 0;
    }
    case ORBIS_CLOCK_UPTIME:
    case ORBIS_CLOCK_UPTIME_PRECISE:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
    case ORBIS_CLOCK_UPTIME_FAST:
    case ORBIS_CLOCK_MONOTONIC_FAST: {
        static LARGE_INTEGER pf = [] {
            LARGE_INTEGER res{};
            QueryPerformanceFrequency(&res);
            return res;
        }();

        LARGE_INTEGER pc{};
        if (!QueryPerformanceCounter(&pc)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        ts->tv_sec = pc.QuadPart / pf.QuadPart;
        ts->tv_nsec = ((pc.QuadPart % pf.QuadPart) * 1000'000'000) / pf.QuadPart;
        return 0;
    }
    case ORBIS_CLOCK_THREAD_CPUTIME_ID: {
        FILETIME ct, et, kt, ut;
        if (!GetThreadTimes(GetCurrentThread(), &ct, &et, &kt, &ut)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        const u64 ns = FileTimeTo100Ns(ut) + FileTimeTo100Ns(kt);
        ts->tv_sec = ns / 10'000'000;
        ts->tv_nsec = (ns % 10'000'000) * 100;
        return 0;
    }
    case ORBIS_CLOCK_VIRTUAL: {
        FILETIME ct, et, kt, ut;
        if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        const u64 ns = FileTimeTo100Ns(ut);
        ts->tv_sec = ns / 10'000'000;
        ts->tv_nsec = (ns % 10'000'000) * 100;
        return 0;
    }
    case ORBIS_CLOCK_PROF: {
        FILETIME ct, et, kt, ut;
        if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        const u64 ns = FileTimeTo100Ns(kt);
        ts->tv_sec = ns / 10'000'000;
        ts->tv_nsec = (ns % 10'000'000) * 100;
        return 0;
    }
    default:
        SetPosixErrno(EFAULT);
        return -1;
    }
#else
    clockid_t pclock_id;
    switch (clock_id) {
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE:
        pclock_id = CLOCK_REALTIME;
        break;
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_REALTIME_FAST:
#ifdef CLOCK_REALTIME_COARSE
        pclock_id = CLOCK_REALTIME_COARSE;
#else
        pclock_id = CLOCK_REALTIME;
#endif
        break;
    case ORBIS_CLOCK_UPTIME:
    case ORBIS_CLOCK_UPTIME_PRECISE:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
        pclock_id = CLOCK_MONOTONIC;
        break;
    case ORBIS_CLOCK_UPTIME_FAST:
    case ORBIS_CLOCK_MONOTONIC_FAST:
#ifdef CLOCK_MONOTONIC_COARSE
        pclock_id = CLOCK_MONOTONIC_COARSE;
#else
        pclock_id = CLOCK_MONOTONIC;
#endif
        break;
    case ORBIS_CLOCK_THREAD_CPUTIME_ID:
        pclock_id = CLOCK_THREAD_CPUTIME_ID;
        break;
    case ORBIS_CLOCK_VIRTUAL: {
        rusage ru;
        const auto res = getrusage(RUSAGE_SELF, &ru);
        if (res < 0) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        ts->tv_sec = ru.ru_utime.tv_sec;
        ts->tv_nsec = ru.ru_utime.tv_usec * 1000;
        return 0;
    }
    case ORBIS_CLOCK_PROF: {
        rusage ru;
        const auto res = getrusage(RUSAGE_SELF, &ru);
        if (res < 0) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        ts->tv_sec = ru.ru_stime.tv_sec;
        ts->tv_nsec = ru.ru_stime.tv_usec * 1000;
        return 0;
    }
    default:
        SetPosixErrno(EFAULT);
        return -1;
    }

    timespec t{};
    const auto result = clock_gettime(pclock_id, &t);
    ts->tv_sec = t.tv_sec;
    ts->tv_nsec = t.tv_nsec;
    if (result < 0) {
        SetPosixErrno(errno);
        return -1;
    }
    return 0;
#endif
}

s32 PS4_SYSV_ABI sceKernelClockGettime(const u32 clock_id, OrbisKernelTimespec* ts) {
    if (const auto ret = posix_clock_gettime(clock_id, ts); ret < 0) {
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_clock_getres(u32 clock_id, OrbisKernelTimespec* res) {
    if (res == nullptr) {
        SetPosixErrno(EFAULT);
        return -1;
    }

    if (clock_id == ORBIS_CLOCK_EXT_NETWORK || clock_id == ORBIS_CLOCK_EXT_DEBUG_NETWORK ||
        clock_id == ORBIS_CLOCK_EXT_AD_NETWORK || clock_id == ORBIS_CLOCK_EXT_RAW_NETWORK) {
        LOG_ERROR(Lib_Kernel, "Unsupported clock type {}, using CLOCK_MONOTONIC", clock_id);
        clock_id = ORBIS_CLOCK_MONOTONIC;
    }

#ifdef _WIN32
    switch (clock_id) {
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_REALTIME_FAST: {
        DWORD timeAdjustment;
        DWORD timeIncrement;
        BOOL isTimeAdjustmentDisabled;
        if (!GetSystemTimeAdjustment(&timeAdjustment, &timeIncrement, &isTimeAdjustmentDisabled)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        res->tv_sec = 0;
        res->tv_nsec = timeIncrement * 100;
        return 0;
    }
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE:
    case ORBIS_CLOCK_UPTIME:
    case ORBIS_CLOCK_UPTIME_PRECISE:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
    case ORBIS_CLOCK_UPTIME_FAST:
    case ORBIS_CLOCK_MONOTONIC_FAST: {
        LARGE_INTEGER pf;
        if (!QueryPerformanceFrequency(&pf)) {
            SetPosixErrno(EFAULT);
            return -1;
        }
        res->tv_sec = 0;
        res->tv_nsec =
            std::max(static_cast<s32>((1000000000 + (pf.QuadPart >> 1)) / pf.QuadPart), 1);
        return 0;
    }
    default:
        UNREACHABLE();
    }
#else
    clockid_t pclock_id;
    switch (clock_id) {
    case ORBIS_CLOCK_REALTIME:
    case ORBIS_CLOCK_REALTIME_PRECISE:
        pclock_id = CLOCK_REALTIME;
        break;
    case ORBIS_CLOCK_SECOND:
    case ORBIS_CLOCK_REALTIME_FAST:
#ifdef CLOCK_REALTIME_COARSE
        pclock_id = CLOCK_REALTIME_COARSE;
#else
        pclock_id = CLOCK_REALTIME;
#endif
        break;
    case ORBIS_CLOCK_UPTIME:
    case ORBIS_CLOCK_UPTIME_PRECISE:
    case ORBIS_CLOCK_MONOTONIC:
    case ORBIS_CLOCK_MONOTONIC_PRECISE:
        pclock_id = CLOCK_MONOTONIC;
        break;
    case ORBIS_CLOCK_UPTIME_FAST:
    case ORBIS_CLOCK_MONOTONIC_FAST:
#ifdef CLOCK_MONOTONIC_COARSE
        pclock_id = CLOCK_MONOTONIC_COARSE;
#else
        pclock_id = CLOCK_MONOTONIC;
#endif
        break;
    default:
        UNREACHABLE();
    }

    timespec t{};
    const auto result = clock_getres(pclock_id, &t);
    res->tv_sec = t.tv_sec;
    res->tv_nsec = t.tv_nsec;
    if (result < 0) {
        SetPosixErrno(errno);
        return -1;
    }
    return 0;
#endif
}

s32 PS4_SYSV_ABI sceKernelClockGetres(const u32 clock_id, OrbisKernelTimespec* res) {
    if (const auto ret = posix_clock_getres(clock_id, res); ret < 0) {
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_gettimeofday(OrbisKernelTimeval* tp, OrbisKernelTimezone* tz) {
#ifdef _WIN64
    if (tp) {
        FILETIME filetime;
        GetSystemTimePreciseAsFileTime(&filetime);

        constexpr u64 UNIX_TIME_START = 0x295E9648864000;
        constexpr u64 TICKS_PER_SECOND = 1000000;

        u64 ticks = filetime.dwHighDateTime;
        ticks <<= 32;
        ticks |= filetime.dwLowDateTime;
        ticks /= 10;
        ticks -= UNIX_TIME_START;

        tp->tv_sec = ticks / TICKS_PER_SECOND;
        tp->tv_usec = ticks % TICKS_PER_SECOND;
    }
    if (tz) {
        static int tzflag = 0;
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
    return 0;
#else
    struct timezone tzz;
    timeval tv;
    const auto ret = gettimeofday(&tv, &tzz);
    if (tp) {
        tp->tv_sec = tv.tv_sec;
        tp->tv_usec = tv.tv_usec;
    }
    if (tz) {
        tz->tz_dsttime = tzz.tz_dsttime;
        tz->tz_minuteswest = tzz.tz_minuteswest;
    }
    if (ret < 0) {
        SetPosixErrno(errno);
        return -1;
    }
    return 0;
#endif
}

s32 PS4_SYSV_ABI sceKernelGettimeofday(OrbisKernelTimeval* tp) {
    if (const auto ret = posix_gettimeofday(tp, nullptr); ret < 0) {
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone* tz) {
    if (const auto ret = posix_gettimeofday(nullptr, tz); ret < 0) {
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelConvertLocaltimeToUtc(time_t param_1, int64_t param_2, time_t* seconds,
                                                OrbisKernelTimezone* timezone, s32* dst_seconds) {
    LOG_INFO(Kernel, "called");
    if (timezone) {
        sceKernelGettimezone(timezone);
        param_1 -= (timezone->tz_minuteswest + timezone->tz_dsttime) * 60;
        if (seconds) {
            *seconds = param_1;
        }
        if (dst_seconds) {
            *dst_seconds = timezone->tz_dsttime * 60;
        }
    } else {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    return ORBIS_OK;
}

namespace Dev {
u64& GetInitialPtc() {
    return initial_ptc;
}

Common::NativeClock* GetClock() {
    return clock.get();
}

} // namespace Dev

s32 PS4_SYSV_ABI sceKernelConvertUtcToLocaltime(time_t time, time_t* local_time,
                                                struct OrbisTimesec* st, u64* dst_sec) {
    LOG_TRACE(Kernel, "Called");
#ifdef _WIN32
    TIME_ZONE_INFORMATION tz{};
    DWORD res = GetTimeZoneInformation(&tz);
    *local_time = time - tz.Bias;

    if (st != nullptr) {
        st->t = time;
        st->west_sec = -tz.Bias * 60;
        st->dst_sec = res == TIME_ZONE_ID_DAYLIGHT ? -_dstbias : 0;
    }

    if (dst_sec != nullptr) {
        *dst_sec = res == TIME_ZONE_ID_DAYLIGHT ? -_dstbias : 0;
    }
#else
#ifdef __APPLE__
    // std::chrono::current_zone() not available yet.
    const auto* time_zone = date::current_zone();
#else
    const auto* time_zone = std::chrono::current_zone();
#endif // __APPLE__
    auto info = time_zone->get_info(std::chrono::system_clock::now());

    *local_time = info.offset.count() + info.save.count() * 60 + time;

    if (st != nullptr) {
        st->t = time;
        st->west_sec = info.offset.count();
        st->dst_sec = info.save.count() * 60;
    }

    if (dst_sec != nullptr) {
        *dst_sec = info.save.count() * 60;
    }
#endif // _WIN32

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_clock_settime(s32 clock_id, OrbisKernelTimespec* tp) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, clock_id: {}", clock_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_settimeofday(OrbisKernelTimeval* _tv, OrbisKernelTimezone* _tz) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelSettimeofday(OrbisKernelTimeval* _tv, OrbisKernelTimezone* _tz) {
    s32 ret = posix_settimeofday(_tv, _tz);
    if (ret < 0) {
        return ErrnoToSceKernelError(ret);
    }
    return ret;
}

void RegisterTime(Core::Loader::SymbolsResolver* sym) {
    clock = std::make_unique<Common::NativeClock>();
    initial_ptc = clock->GetUptime();

    // POSIX
    LIB_FUNCTION("NhpspxdjEKU", "libkernel", 1, "libkernel", posix_nanosleep);
    LIB_FUNCTION("NhpspxdjEKU", "libScePosix", 1, "libkernel", posix_nanosleep);
    LIB_FUNCTION("yS8U2TGCe1A", "libkernel", 1, "libkernel", posix_nanosleep);
    LIB_FUNCTION("yS8U2TGCe1A", "libScePosix", 1, "libkernel", posix_nanosleep);
    LIB_FUNCTION("QcteRwbsnV0", "libkernel", 1, "libkernel", posix_usleep);
    LIB_FUNCTION("QcteRwbsnV0", "libScePosix", 1, "libkernel", posix_usleep);
    LIB_FUNCTION("0wu33hunNdE", "libkernel", 1, "libkernel", posix_sleep);
    LIB_FUNCTION("0wu33hunNdE", "libScePosix", 1, "libkernel", posix_sleep);
    LIB_FUNCTION("lLMT9vJAck0", "libkernel", 1, "libkernel", posix_clock_gettime);
    LIB_FUNCTION("lLMT9vJAck0", "libScePosix", 1, "libkernel", posix_clock_gettime);
    LIB_FUNCTION("smIj7eqzZE8", "libkernel", 1, "libkernel", posix_clock_getres);
    LIB_FUNCTION("smIj7eqzZE8", "libScePosix", 1, "libkernel", posix_clock_getres);
    LIB_FUNCTION("n88vx3C5nW8", "libkernel", 1, "libkernel", posix_gettimeofday);
    LIB_FUNCTION("n88vx3C5nW8", "libScePosix", 1, "libkernel", posix_gettimeofday);
    LIB_FUNCTION("ChCOChPU-YM", "libkernel", 1, "libkernel", sceKernelSettimeofday);
    LIB_FUNCTION("VdXIDAbJ3tQ", "libScePosix", 1, "libkernel", posix_settimeofday);
    LIB_FUNCTION("d7nUj1LOdDU", "libScePosix", 1, "libkernel", posix_clock_settime);

    // Orbis
    LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel", sceKernelGetProcessTime);
    LIB_FUNCTION("fgxnMeTNUtY", "libkernel", 1, "libkernel", sceKernelGetProcessTimeCounter);
    LIB_FUNCTION("BNowx2l588E", "libkernel", 1, "libkernel",
                 sceKernelGetProcessTimeCounterFrequency);
    LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", sceKernelReadTsc);
    LIB_FUNCTION("1j3S3n-tTW4", "libkernel", 1, "libkernel", sceKernelGetTscFrequency);
    LIB_FUNCTION("QvsZxomvUHs", "libkernel", 1, "libkernel", sceKernelNanosleep);
    LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", sceKernelUsleep);
    LIB_FUNCTION("-ZR+hG7aDHw", "libkernel", 1, "libkernel", sceKernelSleep);
    LIB_FUNCTION("QBi7HCK03hw", "libkernel", 1, "libkernel", sceKernelClockGettime);
    LIB_FUNCTION("wRYVA5Zolso", "libkernel", 1, "libkernel", sceKernelClockGetres);
    LIB_FUNCTION("ejekcaNQNq0", "libkernel", 1, "libkernel", sceKernelGettimeofday);
    LIB_FUNCTION("kOcnerypnQA", "libkernel", 1, "libkernel", sceKernelGettimezone);
    LIB_FUNCTION("0NTHN1NKONI", "libkernel", 1, "libkernel", sceKernelConvertLocaltimeToUtc);
    LIB_FUNCTION("-o5uEDpN+oY", "libkernel", 1, "libkernel", sceKernelConvertUtcToLocaltime);
}

} // namespace Libraries::Kernel
