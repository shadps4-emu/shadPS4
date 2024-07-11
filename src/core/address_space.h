// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "common/enum.h"
#include "common/types.h"

namespace Core {

enum class MemoryPermission : u32 {
    Read = 1 << 0,
    Write = 1 << 1,
    ReadWrite = Read | Write,
    Execute = 1 << 2,
    ReadWriteExecute = Read | Write | Execute,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryPermission)

constexpr VAddr SYSTEM_RESERVED = 0x800000000ULL;
constexpr VAddr CODE_BASE_OFFSET = 0x100000000ULL;
constexpr VAddr SYSTEM_MANAGED_MIN = 0x00000400000ULL;
constexpr VAddr SYSTEM_MANAGED_MAX = 0x07FFFFBFFFULL;
constexpr VAddr USER_MIN = 0x1000000000ULL;
constexpr VAddr USER_MAX = 0xFBFFFFFFFFULL;

// User area size is normally larger than this. However games are unlikely to map to high
// regions of that area, so by default we allocate a smaller virtual address space (about 1/4th).
// to save space on page tables.
static constexpr size_t UserSize = 1ULL << 39;
static constexpr size_t SystemSize = USER_MIN - SYSTEM_MANAGED_MIN;

/**
 * Represents the user virtual address space backed by a dmem memory block
 */
class AddressSpace {
public:
    explicit AddressSpace();
    ~AddressSpace();

    [[nodiscard]] VAddr VirtualBase() noexcept {
        return reinterpret_cast<VAddr>(virtual_base);
    }
    [[nodiscard]] const u8* VirtualBase() const noexcept {
        return virtual_base;
    }
    [[nodiscard]] size_t VirtualSize() const noexcept {
        return virtual_size;
    }

    /**
     * @brief Maps memory to the specified virtual address.
     * @param virtual_addr The base address to place the mapping.
     *        If zero is provided an address in system managed area is picked.
     * @param size The size of the area to map.
     * @param phys_addr The offset of the backing file handle to map.
     *                  The same backing region may be aliased into different virtual regions.
     *                  If zero is provided the mapping is considered as private.
     * @return A pointer to the mapped memory.
     */
    void* Map(VAddr virtual_addr, size_t size, u64 alignment = 0, PAddr phys_addr = -1,
              bool exec = false);

    /// Memory maps a specified file descriptor.
    void* MapFile(VAddr virtual_addr, size_t size, size_t offset, u32 prot, uintptr_t fd);

    /// Unmaps specified virtual memory area.
    void Unmap(VAddr virtual_addr, size_t size, bool has_backing);

    void Protect(VAddr virtual_addr, size_t size, MemoryPermission perms);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    u8* backing_base{};
    u8* virtual_base{};
    size_t virtual_size{};
};

} // namespace Core
