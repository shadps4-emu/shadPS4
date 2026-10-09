// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cerrno>
#include <utility>
#include "core/cpu_affinity.h"
#include "core/libraries/kernel/posix_error.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#elif defined(__FreeBSD__)
#include <pthread.h>
#include <pthread_np.h>
#include <sys/cpuset.h>
#endif

namespace Core {
namespace {

int ReadHostAffinity(uintptr_t thread, std::vector<int>& cpus) {
    cpus.clear();
#ifdef _WIN32
    GROUP_AFFINITY affinity{};
    const auto handle = thread == 0 ? GetCurrentThread() : reinterpret_cast<HANDLE>(thread);
    if (!GetThreadGroupAffinity(handle, &affinity)) {
        return POSIX_ESRCH;
    }
    for (int cpu = 0; cpu < sizeof(KAFFINITY) * 8; ++cpu) {
        if (affinity.Mask & (KAFFINITY{1} << cpu)) {
            cpus.push_back((static_cast<int>(affinity.Group) << 16) | cpu);
        }
    }
#elif defined(__linux__)
    const auto handle = thread == 0 ? pthread_self() : static_cast<pthread_t>(thread);
    for (size_t count = CPU_SETSIZE; count <= 65536; count *= 2) {
        const auto size = CPU_ALLOC_SIZE(count);
        auto* affinity = CPU_ALLOC(count);
        if (affinity == nullptr) {
            return POSIX_ENOMEM;
        }
        CPU_ZERO_S(size, affinity);
        const int ret = pthread_getaffinity_np(handle, size, affinity);
        if (ret == 0) {
            for (size_t cpu = 0; cpu < count; ++cpu) {
                if (CPU_ISSET_S(cpu, size, affinity)) {
                    cpus.push_back(static_cast<int>(cpu));
                }
            }
        }
        CPU_FREE(affinity);
        if (ret == 0) {
            break;
        }
        if (ret != EINVAL) {
            return ret == ESRCH ? POSIX_ESRCH : POSIX_EINVAL;
        }
    }
#elif defined(__FreeBSD__)
    cpuset_t affinity{};
    const auto handle = thread == 0 ? pthread_self() : reinterpret_cast<pthread_t>(thread);
    const int ret = pthread_getaffinity_np(handle, sizeof(affinity), &affinity);
    if (ret != 0) {
        return ret == ESRCH ? POSIX_ESRCH : POSIX_EINVAL;
    }
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &affinity)) {
            cpus.push_back(cpu);
        }
    }
#else
    return 0;
#endif
    return cpus.empty() ? POSIX_EAGAIN : 0;
}

int ApplyHostAffinity(uintptr_t thread, const std::vector<int>& cpus) {
#ifdef _WIN32
    GROUP_AFFINITY affinity{};
    affinity.Group = static_cast<WORD>(cpus.front() >> 16);
    for (const int cpu : cpus) {
        if ((cpu >> 16) == affinity.Group) {
            affinity.Mask |= KAFFINITY{1} << (cpu & 0xffff);
        }
    }
    const auto handle = thread == 0 ? GetCurrentThread() : reinterpret_cast<HANDLE>(thread);
    return SetThreadGroupAffinity(handle, &affinity, nullptr) ? 0 : POSIX_EINVAL;
#elif defined(__linux__)
    const auto count = cpus.back() + 1;
    const auto size = CPU_ALLOC_SIZE(count);
    auto* affinity = CPU_ALLOC(count);
    if (affinity == nullptr) {
        return POSIX_ENOMEM;
    }
    CPU_ZERO_S(size, affinity);
    for (const int cpu : cpus) {
        CPU_SET_S(cpu, size, affinity);
    }
    const auto handle = thread == 0 ? pthread_self() : static_cast<pthread_t>(thread);
    const int ret = pthread_setaffinity_np(handle, size, affinity);
    CPU_FREE(affinity);
    return ret == 0 ? 0 : ret == ESRCH ? POSIX_ESRCH : POSIX_EINVAL;
#elif defined(__FreeBSD__)
    cpuset_t affinity{};
    CPU_ZERO(&affinity);
    for (const int cpu : cpus) {
        CPU_SET(cpu, &affinity);
    }
    const auto handle = thread == 0 ? pthread_self() : reinterpret_cast<pthread_t>(thread);
    const int ret = pthread_setaffinity_np(handle, sizeof(affinity), &affinity);
    return ret == 0 ? 0 : ret == ESRCH ? POSIX_ESRCH : POSIX_EINVAL;
#else
    return 0;
#endif
}

int CurrentHostCpu() {
#ifdef _WIN32
    PROCESSOR_NUMBER current{};
    GetCurrentProcessorNumberEx(&current);
    return (static_cast<int>(current.Group) << 16) | current.Number;
#elif defined(__linux__) || defined(__FreeBSD__)
    return sched_getcpu();
#else
    return -1;
#endif
}

} // namespace

CpuAffinity::CpuAffinity(std::vector<int> allowed) {
    Remap(std::move(allowed));
}

void CpuAffinity::Remap(std::vector<int> allowed) {
    allowed_cpus = std::move(allowed);
    if (!allowed_cpus.empty()) {
        for (size_t guest = 0; guest < host_cpus.size(); ++guest) {
            host_cpus[guest] = allowed_cpus[guest % allowed_cpus.size()];
        }
    }
}

int CpuAffinity::Refresh(uintptr_t thread) {
    std::vector<int> effective;
    const int ret = ReadHostAffinity(thread, effective);
    if (ret != 0) {
        return ret;
    }
    if (allowed_cpus.empty() || (!applied_cpus.empty() && effective != applied_cpus)) {
        Remap(effective);
        applied_cpus = std::move(effective);
    }
    return 0;
}

int CpuAffinity::GetAllowedHostCpus(uintptr_t thread, std::vector<int>& cpus) {
    const int ret = Refresh(thread);
    if (ret == 0) {
        cpus = allowed_cpus;
    }
    return ret;
}

int CpuAffinity::SetThreadAffinity(uintptr_t thread, u64 guest_mask) {
    if (guest_mask == 0 || (guest_mask & ~u64{0xff}) != 0) {
        return POSIX_EINVAL;
    }
#if !defined(_WIN32) && !defined(__linux__) && !defined(__FreeBSD__)
    return 0;
#else
    if (const int ret = Refresh(thread); ret != 0) {
        return ret;
    }
    for (int attempt = 0; attempt < 3; ++attempt) {
        std::vector<int> requested;
        for (size_t guest = 0; guest < host_cpus.size(); ++guest) {
            if (guest_mask & (u64{1} << guest)) {
                requested.push_back(host_cpus[guest]);
            }
        }
        std::ranges::sort(requested);
        requested.erase(std::unique(requested.begin(), requested.end()), requested.end());
        const int ret = ApplyHostAffinity(thread, requested);
        if (ret != 0 && ret != POSIX_EINVAL) {
            return ret;
        }
        std::vector<int> effective;
        if (const int read_ret = ReadHostAffinity(thread, effective); read_ret != 0) {
            return read_ret;
        }
        if (ret == 0 && effective == requested) {
            applied_cpus = std::move(effective);
            return 0;
        }
        Remap(effective);
        applied_cpus = std::move(effective);
    }
    return POSIX_EAGAIN;
#endif
}

int CpuAffinity::CurrentGuestCpu(u64 guest_mask) {
    if (guest_mask == 0 || (guest_mask & ~u64{0xff}) != 0) {
        return -1;
    }
#if !defined(_WIN32) && !defined(__linux__) && !defined(__FreeBSD__)
    return std::countr_zero(guest_mask);
#else
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int host_cpu = CurrentHostCpu();
        if (host_cpu < 0) {
            return -1;
        }
        if (!allowed_cpus.empty()) {
            for (size_t guest = 0; guest < host_cpus.size(); ++guest) {
                if ((guest_mask & (u64{1} << guest)) != 0 && host_cpus[guest] == host_cpu) {
                    return static_cast<int>(guest);
                }
            }
        }
        if (SetThreadAffinity(0, guest_mask) != 0) {
            return -1;
        }
    }
    return -1;
#endif
}

} // namespace Core
