#pragma once
#include "common/types.h"

constexpr u64 SYSTEM_RESERVED = 0x800000000u;
constexpr u64 CODE_BASE_OFFSET = 0x100000000u;
constexpr u64 SYSTEM_MANAGED_MIN = 0x0000040000u;
constexpr u64 SYSTEM_MANAGED_MAX = 0x07FFFFBFFFu;
constexpr u64 USER_MIN = 0x1000000000u;
constexpr u64 USER_MAX = 0xFBFFFFFFFFu;

namespace VirtualMemory {
enum class MemoryMode : u32 {
    NoAccess = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Execute = 4,
    ExecuteRead = 5,
    ExecuteWrite = 6,
    ExecuteReadWrite = 7,
};
u64 memory_alloc(u64 address, u64 size, MemoryMode mode);
u64 memory_alloc_aligned(u64 address, u64 size, MemoryMode mode, u64 alignment);
bool memory_protect(u64 address, u64 size, MemoryMode mode, MemoryMode* old_mode);
bool memory_flush(u64 address, u64 size);
bool memory_patch(u64 vaddr, u64 value);

inline bool containsExecuteMode(MemoryMode mode) {
    switch (mode) {
        case MemoryMode::Execute: return true;
        case MemoryMode::ExecuteRead: return true;
        case MemoryMode::ExecuteWrite: return true;
        case MemoryMode::ExecuteReadWrite: return true;
        default: return false;
    }
}

}  // namespace VirtualMemory