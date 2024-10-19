// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "common/arch.h"
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

constexpr VAddr CODE_BASE_OFFSET = 0x100000000ULL;

constexpr VAddr SYSTEM_MANAGED_MIN = 0x00000400000ULL;
constexpr VAddr SYSTEM_MANAGED_MAX = 0x07FFFFBFFFULL;
constexpr VAddr SYSTEM_RESERVED_MIN = 0x07FFFFC000ULL;
#if defined(__APPLE__) && defined(ARCH_X86_64)
// Can only comfortably reserve the first 0x7C0000000 of system reserved space.
constexpr VAddr SYSTEM_RESERVED_MAX = 0xFBFFFFFFFULL;
#else
constexpr VAddr SYSTEM_RESERVED_MAX = 0xFFFFFFFFFULL;
#endif
constexpr VAddr USER_MIN = 0x1000000000ULL;
constexpr VAddr USER_MAX = 0xFBFFFFFFFFULL;

static constexpr size_t SystemManagedSize = SYSTEM_MANAGED_MAX - SYSTEM_MANAGED_MIN + 1;
static constexpr size_t SystemReservedSize = SYSTEM_RESERVED_MAX - SYSTEM_RESERVED_MIN + 1;
static constexpr size_t UserSize = 1ULL << 40;

/**
 * Represents the user virtual address space backed by a dmem memory block
 */
class AddressSpace {
public:
    explicit AddressSpace();
    ~AddressSpace();

    [[nodiscard]] u8* BackingBase() const noexcept {
        return backing_base;
    }

    [[nodiscard]] VAddr SystemManagedVirtualBase() noexcept {
        return reinterpret_cast<VAddr>(system_managed_base);
    }
    [[nodiscard]] const u8* SystemManagedVirtualBase() const noexcept {
        return system_managed_base;
    }
    [[nodiscard]] size_t SystemManagedVirtualSize() const noexcept {
        return system_managed_size;
    }

    [[nodiscard]] VAddr SystemReservedVirtualBase() noexcept {
        return reinterpret_cast<VAddr>(system_reserved_base);
    }
    [[nodiscard]] const u8* SystemReservedVirtualBase() const noexcept {
        return system_reserved_base;
    }
    [[nodiscard]] size_t SystemReservedVirtualSize() const noexcept {
        return system_reserved_size;
    }

    [[nodiscard]] VAddr UserVirtualBase() noexcept {
        return reinterpret_cast<VAddr>(user_base);
    }
    [[nodiscard]] const u8* UserVirtualBase() const noexcept {
        return user_base;
    }
    [[nodiscard]] size_t UserVirtualSize() const noexcept {
        return user_size;
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
    void Unmap(VAddr virtual_addr, size_t size, VAddr start_in_vma, VAddr end_in_vma,
               PAddr phys_base, bool is_exec, bool has_backing, bool readonly_file);

    void Protect(VAddr virtual_addr, size_t size, MemoryPermission perms);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    u8* backing_base{};
    u8* system_managed_base{};
    size_t system_managed_size{};
    u8* system_reserved_base{};
    size_t system_reserved_size{};
    u8* user_base{};
    size_t user_size{};
};

} // namespace Core
