// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/icl/separate_interval_set.hpp>
#include "common/assert.h"
#include "common/error.h"
#include "core/address_space.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/virtual_memory.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace Core {

static constexpr size_t BackingSize = SCE_KERNEL_MAIN_DMEM_SIZE;
static constexpr size_t VirtualSize = USER_MAX - USER_MIN + 1;

#ifdef _WIN32
struct AddressSpace::Impl {
    Impl() : process{GetCurrentProcess()} {
        // Allocate backing file that represents the total physical memory.
        backing_handle =
            CreateFileMapping2(INVALID_HANDLE_VALUE, nullptr, FILE_MAP_WRITE | FILE_MAP_READ,
                               PAGE_READWRITE, SEC_COMMIT, BackingSize, nullptr, nullptr, 0);
        ASSERT(backing_handle);
        // Allocate a virtual memory for the backing file map as placeholder
        backing_base = static_cast<u8*>(VirtualAlloc2(process, nullptr, BackingSize,
                                                      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                      PAGE_NOACCESS, nullptr, 0));
        // Map backing placeholder. This will commit the pages
        void* const ret = MapViewOfFile3(backing_handle, process, backing_base, 0, BackingSize,
                                         MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
        ASSERT(ret == backing_base);
        // Allocate virtual address placeholder for our address space.
        MEM_ADDRESS_REQUIREMENTS req{};
        MEM_EXTENDED_PARAMETER param{};
        req.LowestStartingAddress = reinterpret_cast<PVOID>(USER_MIN);
        req.HighestEndingAddress = reinterpret_cast<PVOID>(USER_MAX);
        req.Alignment = 0;
        param.Type = MemExtendedParameterAddressRequirements;
        param.Pointer = &req;
        virtual_base = static_cast<u8*>(VirtualAlloc2(process, nullptr, VirtualSize,
                                                      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                      PAGE_NOACCESS, &param, 1));
        ASSERT(virtual_base);

        const uintptr_t virtual_addr = reinterpret_cast<uintptr_t>(virtual_base);
        placeholders.insert({virtual_addr, virtual_addr + VirtualSize});
    }

    ~Impl() {
        if (virtual_base) {
            if (!VirtualFree(virtual_base, 0, MEM_RELEASE)) {
                LOG_CRITICAL(Render, "Failed to free virtual memory");
            }
        }
        if (backing_base) {
            if (!UnmapViewOfFile2(process, backing_base, MEM_PRESERVE_PLACEHOLDER)) {
                LOG_CRITICAL(Render, "Failed to unmap backing memory placeholder");
            }
            if (!VirtualFreeEx(process, backing_base, 0, MEM_RELEASE)) {
                LOG_CRITICAL(Render, "Failed to free backing memory");
            }
        }
        if (!CloseHandle(backing_handle)) {
            LOG_CRITICAL(Render, "Failed to free backing memory file handle");
        }
    }

    void* MapUser(VAddr virtual_addr, PAddr phys_addr, size_t size, ULONG prot) {
        const auto it = placeholders.find(virtual_addr);
        ASSERT_MSG(it != placeholders.end(), "Cannot map already mapped region");
        ASSERT_MSG(virtual_addr >= it->lower() && virtual_addr + size <= it->upper(),
                   "Map range must be fully contained in a placeholder");

        // Windows only allows splitting a placeholder into two.
        // This means that if the map range is fully
        // contained the the placeholder we need to perform two split operations,
        // one at the start and at the end.
        const VAddr placeholder_start = it->lower();
        const VAddr placeholder_end = it->upper();
        const VAddr virtual_end = virtual_addr + size;

        // If the placeholder doesn't exactly start at virtual_addr, split it at the start.
        if (placeholder_start != virtual_addr) {
            VirtualFreeEx(process, reinterpret_cast<LPVOID>(placeholder_start),
                          virtual_addr - placeholder_start, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
        }

        // If the placeholder doesn't exactly end at virtual_end, split it at the end.
        if (placeholder_end != virtual_end) {
            VirtualFreeEx(process, reinterpret_cast<LPVOID>(virtual_end),
                          placeholder_end - virtual_end, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
        }

        // Remove the placeholder.
        placeholders.erase({virtual_addr, virtual_end});

        // Perform the map.
        void* ptr = nullptr;
        if (phys_addr != -1) {
            ptr = MapViewOfFile3(backing_handle, process, reinterpret_cast<PVOID>(virtual_addr),
                                 phys_addr, size, MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
        } else {
            ptr = VirtualAlloc2(process, reinterpret_cast<PVOID>(virtual_addr), size,
                                MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
        }
        ASSERT_MSG(ptr, "{}", Common::GetLastErrorMsg());
        return ptr;
    }

    void* MapPrivate(VAddr virtual_addr, size_t size, u64 alignment, ULONG prot) {
        // Map a private allocation
        MEM_ADDRESS_REQUIREMENTS req{};
        MEM_EXTENDED_PARAMETER param{};
        // req.LowestStartingAddress =
        //     (virtual_addr == 0 ? reinterpret_cast<PVOID>(SYSTEM_MANAGED_MIN)
        //                        : reinterpret_cast<PVOID>(virtual_addr));
        req.HighestEndingAddress = reinterpret_cast<PVOID>(SYSTEM_MANAGED_MAX);
        req.Alignment = alignment < 64_KB ? 0 : alignment;
        param.Type = MemExtendedParameterAddressRequirements;
        param.Pointer = &req;
        ULONG alloc_type = MEM_COMMIT | MEM_RESERVE | (alignment > 2_MB ? MEM_LARGE_PAGES : 0);
        void* const ptr = VirtualAlloc2(process, nullptr, size, alloc_type, prot, &param, 1);
        ASSERT_MSG(ptr, "{}", Common::GetLastErrorMsg());
        return ptr;
    }

    void UnmapUser(VAddr virtual_addr, size_t size) {
        const bool ret = UnmapViewOfFile2(process, reinterpret_cast<PVOID>(virtual_addr),
                                          MEM_PRESERVE_PLACEHOLDER);
        ASSERT_MSG(ret, "Unmap operation on virtual_addr={:#X} failed", virtual_addr);

        // The unmap call will create a new placeholder region. We need to see if we can coalesce it
        // with neighbors.
        VAddr placeholder_start = virtual_addr;
        VAddr placeholder_end = virtual_addr + size;

        // Check if a placeholder exists right before us.
        const auto left_it = placeholders.find(virtual_addr - 1);
        if (left_it != placeholders.end()) {
            ASSERT_MSG(left_it->upper() == virtual_addr,
                       "Left placeholder does not end at virtual_addr!");
            placeholder_start = left_it->lower();
            VirtualFreeEx(process, reinterpret_cast<LPVOID>(placeholder_start),
                          placeholder_end - placeholder_start,
                          MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
        }

        // Check if a placeholder exists right after us.
        const auto right_it = placeholders.find(placeholder_end + 1);
        if (right_it != placeholders.end()) {
            ASSERT_MSG(right_it->lower() == placeholder_end,
                       "Right placeholder does not start at virtual_end!");
            placeholder_end = right_it->upper();
            VirtualFreeEx(process, reinterpret_cast<LPVOID>(placeholder_start),
                          placeholder_end - placeholder_start,
                          MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
        }

        // Insert the new placeholder.
        placeholders.insert({placeholder_start, placeholder_end});
    }

    void UnmapPrivate(VAddr virtual_addr, size_t size) {
        const bool ret =
            VirtualFreeEx(process, reinterpret_cast<LPVOID>(virtual_addr), 0, MEM_RELEASE);
        ASSERT_MSG(ret, "{}", Common::GetLastErrorMsg());
    }

    void Protect(VAddr virtual_addr, size_t size, bool read, bool write, bool execute) {
        DWORD new_flags{};
        if (read && write) {
            new_flags = PAGE_READWRITE;
        } else if (read && !write) {
            new_flags = PAGE_READONLY;
        } else if (!read && !write) {
            new_flags = PAGE_NOACCESS;
        } else {
            UNIMPLEMENTED_MSG("Protection flag combination read={} write={}", read, write);
        }

        const VAddr virtual_end = virtual_addr + size;
        auto [it, end] = placeholders.equal_range({virtual_addr, virtual_end});
        while (it != end) {
            const size_t offset = std::max(it->lower(), virtual_addr);
            const size_t protect_length = std::min(it->upper(), virtual_end) - offset;
            DWORD old_flags{};
            if (!VirtualProtect(virtual_base + offset, protect_length, new_flags, &old_flags)) {
                LOG_CRITICAL(Common_Memory, "Failed to change virtual memory protect rules");
            }
            ++it;
        }
    }

    HANDLE process{};
    HANDLE backing_handle{};
    u8* backing_base{};
    u8* virtual_base{};
    boost::icl::separate_interval_set<uintptr_t> placeholders;
};
#else

enum PosixPageProtection {
    PAGE_NOACCESS = 0,
    PAGE_READONLY = PROT_READ,
    PAGE_READWRITE = PROT_READ | PROT_WRITE,
    PAGE_EXECUTE = PROT_EXEC,
    PAGE_EXECUTE_READ = PROT_EXEC | PROT_READ,
    PAGE_EXECUTE_READWRITE = PROT_EXEC | PROT_READ | PROT_WRITE
};

struct AddressSpace::Impl {
    Impl() {
        UNREACHABLE();
    }

    void* MapUser(VAddr virtual_addr, PAddr phys_addr, size_t size, PosixPageProtection prot) {
        UNREACHABLE();
        return nullptr;
    }

    void* MapPrivate(VAddr virtual_addr, size_t size, u64 alignment, PosixPageProtection prot) {
        UNREACHABLE();
        return nullptr;
    }

    void UnmapUser(VAddr virtual_addr, size_t size) {
        UNREACHABLE();
    }

    void UnmapPrivate(VAddr virtual_addr, size_t size) {
        UNREACHABLE();
    }

    void Protect(VAddr virtual_addr, size_t size, bool read, bool write, bool execute) {
        UNREACHABLE();
    }

    u8* backing_base{};
    u8* virtual_base{};
};
#endif

AddressSpace::AddressSpace() : impl{std::make_unique<Impl>()} {
    virtual_base = impl->virtual_base;
    backing_base = impl->backing_base;
}

AddressSpace::~AddressSpace() = default;

void* AddressSpace::Map(VAddr virtual_addr, size_t size, u64 alignment, PAddr phys_addr) {
    if (virtual_addr >= USER_MIN) {
        return impl->MapUser(virtual_addr, phys_addr, size, PAGE_READWRITE);
    }
    return impl->MapPrivate(virtual_addr, size, alignment, PAGE_READWRITE);
}

void AddressSpace::Unmap(VAddr virtual_addr, size_t size) {
    if (virtual_addr >= USER_MIN) {
        return impl->UnmapUser(virtual_addr, size);
    }
    return impl->UnmapPrivate(virtual_addr, size);
}

void AddressSpace::Protect(VAddr virtual_addr, size_t size, MemoryPermission perms) {
    return impl->Protect(virtual_addr, size, true, true, true);
}

} // namespace Core
