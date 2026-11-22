// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/types.h"

constexpr u64 ORBIS_KERNEL_TOTAL_MEM = 5248_MB;
constexpr u64 ORBIS_KERNEL_TOTAL_MEM_PRO = 5888_MB;
constexpr u64 ORBIS_KERNEL_TOTAL_MEM_DEV = 6656_MB;
// TODO: This value needs confirmation
constexpr u64 ORBIS_KERNEL_TOTAL_MEM_DEV_PRO = 7936_MB;

constexpr u64 ORBIS_KERNEL_FLEXIBLE_MEMORY_BASE = 64_MB;
constexpr u64 ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE = 512_MB;

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

enum MemoryTypes : u32 {
    ORBIS_KERNEL_WB_ONION = 0,  // write - back mode (Onion bus)
    ORBIS_KERNEL_WC_GARLIC = 3, // write - combining mode (Garlic bus)
    ORBIS_KERNEL_WB_GARLIC = 10 // write - back mode (Garlic bus)
};

enum MemoryProtection : u32 {
    ORBIS_KERNEL_PROT_CPU_READ = 0x01,  // Permit reads from the CPU
    ORBIS_KERNEL_PROT_CPU_RW = 0x02,    // Permit reads/writes from the CPU
    ORBIS_KERNEL_PROT_CPU_WRITE = 0x02, // Permit reads/writes from the CPU (same)
    ORBIS_KERNEL_PROT_GPU_READ = 0x10,  // Permit reads from the GPU
    ORBIS_KERNEL_PROT_GPU_WRITE = 0x20, // Permit writes from the GPU
    ORBIS_KERNEL_PROT_GPU_RW = 0x30     // Permit reads/writes from the GPU
};

enum MemoryOpTypes : u32 {
    ORBIS_KERNEL_MAP_OP_MAP_DIRECT = 0,
    ORBIS_KERNEL_MAP_OP_UNMAP = 1,
    ORBIS_KERNEL_MAP_OP_PROTECT = 2,
    ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE = 3,
    ORBIS_KERNEL_MAP_OP_TYPE_PROTECT = 4
};

constexpr u32 ORBIS_KERNEL_MAXIMUM_NAME_LENGTH = 32;

struct OrbisQueryInfo {
    uintptr_t start;
    uintptr_t end;
    s32 memoryType;
};

struct OrbisVirtualQueryInfo {
    uintptr_t start;
    uintptr_t end;
    u64 offset;
    s32 protection;
    s32 memory_type;
    u8 is_flexible : 1;
    u8 is_direct : 1;
    u8 is_stack : 1;
    u8 is_pooled : 1;
    u8 is_committed : 1;
    char name[ORBIS_KERNEL_MAXIMUM_NAME_LENGTH];
};
static_assert(sizeof(OrbisVirtualQueryInfo) == 72,
              "OrbisVirtualQueryInfo struct size is incorrect");

struct OrbisKernelBatchMapEntry {
    VAddr start;
    u64 offset;
    u64 length;
    char protection;
    char type;
    s16 reserved;
    s32 operation;
};

enum class OrbisKernelMemoryPoolOpcode : u32 {
    Commit = 1,
    Decommit = 2,
    Protect = 3,
    TypeProtect = 4,
    Move = 5,
};

struct OrbisKernelMemoryPoolBatchEntry {
    OrbisKernelMemoryPoolOpcode opcode;
    u32 flags;
    union {
        struct {
            VAddr addr;
            u64 len;
            u8 prot;
            u8 type;
        } commit_params;
        struct {
            VAddr addr;
            u64 len;
        } decommit_params;
        struct {
            VAddr addr;
            u64 len;
            u8 prot;
        } protect_params;
        struct {
            VAddr addr;
            u64 len;
            u8 prot;
            u8 type;
        } type_protect_params;
        struct {
            VAddr dest_addr;
            VAddr src_addr;
            u64 len;
        } move_params;
        uintptr_t padding[3];
    };
};

struct OrbisKernelMemoryPoolBlockStats {
    s32 available_flushed_blocks;
    s32 available_cached_blocks;
    s32 allocated_flushed_blocks;
    s32 allocated_cached_blocks;
};

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize();
s32 PS4_SYSV_ABI sceKernelAllocateDirectMemory(PAddr search_start, PAddr search_end, u64 size,
                                               u64 alignment, s32 memory_type, PAddr* out_addr);
s32 PS4_SYSV_ABI sceKernelReleaseDirectMemory(u64 start, u64 len);
s32 PS4_SYSV_ABI sceKernelCheckedReleaseDirectMemory(u64 start, u64 len);
s32 PS4_SYSV_ABI sceKernelAvailableDirectMemorySize(u64 search_start, u64 search_end, u64 alignment,
                                                    PAddr* out_addr, u64* out_size);
s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, s32 flags, OrbisVirtualQueryInfo* info,
                                       u64 infoSize);
s32 PS4_SYSV_ABI sceKernelMapNamedDirectMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                               s64 phys_addr, u64 alignment, const char* name);
s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                                 const char* name);
s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags);
s32 PS4_SYSV_ABI sceKernelMapNamedSystemFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                                       const char* name);
s32 PS4_SYSV_ABI posix_mlock(const void* addr, u64 len);
s32 PS4_SYSV_ABI sceKernelMlock(void* addr, u64 len);
s32 PS4_SYSV_ABI posix_munlock(const void* addr, u64 len);
s32 PS4_SYSV_ABI sceKernelMunlock(void* addr, u64 len);
s32 PS4_SYSV_ABI sceKernelReleaseFlexibleMemory(void* addr, u64 len);

s32 PS4_SYSV_ABI sceKernelDirectMemoryQuery(PAddr addr, s32 flags, OrbisQueryInfo* query_info,
                                            u64 info_size);
s32 PS4_SYSV_ABI sceKernelAvailableFlexibleMemorySize(u64* sizeOut);
void PS4_SYSV_ABI _sceKernelRtldSetApplicationHeapAPI(void* func[]);

s32 PS4_SYSV_ABI sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, s32 numEntries,
                                   s32* numEntriesOut);
s32 PS4_SYSV_ABI sceKernelBatchMap2(OrbisKernelBatchMapEntry* entries, s32 numEntries,
                                    s32* numEntriesOut, s32 flags);

s32 PS4_SYSV_ABI sceKernelSetVirtualRangeName(const void* addr, u64 len, const char* name);
s32 PS4_SYSV_ABI sceKernelClearVirtualRangeName(const void* addr, u64 len);

s32 PS4_SYSV_ABI sceKernelMemoryPoolBatch(const OrbisKernelMemoryPoolBatchEntry* entries, s32 count,
                                          s32* num_processed, s32 flags);
s32 PS4_SYSV_ABI sceKernelMemoryPoolGetBlockStats(OrbisKernelMemoryPoolBlockStats* stats, u64 size);

s32 PS4_SYSV_ABI sceKernelMunmap(VAddr addr, u64 len);

void RegisterMemory(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
