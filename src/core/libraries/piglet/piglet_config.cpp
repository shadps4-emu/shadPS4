// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <map>
#include <mutex>
#include <new>

#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/piglet/piglet.h"
#include "core/libraries/piglet/piglet_internal.h"

namespace Libraries::Piglet {

namespace {

constexpr u32 MEMORY_TYPE_SYSTEM = 0x802;
constexpr u32 MEMORY_TYPE_VIDEO = 0x804;

struct PigletConfigState {
    OrbisPigletConfigVSH config{};
    OrbisPigletShaderCacheConfiguration shader_cache_config{};
    bool initialized = false;
    // Memory handed out when the application did not provide its own allocator.
    std::map<void*, size_t> allocations; // Pointer to alignment.
};

PigletConfigState g_piglet;
std::mutex g_piglet_mutex;

// Piglet backs its memory with the heaps of its own library and of the compositor, which do not
// exist here. Plain host memory stands in for both.
void* AllocateMemory(u32 type, u32 size, u32 alignment, void* userdata) {
    OrbisPigletMemoryAllocFunc callback = nullptr;
    {
        std::scoped_lock lk{g_piglet_mutex};
        callback = g_piglet.config.memory_alloc;
    }
    if (callback != nullptr) {
        return callback(size, alignment, type, userdata);
    }
    const size_t align = std::max<size_t>(std::bit_ceil<size_t>(std::max<u32>(alignment, 1)),
                                          alignof(std::max_align_t));
    void* memory = ::operator new(std::max<size_t>(size, 1), std::align_val_t{align}, std::nothrow);
    if (memory != nullptr) {
        std::scoped_lock lk{g_piglet_mutex};
        g_piglet.allocations.emplace(memory, align);
    }
    return memory;
}

void ReleaseMemory(u32 type, void* ptr, void* userdata) {
    OrbisPigletMemoryFreeFunc callback = nullptr;
    {
        std::scoped_lock lk{g_piglet_mutex};
        callback = g_piglet.config.memory_free;
        if (callback == nullptr) {
            const auto it = g_piglet.allocations.find(ptr);
            if (it == g_piglet.allocations.end()) {
                LOG_ERROR(Lib_Piglet, "Releasing memory that was not allocated: {}", fmt::ptr(ptr));
                return;
            }
            ::operator delete(ptr, std::align_val_t{it->second});
            g_piglet.allocations.erase(it);
            return;
        }
    }
    callback(ptr, type, userdata);
}

} // namespace

void MarkPigletInitialized() {
    std::scoped_lock lk{g_piglet_mutex};
    g_piglet.initialized = true;
}

bool PS4_SYSV_ABI scePigletSetConfigurationVSH(const OrbisPigletConfigVSH* config) {
    if (config == nullptr) {
        return false;
    }
    std::scoped_lock lk{g_piglet_mutex};
    if (g_piglet.initialized) {
        LOG_ERROR(Lib_Piglet, "Setting Piglet configuration after initialization!");
    }
    std::memset(&g_piglet.config, 0, sizeof(g_piglet.config));
    u32 copy_size = PIGLET_CONFIG_VSH_SIZE;
    if (config->size < PIGLET_CONFIG_VSH_SIZE) {
        LOG_WARNING(Lib_Piglet, "Configuration smaller than expected; continuing anyway");
        copy_size = config->size;
    }
    std::memcpy(&g_piglet.config, config, copy_size);
    g_piglet.config.size = PIGLET_CONFIG_VSH_SIZE;
    return true;
}

bool PS4_SYSV_ABI scePigletGetConfigurationVSH(OrbisPigletConfigVSH* config) {
    if (config == nullptr) {
        return false;
    }
    const u32 size = config->size;
    if (size == 0) {
        return false;
    }
    std::scoped_lock lk{g_piglet_mutex};
    std::memset(config, 0, size);
    std::memcpy(config, &g_piglet.config, std::min(size, PIGLET_CONFIG_VSH_SIZE));
    config->size = size;
    return true;
}

void* PS4_SYSV_ABI scePigletAllocateVideoMemoryEx(u32 size, u32 alignment, void* userdata) {
    return AllocateMemory(MEMORY_TYPE_VIDEO, size, alignment, userdata);
}

void PS4_SYSV_ABI scePigletReleaseVideoMemoryEx(void* ptr, void* userdata) {
    ReleaseMemory(MEMORY_TYPE_VIDEO, ptr, userdata);
}

void* PS4_SYSV_ABI scePigletAllocateSystemMemoryEx(u32 size, u32 alignment, void* userdata) {
    return AllocateMemory(MEMORY_TYPE_SYSTEM, size, alignment, userdata);
}

void PS4_SYSV_ABI scePigletReleaseSystemMemoryEx(void* ptr, void* userdata) {
    ReleaseMemory(MEMORY_TYPE_SYSTEM, ptr, userdata);
}

bool PS4_SYSV_ABI
scePigletSetShaderCacheConfiguration(const OrbisPigletShaderCacheConfiguration* config) {
    if (config == nullptr) {
        return false;
    }
    if (config->version != PIGLET_SHADER_CACHE_CONFIG_VERSION) {
        LOG_ERROR(Lib_Piglet, "Unsupported shader cache configuration version {:#x}",
                  config->version);
        return false;
    }
    // TODO: Piglet also validates the cache path stored in the configuration.
    std::scoped_lock lk{g_piglet_mutex};
    std::memcpy(&g_piglet.shader_cache_config, config, sizeof(g_piglet.shader_cache_config));
    return true;
}

bool PS4_SYSV_ABI
scePigletGetShaderCacheConfiguration(OrbisPigletShaderCacheConfiguration* config) {
    if (config == nullptr) {
        return false;
    }
    if (config->version != PIGLET_SHADER_CACHE_CONFIG_VERSION) {
        LOG_ERROR(Lib_Piglet, "Unsupported shader cache configuration version {:#x}",
                  config->version);
        return false;
    }
    std::scoped_lock lk{g_piglet_mutex};
    std::memcpy(config, &g_piglet.shader_cache_config, sizeof(*config));
    return true;
}

bool PS4_SYSV_ABI scePigletGetInteger(s32 which, s64* value) {
    if (!IsDisplayCreated()) {
        LOG_ERROR(Lib_Piglet, "No valid display exists!");
        return false;
    }
    if (which != 1 || value == nullptr) {
        return false;
    }
    // Piglet reports the index it allocated from the compositor for this display. There is no
    // compositor, so the first index is reported.
    *value = 0;
    return true;
}

// The functions below report on Piglet's own heaps and on the compositor memory it is built on,
// neither of which exists here. They validate their arguments like Piglet does and report that no
// information is available.
bool PS4_SYSV_ABI scePigletGetUsageVSH(s32 kind, void* first, void* second) {
    if (first == nullptr || second == nullptr || kind < 1 || kind > 10) {
        return false;
    }
    LogStubOnce("scePigletGetUsageVSH");
    return false;
}

bool PS4_SYSV_ABI scePigletHeapGetInfo(void* heap, u32 type) {
    if (heap == nullptr || type > 2) {
        return false;
    }
    LogStubOnce("scePigletHeapGetInfo");
    return false;
}

bool PS4_SYSV_ABI scePigletHeapDump(void* heap, u32 type, u32 mode) {
    if (heap == nullptr || type > 2 || mode > 2) {
        return false;
    }
    LogStubOnce("scePigletHeapDump");
    return false;
}

void RegisterPigletApi(Core::Loader::SymbolsResolver* sym) {
    PIGLET_EXPORT("sDotUt1uEhA", scePigletAllocateVideoMemoryEx);
    PIGLET_EXPORT("AcruVcmKz78", scePigletReleaseVideoMemoryEx);
    PIGLET_EXPORT("oXkQEVitkCs", scePigletAllocateSystemMemoryEx);
    PIGLET_EXPORT("T-+xooU3VvQ", scePigletReleaseSystemMemoryEx);
    PIGLET_EXPORT("M9RtXpjSYtE", scePigletSetConfigurationVSH);
    PIGLET_EXPORT("3gtNzvkq-XY", scePigletGetConfigurationVSH);
    PIGLET_EXPORT("6u98-pOEZ7A", scePigletGetUsageVSH);
    PIGLET_EXPORT("PNO0IUA8nwc", scePigletHeapGetInfo);
    PIGLET_EXPORT("BHBgaf-ZEy4", scePigletHeapDump);
    PIGLET_EXPORT("oWhbxCJiBMM", scePigletGetInteger);
    PIGLET_EXPORT("nXwKrwF-wUM", scePigletSetShaderCacheConfiguration);
    PIGLET_EXPORT("cYSjErZW5gE", scePigletGetShaderCacheConfiguration);
}

} // namespace Libraries::Piglet
