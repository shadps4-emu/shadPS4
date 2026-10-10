// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <span>
#include <utility>
#include <vector>
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

struct CpuAffinityMap {
    explicit CpuAffinityMap(std::span<const int> allowed)
        : host_count{static_cast<size_t>(allowed[std::min<size_t>(8, allowed.size()) - 1]) + 1},
          host_to_guest{std::make_unique<u8[]>(host_count)} {
        for (size_t guest = 0; guest < guest_to_host.size(); ++guest) {
            const int host = allowed[guest % allowed.size()];
            guest_to_host[guest] = host;
            host_to_guest[host] |= 1U << guest;
        }
    }

    const size_t host_count;
    const std::unique_ptr<u8[]> host_to_guest;
    std::array<int, 8> guest_to_host{};
};

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

int ApplyHostAffinity(uintptr_t thread, std::span<const int> cpus) {
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

int CpuAffinity::SetThreadAffinity(uintptr_t thread, u64 guest_mask) {
    if (guest_mask == 0 || (guest_mask & ~u64{0xff}) != 0) {
        return POSIX_EINVAL;
    }
#if !defined(_WIN32) && !defined(__linux__) && !defined(__FreeBSD__)
    return 0;
#else
    if (!mapping) {
        static const auto initial = [] {
            std::vector<int> allowed;
            const int ret = ReadHostAffinity(0, allowed);
            return std::pair{ret == 0 ? std::make_shared<const CpuAffinityMap>(allowed) : nullptr,
                             ret};
        }();
        if (initial.second != 0) {
            return initial.second;
        }
        mapping = initial.first;
    }
    for (int attempt = 0; attempt < 3; ++attempt) {
        auto requested = mapping->guest_to_host;
        std::ranges::sort(requested);
        auto end = std::unique(requested.begin(), requested.end());
        end = std::remove_if(requested.begin(), end, [&](int host) {
            return (mapping->host_to_guest[host] & guest_mask) == 0;
        });
        const int ret = ApplyHostAffinity(thread, {requested.begin(), end});
        if (ret != POSIX_EINVAL) {
            return ret;
        }
        std::vector<int> effective;
        if (const int read_ret = ReadHostAffinity(thread, effective); read_ret != 0) {
            return read_ret;
        }
        mapping = std::make_shared<const CpuAffinityMap>(effective);
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
        if (mapping && static_cast<size_t>(host_cpu) < mapping->host_count) {
            const u64 candidates = mapping->host_to_guest[host_cpu] & guest_mask;
            if (candidates != 0) {
                return std::countr_zero(candidates);
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
