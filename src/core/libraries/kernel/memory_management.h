// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/types.h"

constexpr u64 SCE_KERNEL_MAIN_DMEM_SIZE = 5056_MB; // ~ 5GB
// TODO: Confirm this value on hardware.
constexpr u64 SCE_KERNEL_MAIN_DMEM_SIZE_PRO = 5568_MB; // ~ 5.5GB

namespace Libraries::Kernel {

enum MemoryTypes : u32 {
    SCE_KERNEL_WB_ONION = 0,  // write - back mode (Onion bus)
    SCE_KERNEL_WC_GARLIC = 3, // write - combining mode (Garlic bus)
    SCE_KERNEL_WB_GARLIC = 10 // write - back mode (Garlic bus)
};

enum MemoryFlags : u32 {
    SCE_KERNEL_MAP_FIXED = 0x0010, // Fixed
    SCE_KERNEL_MAP_NO_OVERWRITE = 0x0080,
    SCE_KERNEL_MAP_NO_COALESCE = 0x400000
};

enum MemoryProtection : u32 {
    SCE_KERNEL_PROT_CPU_READ = 0x01,  // Permit reads from the CPU
    SCE_KERNEL_PROT_CPU_RW = 0x02,    // Permit reads/writes from the CPU
    SCE_KERNEL_PROT_CPU_WRITE = 0x02, // Permit reads/writes from the CPU (same)
    SCE_KERNEL_PROT_GPU_READ = 0x10,  // Permit reads from the GPU
    SCE_KERNEL_PROT_GPU_WRITE = 0x20, // Permit writes from the GPU
    SCE_KERNEL_PROT_GPU_RW = 0x30     // Permit reads/writes from the GPU
};

enum MemoryOpTypes : u32 {
    ORBIS_KERNEL_MAP_OP_MAP_DIRECT = 0,
    ORBIS_KERNEL_MAP_OP_UNMAP = 1,
    ORBIS_KERNEL_MAP_OP_PROTECT = 2,
    ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE = 3,
    ORBIS_KERNEL_MAP_OP_TYPE_PROTECT = 4
};

struct OrbisQueryInfo {
    uintptr_t start;
    uintptr_t end;
    int memoryType;
};

struct OrbisVirtualQueryInfo {
    uintptr_t start;
    uintptr_t end;
    size_t offset;
    s32 protection;
    s32 memory_type;
    union {
        BitField<0, 1, u32> is_flexible;
        BitField<1, 1, u32> is_direct;
        BitField<2, 1, u32> is_stack;
        BitField<3, 1, u32> is_pooled;
        BitField<4, 1, u32> is_committed;
    };
    std::array<char, 32> name;
};

struct OrbisKernelBatchMapEntry {
    void* start;
    size_t offset;
    size_t length;
    char protection;
    char type;
    short reserved;
    int operation;
};

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize();
int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                               u64 alignment, int memoryType, s64* physAddrOut);
int PS4_SYSV_ABI sceKernelMapNamedDirectMemory(void** addr, u64 len, int prot, int flags,
                                               s64 directMemoryStart, u64 alignment,
                                               const char* name);
int PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, int prot, int flags,
                                          s64 directMemoryStart, u64 alignment);
s32 PS4_SYSV_ABI sceKernelAllocateMainDirectMemory(size_t len, size_t alignment, int memoryType,
                                                   s64* physAddrOut);
s32 PS4_SYSV_ABI sceKernelReleaseDirectMemory(u64 start, size_t len);
s32 PS4_SYSV_ABI sceKernelCheckedReleaseDirectMemory(u64 start, size_t len);
s32 PS4_SYSV_ABI sceKernelAvailableDirectMemorySize(u64 searchStart, u64 searchEnd,
                                                    size_t alignment, u64* physAddrOut,
                                                    size_t* sizeOut);
s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, int flags, OrbisVirtualQueryInfo* info,
                                       size_t infoSize);
s32 PS4_SYSV_ABI sceKernelReserveVirtualRange(void** addr, u64 len, int flags, u64 alignment);
s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addrInOut, std::size_t len, int prot,
                                                 int flags, const char* name);
s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                            int flags);
int PS4_SYSV_ABI sceKernelQueryMemoryProtection(void* addr, void** start, void** end, u32* prot);

int PS4_SYSV_ABI sceKernelMProtect(const void* addr, size_t size, int prot);

int PS4_SYSV_ABI sceKernelMTypeProtect(const void* addr, size_t size, int mtype, int prot);

int PS4_SYSV_ABI sceKernelDirectMemoryQuery(u64 offset, int flags, OrbisQueryInfo* query_info,
                                            size_t infoSize);
s32 PS4_SYSV_ABI sceKernelAvailableFlexibleMemorySize(size_t* sizeOut);
void PS4_SYSV_ABI _sceKernelRtldSetApplicationHeapAPI(void* func[]);
int PS4_SYSV_ABI sceKernelGetDirectMemoryType(u64 addr, int* directMemoryTypeOut,
                                              void** directMemoryStartOut,
                                              void** directMemoryEndOut);

s32 PS4_SYSV_ABI sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, int numEntries,
                                   int* numEntriesOut);
s32 PS4_SYSV_ABI sceKernelBatchMap2(OrbisKernelBatchMapEntry* entries, int numEntries,
                                    int* numEntriesOut, int flags);

s32 PS4_SYSV_ABI sceKernelSetVirtualRangeName(const void* addr, size_t len, const char* name);

s32 PS4_SYSV_ABI sceKernelMemoryPoolExpand(u64 searchStart, u64 searchEnd, size_t len,
                                           size_t alignment, u64* physAddrOut);
s32 PS4_SYSV_ABI sceKernelMemoryPoolReserve(void* addrIn, size_t len, size_t alignment, int flags,
                                            void** addrOut);
s32 PS4_SYSV_ABI sceKernelMemoryPoolCommit(void* addr, size_t len, int type, int prot, int flags);
s32 PS4_SYSV_ABI sceKernelMemoryPoolDecommit(void* addr, size_t len, int flags);

} // namespace Libraries::Kernel
