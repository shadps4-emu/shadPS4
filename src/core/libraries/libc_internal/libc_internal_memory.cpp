// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "libc_internal_memory.h"

namespace Libraries::LibcInternal {

void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

s32 PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI internal_memmove(void* dest, const void* src, size_t n) {
    return std::memmove(dest, src, n);
}

struct SceLibcMallocManagedSize {
    u16 size;
    u16 version;
    u32 reserved;
    u64 max_system_size;
    u64 current_system_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
};
static_assert(sizeof(SceLibcMallocManagedSize) == 0x28);

namespace {

constexpr u64 MspaceAlignment = 16;
constexpr u64 MspaceMinCapacity = 0x580;
constexpr u64 MspaceMapGranularity = 0x10000;
constexpr u32 MspaceMaxFlags = 0xf;
constexpr u16 MallocManagedSizeVersion = 1;
constexpr s32 MspaceProt =
    Libraries::Kernel::ORBIS_KERNEL_PROT_CPU_READ | Libraries::Kernel::ORBIS_KERNEL_PROT_CPU_RW;

struct Mspace {
    u8* base;
    u64 size;
    bool owns_memory;
    u64 current_inuse;
    u64 max_inuse;
    std::map<u64, u64> free_blocks;
    std::unordered_map<u64, u64> used_blocks;
};

std::mutex g_mspace_mutex;
std::map<uintptr_t, std::unique_ptr<Mspace>> g_mspaces;

Mspace* FindMspace(void* handle) {
    const auto it = g_mspaces.find(reinterpret_cast<uintptr_t>(handle));
    return it != g_mspaces.end() ? it->second.get() : nullptr;
}

Mspace* FindMspaceByPointer(const void* ptr) {
    const auto addr = reinterpret_cast<uintptr_t>(ptr);
    auto it = g_mspaces.upper_bound(addr);
    if (it == g_mspaces.begin()) {
        return nullptr;
    }
    --it;
    const auto* mspace = it->second.get();
    return addr < it->first + mspace->size ? it->second.get() : nullptr;
}

void* AllocateBlock(Mspace& mspace, u64 size, u64 alignment) {
    if (size > mspace.size) {
        return nullptr;
    }
    alignment = std::max(alignment, MspaceAlignment);
    const u64 block_size = std::max(Common::AlignUp(size, MspaceAlignment), MspaceAlignment);
    const auto base_addr = reinterpret_cast<uintptr_t>(mspace.base);

    for (auto it = mspace.free_blocks.begin(); it != mspace.free_blocks.end(); ++it) {
        const u64 block_offset = it->first;
        const u64 block_length = it->second;
        const u64 aligned_offset = Common::AlignUp(base_addr + block_offset, alignment) - base_addr;
        const u64 padding = aligned_offset - block_offset;
        if (block_length < padding + block_size) {
            continue;
        }

        const u64 tail_length = block_length - padding - block_size;
        mspace.free_blocks.erase(it);
        if (padding != 0) {
            mspace.free_blocks.emplace(block_offset, padding);
        }
        if (tail_length != 0) {
            mspace.free_blocks.emplace(aligned_offset + block_size, tail_length);
        }

        mspace.used_blocks.emplace(aligned_offset, block_size);
        mspace.current_inuse += block_size;
        mspace.max_inuse = std::max(mspace.max_inuse, mspace.current_inuse);
        return mspace.base + aligned_offset;
    }
    return nullptr;
}

bool ReleaseBlock(Mspace& mspace, void* ptr) {
    const u64 offset = static_cast<u8*>(ptr) - mspace.base;
    const auto used_it = mspace.used_blocks.find(offset);
    if (used_it == mspace.used_blocks.end()) {
        return false;
    }

    u64 free_offset = offset;
    u64 free_length = used_it->second;
    mspace.current_inuse -= free_length;
    mspace.used_blocks.erase(used_it);

    auto next_it = mspace.free_blocks.lower_bound(free_offset);
    if (next_it != mspace.free_blocks.end() && free_offset + free_length == next_it->first) {
        free_length += next_it->second;
        next_it = mspace.free_blocks.erase(next_it);
    }
    if (next_it != mspace.free_blocks.begin()) {
        const auto prev_it = std::prev(next_it);
        if (prev_it->first + prev_it->second == free_offset) {
            free_offset = prev_it->first;
            free_length += prev_it->second;
            mspace.free_blocks.erase(prev_it);
        }
    }
    mspace.free_blocks.emplace(free_offset, free_length);
    return true;
}

} // namespace

void* PS4_SYSV_ABI sceLibcMspaceCreate(const char* name, void* base, size_t size, u32 flags) {
    LOG_INFO(Lib_LibcInternal, "called, name = {}, base = {:p}, size = {:#x}, flags = {:#x}",
             name ? name : "(null)", base, size, flags);
    if (flags > MspaceMaxFlags) {
        LOG_ERROR(Lib_LibcInternal, "invalid flags {:#x}", flags);
        return nullptr;
    }

    auto mspace = std::make_unique<Mspace>();
    if (base != nullptr) {
        const auto base_addr = reinterpret_cast<uintptr_t>(base);
        if (!Common::IsAligned(base_addr, 8) || !Common::IsAligned(size, 8) ||
            size <= MspaceMinCapacity) {
            LOG_ERROR(Lib_LibcInternal, "invalid base {:p} or size {:#x}", base, size);
            return nullptr;
        }
        mspace->base = static_cast<u8*>(base);
        mspace->size = size;
        mspace->owns_memory = false;
    } else {
        const u64 map_size = Common::AlignUp(size, MspaceMapGranularity);
        void* addr = nullptr;
        if (map_size == 0 || Libraries::Kernel::sceKernelMapNamedFlexibleMemory(
                                 &addr, map_size, MspaceProt, 0, name) != ORBIS_OK) {
            LOG_ERROR(Lib_LibcInternal, "failed to map {:#x} bytes", map_size);
            return nullptr;
        }
        mspace->base = static_cast<u8*>(addr);
        mspace->size = map_size;
        mspace->owns_memory = true;
    }

    const auto base_addr = reinterpret_cast<uintptr_t>(mspace->base);
    const u64 first_offset = Common::AlignUp(base_addr, MspaceAlignment) - base_addr;
    mspace->free_blocks.emplace(first_offset,
                                Common::AlignDown(mspace->size - first_offset, MspaceAlignment));

    void* handle = mspace->base;
    std::scoped_lock lk{g_mspace_mutex};
    g_mspaces.emplace(reinterpret_cast<uintptr_t>(handle), std::move(mspace));
    return handle;
}

s32 PS4_SYSV_ABI sceLibcMspaceDestroy(void* handle) {
    std::unique_ptr<Mspace> mspace;
    {
        std::scoped_lock lk{g_mspace_mutex};
        const auto it = g_mspaces.find(reinterpret_cast<uintptr_t>(handle));
        if (it == g_mspaces.end()) {
            LOG_ERROR(Lib_LibcInternal, "invalid mspace {:p}", handle);
            return 1;
        }
        mspace = std::move(it->second);
        g_mspaces.erase(it);
    }
    if (mspace->owns_memory) {
        Libraries::Kernel::sceKernelMunmap(mspace->base, mspace->size);
    }
    return 0;
}

void* PS4_SYSV_ABI sceLibcMspaceMalloc(void* handle, size_t size) {
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    return mspace ? AllocateBlock(*mspace, size, MspaceAlignment) : nullptr;
}

void PS4_SYSV_ABI sceLibcMspaceFree(void* handle, void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    if (!mspace || !ReleaseBlock(*mspace, ptr)) {
        LOG_ERROR(Lib_LibcInternal, "invalid free, mspace = {:p}, ptr = {:p}", handle, ptr);
    }
}

void* PS4_SYSV_ABI sceLibcMspaceCalloc(void* handle, size_t nelem, size_t size) {
    u64 total_size{};
    if (__builtin_mul_overflow(nelem, size, &total_size)) {
        return nullptr;
    }
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    void* ptr = mspace ? AllocateBlock(*mspace, total_size, MspaceAlignment) : nullptr;
    if (ptr != nullptr) {
        std::memset(ptr, 0, total_size);
    }
    return ptr;
}

void* PS4_SYSV_ABI sceLibcMspaceRealloc(void* handle, void* ptr, size_t size) {
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    if (!mspace) {
        return nullptr;
    }
    if (ptr == nullptr) {
        return AllocateBlock(*mspace, size, MspaceAlignment);
    }

    const u64 offset = static_cast<u8*>(ptr) - mspace->base;
    const auto used_it = mspace->used_blocks.find(offset);
    if (used_it == mspace->used_blocks.end()) {
        LOG_ERROR(Lib_LibcInternal, "invalid realloc, mspace = {:p}, ptr = {:p}", handle, ptr);
        return nullptr;
    }
    const u64 old_size = used_it->second;
    if (size <= old_size) {
        return ptr;
    }

    void* new_ptr = AllocateBlock(*mspace, size, MspaceAlignment);
    if (new_ptr == nullptr) {
        return nullptr;
    }
    std::memcpy(new_ptr, ptr, old_size);
    ReleaseBlock(*mspace, ptr);
    return new_ptr;
}

void* PS4_SYSV_ABI sceLibcMspaceMemalign(void* handle, size_t alignment, size_t size) {
    if (alignment > MspaceAlignment) {
        alignment = std::bit_ceil(alignment);
    }
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    return mspace ? AllocateBlock(*mspace, size, alignment) : nullptr;
}

s32 PS4_SYSV_ABI sceLibcMspacePosixMemalign(void* handle, void** ptr, size_t alignment,
                                            size_t size) {
    if (alignment < sizeof(void*) || !std::has_single_bit(alignment)) {
        return POSIX_EINVAL;
    }
    std::scoped_lock lk{g_mspace_mutex};
    auto* mspace = FindMspace(handle);
    void* result = mspace ? AllocateBlock(*mspace, size, alignment) : nullptr;
    if (result == nullptr) {
        return POSIX_ENOMEM;
    }
    *ptr = result;
    return 0;
}

size_t PS4_SYSV_ABI sceLibcMspaceMallocUsableSize(void* ptr) {
    if (ptr == nullptr) {
        return 0;
    }
    std::scoped_lock lk{g_mspace_mutex};
    const auto* mspace = FindMspaceByPointer(ptr);
    if (!mspace) {
        return 0;
    }
    const auto it = mspace->used_blocks.find(static_cast<u8*>(ptr) - mspace->base);
    return it != mspace->used_blocks.end() ? it->second : 0;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocStats(void* handle, SceLibcMallocManagedSize* stats) {
    std::scoped_lock lk{g_mspace_mutex};
    const auto* mspace = FindMspace(handle);
    if (!mspace) {
        return 1;
    }
    if (stats != nullptr) {
        if (stats->size != sizeof(SceLibcMallocManagedSize) ||
            stats->version != MallocManagedSizeVersion) {
            return 1;
        }
        stats->max_system_size = mspace->size;
        stats->current_system_size = mspace->size;
        stats->max_inuse_size = mspace->max_inuse;
        stats->current_inuse_size = mspace->current_inuse;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceLibcMspaceIsHeapEmpty(void* handle) {
    std::scoped_lock lk{g_mspace_mutex};
    const auto* mspace = FindMspace(handle);
    return mspace && mspace->used_blocks.empty();
}

static u64 g_mspace_atomic_id_mask = 0;
static u64 g_mstate_table[64] = {0};

struct HeapInfoInfo {
    u64 size = sizeof(HeapInfoInfo);
    u32 flag;
    u32 getSegmentInfo;
    u64* mspace_atomic_id_mask;
    u64* mstate_table;
};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy);
    LIB_FUNCTION("+P6FRGH4LfA", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memmove);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memset);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcmp);

    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal",
                 sceLibcHeapGetTraceInfo);

    LIB_FUNCTION("-hn1tcVHq5Q", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceCreate);
    LIB_FUNCTION("W6SiVSiCDtI", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceDestroy);
    LIB_FUNCTION("OJjm-QOIHlI", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceMalloc);
    LIB_FUNCTION("Vla-Z+eXlxo", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceFree);
    LIB_FUNCTION("LYo3GhIlB38", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceCalloc);
    LIB_FUNCTION("gigoVHZvVPE", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceRealloc);
    LIB_FUNCTION("iF1iQHzxBJU", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceMemalign);
    LIB_FUNCTION("qWESlyXMI3E", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspacePosixMemalign);
    LIB_FUNCTION("fEoW6BJsPt4", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceMallocUsableSize);
    LIB_FUNCTION("mfHdJTIvhuo", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceMallocStats);
    LIB_FUNCTION("k04jLXu3+Ic", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceMallocStats);
    LIB_FUNCTION("pzUa7KEoydw", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceIsHeapEmpty);
}

} // namespace Libraries::LibcInternal
