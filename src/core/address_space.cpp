// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <boost/icl/separate_interval_set.hpp>
#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/error.h"
#include "core/address_space.h"
#include "core/libraries/kernel/memory.h"
#include "core/memory.h"
#include "libraries/error_codes.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#endif

#if defined(__APPLE__) && defined(ARCH_X86_64)
// Reserve space for the system address space using a zerofill section.
asm(".zerofill SYSTEM_MANAGED,SYSTEM_MANAGED,__SYSTEM_MANAGED,0x7FFBFC000");
asm(".zerofill SYSTEM_RESERVED,SYSTEM_RESERVED,__SYSTEM_RESERVED,0x7C0004000");
#endif

namespace Core {

static constexpr size_t BackingSize = SCE_KERNEL_TOTAL_MEM_PRO;

#ifdef _WIN32

[[nodiscard]] constexpr u64 ToWindowsProt(Core::MemoryProt prot) {
    if (True(prot & Core::MemoryProt::CpuReadWrite) ||
        True(prot & Core::MemoryProt::GpuReadWrite)) {
        return PAGE_READWRITE;
    } else if (True(prot & Core::MemoryProt::CpuRead) || True(prot & Core::MemoryProt::GpuRead)) {
        return PAGE_READONLY;
    } else {
        return PAGE_NOACCESS;
    }
}

struct MemoryRegion {
    VAddr base;
    size_t size;
    bool is_mapped;
};

struct AddressSpace::Impl {
    Impl() : process{GetCurrentProcess()} {
        // Allocate virtual address placeholder for our address space.
        MEM_ADDRESS_REQUIREMENTS req{};
        MEM_EXTENDED_PARAMETER param{};
        req.LowestStartingAddress = reinterpret_cast<PVOID>(SYSTEM_MANAGED_MIN);
        // The ending address must align to page boundary - 1
        // https://stackoverflow.com/questions/54223343/virtualalloc2-with-memextendedparameteraddressrequirements-always-produces-error
        req.HighestEndingAddress = reinterpret_cast<PVOID>(USER_MIN + UserSize - 1);
        req.Alignment = 0;
        param.Type = MemExtendedParameterAddressRequirements;
        param.Pointer = &req;

        // Typically, lower parts of system managed area is already reserved in windows.
        // If reservation fails attempt again by reducing the area size a little bit.
        // System managed is about 31GB in size so also cap the number of times we can reduce it
        // to a reasonable amount.
        static constexpr size_t ReductionOnFail = 1_GB;
        static constexpr size_t MaxReductions = 10;

        size_t virtual_size = SystemManagedSize + SystemReservedSize + UserSize;
        for (u32 i = 0; i < MaxReductions; i++) {
            virtual_base = static_cast<u8*>(VirtualAlloc2(process, NULL, virtual_size,
                                                          MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                          PAGE_NOACCESS, &param, 1));
            if (virtual_base) {
                break;
            }
            virtual_size -= ReductionOnFail;
        }
        ASSERT_MSG(virtual_base, "Unable to reserve virtual address space: {}",
                   Common::GetLastErrorMsg());

        system_reserved_base = reinterpret_cast<u8*>(SYSTEM_RESERVED_MIN);
        system_reserved_size = SystemReservedSize;
        system_managed_base = virtual_base;
        system_managed_size = system_reserved_base - virtual_base;
        user_base = reinterpret_cast<u8*>(USER_MIN);
        user_size = virtual_base + virtual_size - user_base;

        LOG_INFO(Kernel_Vmm, "System managed virtual memory region: {} - {}",
                 fmt::ptr(system_managed_base),
                 fmt::ptr(system_managed_base + system_managed_size - 1));
        LOG_INFO(Kernel_Vmm, "System reserved virtual memory region: {} - {}",
                 fmt::ptr(system_reserved_base),
                 fmt::ptr(system_reserved_base + system_reserved_size - 1));
        LOG_INFO(Kernel_Vmm, "User virtual memory region: {} - {}", fmt::ptr(user_base),
                 fmt::ptr(user_base + user_size - 1));

        // Initializer placeholder tracker
        const uintptr_t system_managed_addr = reinterpret_cast<uintptr_t>(system_managed_base);
        regions.emplace(system_managed_addr,
                        MemoryRegion{system_managed_addr, virtual_size, false});

        // Allocate backing file that represents the total physical memory.
        backing_handle =
            CreateFileMapping2(INVALID_HANDLE_VALUE, nullptr, FILE_MAP_WRITE | FILE_MAP_READ,
                               PAGE_READWRITE, SEC_COMMIT, BackingSize, nullptr, nullptr, 0);
        ASSERT_MSG(backing_handle, "{}", Common::GetLastErrorMsg());
        // Allocate a virtual memory for the backing file map as placeholder
        backing_base = static_cast<u8*>(VirtualAlloc2(process, nullptr, BackingSize,
                                                      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                      PAGE_NOACCESS, nullptr, 0));
        // Map backing placeholder. This will commit the pages
        void* const ret = MapViewOfFile3(backing_handle, process, backing_base, 0, BackingSize,
                                         MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
        ASSERT_MSG(ret == backing_base, "{}", Common::GetLastErrorMsg());
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

    void* Map(VAddr virtual_addr, PAddr phys_addr, size_t size, ULONG prot, uintptr_t fd = 0) {
        // Before mapping we must carve a placeholder with the exact properties of our mapping.
        auto* region = EnsureSplitRegionForMapping(virtual_addr, size);
        region->is_mapped = true;
        void* ptr = nullptr;
        if (phys_addr != -1) {
            HANDLE backing = fd ? reinterpret_cast<HANDLE>(fd) : backing_handle;
            if (fd && prot == PAGE_READONLY) {
                DWORD resultvar;
                ptr = VirtualAlloc2(process, reinterpret_cast<PVOID>(virtual_addr), size,
                                    MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER,
                                    PAGE_READWRITE, nullptr, 0);
                bool ret = ReadFile(backing, ptr, size, &resultvar, NULL);
                ASSERT_MSG(ret, "ReadFile failed. {}", Common::GetLastErrorMsg());
                ret = VirtualProtect(ptr, size, prot, &resultvar);
                ASSERT_MSG(ret, "VirtualProtect failed. {}", Common::GetLastErrorMsg());
            } else {
                ptr = MapViewOfFile3(backing, process, reinterpret_cast<PVOID>(virtual_addr),
                                     phys_addr, size, MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
            }
        } else {
            ptr =
                VirtualAlloc2(process, reinterpret_cast<PVOID>(virtual_addr), size,
                              MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
        }
        ASSERT_MSG(ptr, "{}", Common::GetLastErrorMsg());
        return ptr;
    }

    void Unmap(VAddr virtual_addr, size_t size, bool has_backing) {
        bool ret;
        if (has_backing) {
            ret = UnmapViewOfFile2(process, reinterpret_cast<PVOID>(virtual_addr),
                                   MEM_PRESERVE_PLACEHOLDER);
        } else {
            ret = VirtualFreeEx(process, reinterpret_cast<PVOID>(virtual_addr), size,
                                MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
        }
        ASSERT_MSG(ret, "Unmap operation on virtual_addr={:#X} failed: {}", virtual_addr,
                   Common::GetLastErrorMsg());

        // The unmap call will create a new placeholder region. We need to see if we can coalesce it
        // with neighbors.
        JoinRegionsAfterUnmap(virtual_addr, size);
    }

    // The following code is inspired from Dolphin's MemArena
    // https://github.com/dolphin-emu/dolphin/blob/deee3ee4/Source/Core/Common/MemArenaWin.cpp#L212
    MemoryRegion* EnsureSplitRegionForMapping(VAddr address, size_t size) {
        // Find closest region that is <= the given address by using upper bound and decrementing
        auto it = regions.upper_bound(address);
        ASSERT_MSG(it != regions.begin(), "Invalid address {:#x}", address);
        --it;
        ASSERT_MSG(!it->second.is_mapped,
                   "Attempt to map {:#x} with size {:#x} which overlaps with {:#x} mapping",
                   address, size, it->second.base);
        auto& [base, region] = *it;

        const VAddr mapping_address = region.base;
        const size_t region_size = region.size;
        if (mapping_address == address) {
            // If this region is already split up correctly we don't have to do anything
            if (region_size == size) {
                return &region;
            }

            ASSERT_MSG(region_size >= size,
                       "Region with address {:#x} and size {:#x} can't fit {:#x}", mapping_address,
                       region_size, size);

            // Split the placeholder.
            if (!VirtualFreeEx(process, LPVOID(address), size,
                               MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
                UNREACHABLE_MSG("Region splitting failed: {}", Common::GetLastErrorMsg());
                return nullptr;
            }

            // Update tracked mappings and return the first of the two
            region.size = size;
            const VAddr new_mapping_start = address + size;
            regions.emplace_hint(std::next(it), new_mapping_start,
                                 MemoryRegion(new_mapping_start, region_size - size, false));
            return &region;
        }

        ASSERT(mapping_address < address);

        // Is there enough space to map this?
        const size_t offset_in_region = address - mapping_address;
        const size_t minimum_size = size + offset_in_region;
        ASSERT(region_size >= minimum_size);

        // Split the placeholder.
        if (!VirtualFreeEx(process, LPVOID(address), size,
                           MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            UNREACHABLE_MSG("Region splitting failed: {}", Common::GetLastErrorMsg());
            return nullptr;
        }

        // Do we now have two regions or three regions?
        if (region_size == minimum_size) {
            // Split into two; update tracked mappings and return the second one
            region.size = offset_in_region;
            it = regions.emplace_hint(std::next(it), address, MemoryRegion(address, size, false));
            return &it->second;
        } else {
            // Split into three; update tracked mappings and return the middle one
            region.size = offset_in_region;
            const VAddr middle_mapping_start = address;
            const size_t middle_mapping_size = size;
            const VAddr after_mapping_start = address + size;
            const size_t after_mapping_size = region_size - minimum_size;
            it = regions.emplace_hint(std::next(it), after_mapping_start,
                                      MemoryRegion(after_mapping_start, after_mapping_size, false));
            it = regions.emplace_hint(
                it, middle_mapping_start,
                MemoryRegion(middle_mapping_start, middle_mapping_size, false));
            return &it->second;
        }
    }

    void JoinRegionsAfterUnmap(VAddr address, size_t size) {
        // There should be a mapping that matches the request exactly, find it
        auto it = regions.find(address);
        ASSERT_MSG(it != regions.end() && it->second.size == size,
                   "Invalid address/size given to unmap.");
        auto& [base, region] = *it;
        region.is_mapped = false;

        // Check if a placeholder exists right before us.
        auto it_prev = it != regions.begin() ? std::prev(it) : regions.end();
        if (it_prev != regions.end() && !it_prev->second.is_mapped) {
            const size_t total_size = it_prev->second.size + size;
            if (!VirtualFreeEx(process, LPVOID(it_prev->first), total_size,
                               MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) {
                UNREACHABLE_MSG("Region coalescing failed: {}", Common::GetLastErrorMsg());
            }

            it_prev->second.size = total_size;
            regions.erase(it);
            it = it_prev;
        }

        // Check if a placeholder exists right after us.
        auto it_next = std::next(it);
        if (it_next != regions.end() && !it_next->second.is_mapped) {
            const size_t total_size = it->second.size + it_next->second.size;
            if (!VirtualFreeEx(process, LPVOID(it->first), total_size,
                               MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) {
                UNREACHABLE_MSG("Region coalescing failed: {}", Common::GetLastErrorMsg());
            }

            it->second.size = total_size;
            regions.erase(it_next);
        }
    }

    void Protect(VAddr virtual_addr, size_t size, bool read, bool write, bool execute) {
        DWORD new_flags{};

        if (read && write && execute) {
            new_flags = PAGE_EXECUTE_READWRITE;
        } else if (read && write) {
            new_flags = PAGE_READWRITE;
        } else if (read && !write) {
            new_flags = PAGE_READONLY;
        } else if (execute && !read && !write) {
            new_flags = PAGE_EXECUTE;
        } else if (!read && !write && !execute) {
            new_flags = PAGE_NOACCESS;
        } else {
            LOG_CRITICAL(Common_Memory,
                         "Unsupported protection flag combination for address {:#x}, size {}, "
                         "read={}, write={}, execute={}",
                         virtual_addr, size, read, write, execute);
            return;
        }

        const VAddr virtual_end = virtual_addr + size;
        auto it = --regions.upper_bound(virtual_addr);
        for (; it->first < virtual_end; it++) {
            if (!it->second.is_mapped) {
                continue;
            }
            const auto& region = it->second;
            const size_t range_addr = std::max(region.base, virtual_addr);
            const size_t range_size = std::min(region.base + region.size, virtual_end) - range_addr;
            DWORD old_flags{};
            if (!VirtualProtectEx(process, LPVOID(range_addr), range_size, new_flags, &old_flags)) {
                UNREACHABLE_MSG(
                    "Failed to change virtual memory protection for address {:#x}, size {}",
                    range_addr, range_size);
            }
        }
    }

    HANDLE process{};
    HANDLE backing_handle{};
    u8* backing_base{};
    u8* virtual_base{};
    u8* system_managed_base{};
    size_t system_managed_size{};
    u8* system_reserved_base{};
    size_t system_reserved_size{};
    u8* user_base{};
    size_t user_size{};
    std::map<VAddr, MemoryRegion> regions;
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

[[nodiscard]] constexpr PosixPageProtection ToPosixProt(Core::MemoryProt prot) {
    if (True(prot & Core::MemoryProt::CpuReadWrite) ||
        True(prot & Core::MemoryProt::GpuReadWrite)) {
        return PAGE_READWRITE;
    } else if (True(prot & Core::MemoryProt::CpuRead) || True(prot & Core::MemoryProt::GpuRead)) {
        return PAGE_READONLY;
    } else {
        return PAGE_NOACCESS;
    }
}

struct AddressSpace::Impl {
    Impl() {
        // Allocate virtual address placeholder for our address space.
        system_managed_size = SystemManagedSize;
        system_reserved_size = SystemReservedSize;
        user_size = UserSize;

        constexpr int protection_flags = PROT_READ | PROT_WRITE;
        constexpr int base_map_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
#if defined(__APPLE__) && defined(ARCH_X86_64)
        // On ARM64 Macs under Rosetta 2, we run into limitations due to the commpage from
        // 0xFC0000000 - 0xFFFFFFFFF and the GPU carveout region from 0x1000000000 - 0x6FFFFFFFFF.
        // We can allocate the system managed region, as well as system reserved if reduced in size
        // slightly, but we cannot map the user region where we want, so we must let the OS put it
        // wherever possible and hope the game won't rely on its location.
        system_managed_base = reinterpret_cast<u8*>(
            mmap(reinterpret_cast<void*>(SYSTEM_MANAGED_MIN), system_managed_size, protection_flags,
                 base_map_flags | MAP_FIXED, -1, 0));
        system_reserved_base = reinterpret_cast<u8*>(
            mmap(reinterpret_cast<void*>(SYSTEM_RESERVED_MIN), system_reserved_size,
                 protection_flags, base_map_flags | MAP_FIXED, -1, 0));
        // Cannot guarantee enough space for these areas at the desired addresses, so not MAP_FIXED.
        user_base = reinterpret_cast<u8*>(mmap(reinterpret_cast<void*>(USER_MIN), user_size,
                                               protection_flags, base_map_flags, -1, 0));
#else
        const auto virtual_size = system_managed_size + system_reserved_size + user_size;
#if defined(ARCH_X86_64)
        const auto virtual_base =
            reinterpret_cast<u8*>(mmap(reinterpret_cast<void*>(SYSTEM_MANAGED_MIN), virtual_size,
                                       protection_flags, base_map_flags | MAP_FIXED, -1, 0));
        system_managed_base = virtual_base;
        system_reserved_base = reinterpret_cast<u8*>(SYSTEM_RESERVED_MIN);
        user_base = reinterpret_cast<u8*>(USER_MIN);
#else
        // Map memory wherever possible and instruction translation can handle offsetting to the
        // base.
        const auto virtual_base = reinterpret_cast<u8*>(
            mmap(nullptr, virtual_size, protection_flags, base_map_flags, -1, 0));
        system_managed_base = virtual_base;
        system_reserved_base = virtual_base + SYSTEM_RESERVED_MIN - SYSTEM_MANAGED_MIN;
        user_base = virtual_base + USER_MIN - SYSTEM_MANAGED_MIN;
#endif
#endif
        if (system_managed_base == MAP_FAILED || system_reserved_base == MAP_FAILED ||
            user_base == MAP_FAILED) {
            LOG_CRITICAL(Kernel_Vmm, "mmap failed: {}", strerror(errno));
            throw std::bad_alloc{};
        }

        LOG_INFO(Kernel_Vmm, "System managed virtual memory region: {} - {}",
                 fmt::ptr(system_managed_base),
                 fmt::ptr(system_managed_base + system_managed_size - 1));
        LOG_INFO(Kernel_Vmm, "System reserved virtual memory region: {} - {}",
                 fmt::ptr(system_reserved_base),
                 fmt::ptr(system_reserved_base + system_reserved_size - 1));
        LOG_INFO(Kernel_Vmm, "User virtual memory region: {} - {}", fmt::ptr(user_base),
                 fmt::ptr(user_base + user_size - 1));

        const VAddr system_managed_addr = reinterpret_cast<VAddr>(system_managed_base);
        const VAddr system_reserved_addr = reinterpret_cast<VAddr>(system_managed_base);
        const VAddr user_addr = reinterpret_cast<VAddr>(user_base);
        m_free_regions.insert({system_managed_addr, system_managed_addr + system_managed_size});
        m_free_regions.insert({system_reserved_addr, system_reserved_addr + system_reserved_size});
        m_free_regions.insert({user_addr, user_addr + user_size});

#ifdef __APPLE__
        const auto shm_path = fmt::format("/BackingDmem{}", getpid());
        backing_fd = shm_open(shm_path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
        if (backing_fd < 0) {
            LOG_CRITICAL(Kernel_Vmm, "shm_open failed: {}", strerror(errno));
            throw std::bad_alloc{};
        }
        shm_unlink(shm_path.c_str());
#else
        madvise(virtual_base, virtual_size, MADV_HUGEPAGE);

        backing_fd = memfd_create("BackingDmem", 0);
        if (backing_fd < 0) {
            LOG_CRITICAL(Kernel_Vmm, "memfd_create failed: {}", strerror(errno));
            throw std::bad_alloc{};
        }
#endif

        // Defined to extend the file with zeros
        int ret = ftruncate(backing_fd, BackingSize);
        if (ret != 0) {
            LOG_CRITICAL(Kernel_Vmm, "ftruncate failed with {}, are you out-of-memory?",
                         strerror(errno));
            throw std::bad_alloc{};
        }

        // Map backing dmem handle.
        backing_base = static_cast<u8*>(
            mmap(nullptr, BackingSize, PROT_READ | PROT_WRITE, MAP_SHARED, backing_fd, 0));
        if (backing_base == MAP_FAILED) {
            LOG_CRITICAL(Kernel_Vmm, "mmap failed: {}", strerror(errno));
            throw std::bad_alloc{};
        }
    }

    void* Map(VAddr virtual_addr, PAddr phys_addr, size_t size, PosixPageProtection prot,
              int fd = -1) {
        m_free_regions.subtract({virtual_addr, virtual_addr + size});
        const int handle = phys_addr != -1 ? (fd == -1 ? backing_fd : fd) : -1;
        const off_t host_offset = phys_addr != -1 ? phys_addr : 0;
        const int flag = phys_addr != -1 ? MAP_SHARED : (MAP_ANONYMOUS | MAP_PRIVATE);
        void* ret = mmap(reinterpret_cast<void*>(virtual_addr), size, prot, MAP_FIXED | flag,
                         handle, host_offset);
        ASSERT_MSG(ret != MAP_FAILED, "mmap failed: {}", strerror(errno));
        return ret;
    }

    void Unmap(VAddr virtual_addr, size_t size, bool) {
        // Check to see if we are adjacent to any regions.
        auto start_address = virtual_addr;
        auto end_address = start_address + size;
        auto it = m_free_regions.find({start_address - 1, end_address + 1});

        // If we are, join with them, ensuring we stay in bounds.
        if (it != m_free_regions.end()) {
            start_address = std::min(start_address, it->lower());
            end_address = std::max(end_address, it->upper());
        }

        // Free the relevant region.
        m_free_regions.insert({start_address, end_address});

        // Return the adjusted pointers.
        void* ret = mmap(reinterpret_cast<void*>(start_address), end_address - start_address,
                         PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        ASSERT_MSG(ret != MAP_FAILED, "mmap failed: {}", strerror(errno));
    }

    void Protect(VAddr virtual_addr, size_t size, bool read, bool write, bool execute) {
        int flags = PROT_NONE;
        if (read) {
            flags |= PROT_READ;
        }
        if (write) {
            flags |= PROT_WRITE;
        }
#ifdef ARCH_X86_64
        if (execute) {
            flags |= PROT_EXEC;
        }
#endif
        int ret = mprotect(reinterpret_cast<void*>(virtual_addr), size, flags);
        ASSERT_MSG(ret == 0, "mprotect failed: {}", strerror(errno));
    }

    int backing_fd;
    u8* backing_base{};
    u8* system_managed_base{};
    size_t system_managed_size{};
    u8* system_reserved_base{};
    size_t system_reserved_size{};
    u8* user_base{};
    size_t user_size{};
    boost::icl::interval_set<VAddr> m_free_regions;
};
#endif

AddressSpace::AddressSpace() : impl{std::make_unique<Impl>()} {
    backing_base = impl->backing_base;
    system_managed_base = impl->system_managed_base;
    system_managed_size = impl->system_managed_size;
    system_reserved_base = impl->system_reserved_base;
    system_reserved_size = impl->system_reserved_size;
    user_base = impl->user_base;
    user_size = impl->user_size;
}

AddressSpace::~AddressSpace() = default;

void* AddressSpace::Map(VAddr virtual_addr, size_t size, u64 alignment, PAddr phys_addr,
                        bool is_exec) {
#if ARCH_X86_64
    const auto prot = is_exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
#else
    // On non-native architectures, we can simplify things by ignoring the execute flag for the
    // canonical copy of the memory and rely on the JIT to map translated code as executable.
    constexpr auto prot = PAGE_READWRITE;
#endif
    return impl->Map(virtual_addr, phys_addr, size, prot);
}

void* AddressSpace::MapFile(VAddr virtual_addr, size_t size, size_t offset, u32 prot,
                            uintptr_t fd) {
#ifdef _WIN32
    return impl->Map(virtual_addr, offset, size,
                     ToWindowsProt(std::bit_cast<Core::MemoryProt>(prot)), fd);
#else
    return impl->Map(virtual_addr, offset, size, ToPosixProt(std::bit_cast<Core::MemoryProt>(prot)),
                     fd);
#endif
}

void AddressSpace::Unmap(VAddr virtual_addr, size_t size, VAddr start_in_vma, VAddr end_in_vma,
                         PAddr phys_base, bool is_exec, bool has_backing, bool readonly_file) {
#ifdef _WIN32
    // There does not appear to be comparable support for partial unmapping on Windows.
    // Unfortunately, a least one title was found to require this. The workaround is to unmap
    // the entire allocation and remap the portions outside of the requested unmapping range.
    impl->Unmap(virtual_addr, size, has_backing && !readonly_file);

    // TODO: Determine if any titles require partial unmapping support for flexible allocations.
    ASSERT_MSG(has_backing || (start_in_vma == 0 && end_in_vma == size),
               "Partial unmapping of flexible allocations is not supported");

    if (start_in_vma != 0) {
        Map(virtual_addr, start_in_vma, 0, phys_base, is_exec);
    }

    if (end_in_vma != size) {
        Map(virtual_addr + end_in_vma, size - end_in_vma, 0, phys_base + end_in_vma, is_exec);
    }
#else
    impl->Unmap(virtual_addr + start_in_vma, end_in_vma - start_in_vma, has_backing);
#endif
}

void AddressSpace::Protect(VAddr virtual_addr, size_t size, MemoryPermission perms) {
    const bool read = True(perms & MemoryPermission::Read);
    const bool write = True(perms & MemoryPermission::Write);
    const bool execute = True(perms & MemoryPermission::Execute);
    return impl->Protect(virtual_addr, size, read, write, execute);
}

} // namespace Core
