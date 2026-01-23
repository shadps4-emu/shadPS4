// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/elf_info.h"
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
asm(".zerofill USER_AREA,USER_AREA,__USER_AREA,0x5F9000000000");
#endif

namespace Core {

// Constants used for mapping address space.
constexpr VAddr SYSTEM_MANAGED_MIN = 0x400000ULL;
constexpr VAddr SYSTEM_MANAGED_MAX = 0x7FFFFBFFFULL;
constexpr VAddr SYSTEM_RESERVED_MIN = 0x7FFFFC000ULL;
#if defined(__APPLE__) && defined(ARCH_X86_64)
// Commpage ranges from 0xFC0000000 - 0xFFFFFFFFF, so decrease the system reserved maximum.
constexpr VAddr SYSTEM_RESERVED_MAX = 0xFBFFFFFFFULL;
// GPU-reserved memory ranges from 0x1000000000 - 0x6FFFFFFFFF, so increase the user minimum.
constexpr VAddr USER_MIN = 0x7000000000ULL;
#else
constexpr VAddr SYSTEM_RESERVED_MAX = 0xFFFFFFFFFULL;
constexpr VAddr USER_MIN = 0x1000000000ULL;
#endif
#if defined(__linux__)
// Linux maps the shadPS4 executable around here, so limit the user maximum
constexpr VAddr USER_MAX = 0x54FFFFFFFFFFULL;
#else
constexpr VAddr USER_MAX = 0x5FFFFFFFFFFFULL;
#endif

// Constants for the sizes of the ranges in address space.
static constexpr u64 SystemManagedSize = SYSTEM_MANAGED_MAX - SYSTEM_MANAGED_MIN + 1;
static constexpr u64 SystemReservedSize = SYSTEM_RESERVED_MAX - SYSTEM_RESERVED_MIN + 1;
static constexpr u64 UserSize = USER_MAX - USER_MIN + 1;

// Required backing file size for mapping physical address space.
static u64 BackingSize = ORBIS_KERNEL_TOTAL_MEM_DEV_PRO;

#ifdef _WIN32

[[nodiscard]] constexpr u64 ToWindowsProt(Core::MemoryProt prot) {
    const bool read =
        True(prot & Core::MemoryProt::CpuRead) || True(prot & Core::MemoryProt::GpuRead);
    const bool write =
        True(prot & Core::MemoryProt::CpuWrite) || True(prot & Core::MemoryProt::GpuWrite);
    const bool execute = True(prot & Core::MemoryProt::CpuExec);

    if (write && !read) {
        // While write-only CPU mappings aren't possible, write-only GPU mappings are.
        LOG_WARNING(Core, "Converting write-only mapping to read-write");
    }

    // All cases involving execute permissions have separate permissions.
    if (execute) {
        if (write) {
            return PAGE_EXECUTE_READWRITE;
        } else if (read && !write) {
            return PAGE_EXECUTE_READ;
        } else {
            return PAGE_EXECUTE;
        }
    } else {
        if (write) {
            return PAGE_READWRITE;
        } else if (read && !write) {
            return PAGE_READONLY;
        } else {
            return PAGE_NOACCESS;
        }
    }
}

struct MemoryRegion {
    VAddr base;
    PAddr phys_base;
    u64 size;
    u32 prot;
    s32 fd;
    bool is_mapped;
};

struct AddressSpace::Impl {
    Impl() : process{GetCurrentProcess()} {
        // Determine the system's page alignment
        SYSTEM_INFO sys_info{};
        GetSystemInfo(&sys_info);
        u64 alignment = sys_info.dwAllocationGranularity;

        // Older Windows builds have a severe performance issue with VirtualAlloc2.
        // We need to get the host's Windows version, then determine if it needs a workaround.
        auto ntdll_handle = GetModuleHandleW(L"ntdll.dll");
        ASSERT_MSG(ntdll_handle, "Failed to retrieve ntdll handle");

        // Get the RtlGetVersion function
        s64(WINAPI * RtlGetVersion)(LPOSVERSIONINFOW);
        *(FARPROC*)&RtlGetVersion = GetProcAddress(ntdll_handle, "RtlGetVersion");
        ASSERT_MSG(RtlGetVersion, "failed to retrieve function pointer for RtlGetVersion");

        // Call RtlGetVersion
        RTL_OSVERSIONINFOW os_version_info{};
        RtlGetVersion(&os_version_info);

        u64 supported_user_max = USER_MAX;
        // This is the build number for Windows 11 22H2
        static constexpr s32 AffectedBuildNumber = 22621;

        // Higher PS4 firmware versions prevent higher address mappings too.
        s32 sdk_ver = Common::ElfInfo::Instance().CompiledSdkVer();
        if (os_version_info.dwBuildNumber <= AffectedBuildNumber ||
            sdk_ver >= Common::ElfInfo::FW_30) {
            supported_user_max = 0x10000000000ULL;
            // Only log the message if we're restricting the user max due to operating system.
            // Since higher compiled SDK versions also get reduced max, we don't need to log there.
            if (sdk_ver < Common::ElfInfo::FW_30) {
                LOG_WARNING(
                    Core,
                    "Older Windows version detected, reducing user max to {:#x} to avoid problems",
                    supported_user_max);
            }
        }

        // Determine the free address ranges we can access.
        VAddr next_addr = SYSTEM_MANAGED_MIN;
        MEMORY_BASIC_INFORMATION info{};
        while (next_addr <= supported_user_max) {
            ASSERT_MSG(VirtualQuery(reinterpret_cast<PVOID>(next_addr), &info, sizeof(info)),
                       "Failed to query memory information for address {:#x}", next_addr);

            // Ensure logic uses values aligned to bage boundaries.
            next_addr = reinterpret_cast<VAddr>(info.BaseAddress) + info.RegionSize;
            next_addr = Common::AlignUp(next_addr, alignment);

            // Prevent size from going past supported_user_max
            u64 size = info.RegionSize;
            if (next_addr > supported_user_max) {
                size -= (next_addr - supported_user_max);
            }
            size = Common::AlignDown(size, alignment);

            // Check for free memory areas
            // Restrict region size to avoid overly fragmenting the virtual memory space.
            if (info.State == MEM_FREE && info.RegionSize > 0x1000000) {
                VAddr addr = Common::AlignUp(reinterpret_cast<VAddr>(info.BaseAddress), alignment);
                regions.emplace(addr,
                                MemoryRegion{addr, PAddr(-1), size, PAGE_NOACCESS, -1, false});
            }
        }

        // Reserve all detected free regions.
        for (auto region : regions) {
            auto addr = static_cast<u8*>(VirtualAlloc2(
                process, reinterpret_cast<PVOID>(region.second.base), region.second.size,
                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0));
            // All marked regions should reserve fine since they're free.
            ASSERT_MSG(addr, "Unable to reserve virtual address space: {}",
                       Common::GetLastErrorMsg());
        }

        // Set these constants to ensure code relying on them works.
        // These do not fully encapsulate the state of the address space.
        system_managed_base = reinterpret_cast<u8*>(regions.begin()->first);
        system_managed_size = SystemManagedSize - (regions.begin()->first - SYSTEM_MANAGED_MIN);
        system_reserved_base = reinterpret_cast<u8*>(SYSTEM_RESERVED_MIN);
        system_reserved_size = SystemReservedSize;
        user_base = reinterpret_cast<u8*>(USER_MIN);
        user_size = supported_user_max - USER_MIN - 1;

        // Increase BackingSize to account for config options.
        BackingSize += Config::getExtraDmemInMbytes() * 1_MB;

        // Allocate backing file that represents the total physical memory.
        backing_handle = CreateFileMapping2(INVALID_HANDLE_VALUE, nullptr, FILE_MAP_ALL_ACCESS,
                                            PAGE_EXECUTE_READWRITE, SEC_COMMIT, BackingSize,
                                            nullptr, nullptr, 0);

        ASSERT_MSG(backing_handle, "{}", Common::GetLastErrorMsg());
        // Allocate a virtual memory for the backing file map as placeholder
        backing_base = static_cast<u8*>(VirtualAlloc2(process, nullptr, BackingSize,
                                                      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                      PAGE_NOACCESS, nullptr, 0));
        ASSERT_MSG(backing_base, "{}", Common::GetLastErrorMsg());

        // Map backing placeholder. This will commit the pages
        void* const ret =
            MapViewOfFile3(backing_handle, process, backing_base, 0, BackingSize,
                           MEM_REPLACE_PLACEHOLDER, PAGE_EXECUTE_READWRITE, nullptr, 0);
        ASSERT_MSG(ret == backing_base, "{}", Common::GetLastErrorMsg());
    }

    ~Impl() {
        if (virtual_base) {
            if (!VirtualFree(virtual_base, 0, MEM_RELEASE)) {
                LOG_CRITICAL(Core, "Failed to free virtual memory");
            }
        }
        if (backing_base) {
            if (!UnmapViewOfFile2(process, backing_base, MEM_PRESERVE_PLACEHOLDER)) {
                LOG_CRITICAL(Core, "Failed to unmap backing memory placeholder");
            }
            if (!VirtualFreeEx(process, backing_base, 0, MEM_RELEASE)) {
                LOG_CRITICAL(Core, "Failed to free backing memory");
            }
        }
        if (!CloseHandle(backing_handle)) {
            LOG_CRITICAL(Core, "Failed to free backing memory file handle");
        }
    }

    void* MapRegion(MemoryRegion* region) {
        VAddr virtual_addr = region->base;
        PAddr phys_addr = region->phys_base;
        u64 size = region->size;
        ULONG prot = region->prot;
        s32 fd = region->fd;

        void* ptr = nullptr;
        if (phys_addr != -1) {
            HANDLE backing = fd != -1 ? reinterpret_cast<HANDLE>(fd) : backing_handle;
            if (fd != -1 && prot == PAGE_READONLY) {
                DWORD resultvar;
                ptr = VirtualAlloc2(process, reinterpret_cast<PVOID>(virtual_addr), size,
                                    MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER,
                                    PAGE_READWRITE, nullptr, 0);

                // phys_addr serves as an offset for file mmaps.
                // Create an OVERLAPPED with the offset, then supply that to ReadFile
                OVERLAPPED param{};
                // Offset is the least-significant 32 bits, OffsetHigh is the most-significant.
                param.Offset = phys_addr & 0xffffffffull;
                param.OffsetHigh = (phys_addr & 0xffffffff00000000ull) >> 32;
                bool ret = ReadFile(backing, ptr, size, &resultvar, &param);
                ASSERT_MSG(ret, "ReadFile failed. {}", Common::GetLastErrorMsg());
                ret = VirtualProtect(ptr, size, prot, &resultvar);
                ASSERT_MSG(ret, "VirtualProtect failed. {}", Common::GetLastErrorMsg());
            } else {
                ptr = MapViewOfFile3(backing, process, reinterpret_cast<PVOID>(virtual_addr),
                                     phys_addr, size, MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
                ASSERT_MSG(ptr, "MapViewOfFile3 failed. {}", Common::GetLastErrorMsg());
            }
        } else {
            ptr =
                VirtualAlloc2(process, reinterpret_cast<PVOID>(virtual_addr), size,
                              MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, prot, nullptr, 0);
        }
        ASSERT_MSG(ptr, "{}", Common::GetLastErrorMsg());
        return ptr;
    }

    void UnmapRegion(MemoryRegion* region) {
        VAddr virtual_addr = region->base;
        PAddr phys_base = region->phys_base;
        u64 size = region->size;
        ULONG prot = region->prot;
        s32 fd = region->fd;

        bool ret = false;
        if ((fd != -1 && prot != PAGE_READONLY) || (fd == -1 && phys_base != -1)) {
            ret = UnmapViewOfFile2(process, reinterpret_cast<PVOID>(virtual_addr),
                                   MEM_PRESERVE_PLACEHOLDER);
        } else {
            ret = VirtualFreeEx(process, reinterpret_cast<PVOID>(virtual_addr), size,
                                MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
        }
        ASSERT_MSG(ret, "Unmap on virtual_addr {:#x}, size {:#x} failed: {}", virtual_addr, size,
                   Common::GetLastErrorMsg());
    }

    void SplitRegion(VAddr virtual_addr, u64 size) {
        // First, get the region this range covers
        auto it = std::prev(regions.upper_bound(virtual_addr));

        // All unmapped areas will coalesce, so there should be a region
        // containing the full requested range. If not, then something is mapped here.
        ASSERT_MSG(it->second.base + it->second.size >= virtual_addr + size,
                   "Cannot fit region into one placeholder");

        // If the region is mapped, we need to unmap first before we can modify the placeholders.
        if (it->second.is_mapped) {
            ASSERT_MSG(it->second.phys_base != -1 || !it->second.is_mapped,
                       "Cannot split unbacked mapping");
            UnmapRegion(&it->second);
        }

        // We need to split this region to create a matching placeholder.
        if (it->second.base != virtual_addr) {
            // Requested address is not the start of the containing region,
            // create a new region to represent the memory before the requested range.
            auto& region = it->second;
            u64 base_offset = virtual_addr - region.base;
            u64 next_region_size = region.size - base_offset;
            PAddr next_region_phys_base = -1;
            if (region.is_mapped) {
                next_region_phys_base = region.phys_base + base_offset;
            }
            region.size = base_offset;

            // Use VirtualFreeEx to create the split.
            if (!VirtualFreeEx(process, LPVOID(region.base), region.size,
                               MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
                UNREACHABLE_MSG("Region splitting failed: {}", Common::GetLastErrorMsg());
            }

            // If the mapping was mapped, remap the region.
            if (region.is_mapped) {
                MapRegion(&region);
            }

            // Store a new region matching the removed area.
            it = regions.emplace_hint(std::next(it), virtual_addr,
                                      MemoryRegion(virtual_addr, next_region_phys_base,
                                                   next_region_size, region.prot, region.fd,
                                                   region.is_mapped));
        }

        // At this point, the region's base will match virtual_addr.
        // Now check for a size difference.
        if (it->second.size != size) {
            // The requested size is smaller than the current region placeholder.
            // Update region to match the requested region,
            // then make a new region to represent the remaining space.
            auto& region = it->second;
            VAddr next_region_addr = region.base + size;
            u64 next_region_size = region.size - size;
            PAddr next_region_phys_base = -1;
            if (region.is_mapped) {
                next_region_phys_base = region.phys_base + size;
            }
            region.size = size;

            // Store the new region matching the remaining space
            regions.emplace_hint(std::next(it), next_region_addr,
                                 MemoryRegion(next_region_addr, next_region_phys_base,
                                              next_region_size, region.prot, region.fd,
                                              region.is_mapped));

            // Use VirtualFreeEx to create the split.
            if (!VirtualFreeEx(process, LPVOID(region.base), region.size,
                               MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
                UNREACHABLE_MSG("Region splitting failed: {}", Common::GetLastErrorMsg());
            }

            // If these regions were mapped, then map the unmapped area beyond the requested range.
            if (region.is_mapped) {
                MapRegion(&std::next(it)->second);
            }
        }

        // If the requested region was mapped, remap it.
        if (it->second.is_mapped) {
            MapRegion(&it->second);
        }
    }

    void* Map(VAddr virtual_addr, PAddr phys_addr, u64 size, ULONG prot, s32 fd = -1) {
        // Get a pointer to the region containing virtual_addr
        auto it = std::prev(regions.upper_bound(virtual_addr));

        // If needed, split surrounding regions to create a placeholder
        if (it->first != virtual_addr || it->second.size != size) {
            SplitRegion(virtual_addr, size);
            it = std::prev(regions.upper_bound(virtual_addr));
        }

        // Get the address and region for this range.
        auto& [base, region] = *it;
        ASSERT_MSG(!region.is_mapped, "Cannot overwrite mapped region");

        // Now we have a region matching the requested region, perform the actual mapping.
        region.is_mapped = true;
        region.phys_base = phys_addr;
        region.prot = prot;
        region.fd = fd;
        return MapRegion(&region);
    }

    void CoalesceFreeRegions(VAddr virtual_addr) {
        // First, get the region to update
        auto it = std::prev(regions.upper_bound(virtual_addr));
        ASSERT_MSG(!it->second.is_mapped, "Cannot coalesce mapped regions");

        // Check if there are free placeholders before this area.
        bool can_coalesce = false;
        auto it_prev = it != regions.begin() ? std::prev(it) : regions.end();
        while (it_prev != regions.end() && !it_prev->second.is_mapped) {
            // If there is an earlier region, move our iterator to that and increase size.
            it_prev->second.size = it_prev->second.size + it->second.size;
            regions.erase(it);
            it = it_prev;

            // Mark this region as coalesce-able.
            can_coalesce = true;

            // Get the next previous region.
            it_prev = it != regions.begin() ? std::prev(it) : regions.end();
        }

        // Check if there are free placeholders after this area.
        auto it_next = std::next(it);
        while (it_next != regions.end() && !it_next->second.is_mapped) {
            // If there is a later region, increase our current region's size
            it->second.size = it->second.size + it_next->second.size;
            regions.erase(it_next);

            // Mark this region as coalesce-able.
            can_coalesce = true;

            // Get the next region
            it_next = std::next(it);
        }

        // If there are placeholders to coalesce, then coalesce them.
        if (can_coalesce) {
            if (!VirtualFreeEx(process, LPVOID(it->first), it->second.size,
                               MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) {
                UNREACHABLE_MSG("Region coalescing failed: {}", Common::GetLastErrorMsg());
            }
        }
    }

    void Unmap(VAddr virtual_addr, u64 size) {
        // Loop through all regions in the requested range
        u64 remaining_size = size;
        VAddr current_addr = virtual_addr;
        while (remaining_size > 0) {
            // Get a pointer to the region containing virtual_addr
            auto it = std::prev(regions.upper_bound(current_addr));

            // If necessary, split regions to ensure a valid unmap.
            // To prevent complication, ensure size is within the bounds of the current region.
            u64 base_offset = current_addr - it->second.base;
            u64 size_to_unmap = std::min<u64>(it->second.size - base_offset, remaining_size);
            if (current_addr != it->second.base || size_to_unmap != it->second.size) {
                SplitRegion(current_addr, size_to_unmap);
                it = std::prev(regions.upper_bound(current_addr));
            }

            // Get the address and region corresponding to this range.
            auto& [base, region] = *it;

            // Unmap the region if it was previously mapped
            if (region.is_mapped) {
                UnmapRegion(&region);
            }

            // Update region data
            region.is_mapped = false;
            region.fd = -1;
            region.phys_base = -1;
            region.prot = PAGE_NOACCESS;

            // Update loop variables
            remaining_size -= size_to_unmap;
            current_addr += size_to_unmap;
        }

        // Coalesce any free space produced from these unmaps.
        CoalesceFreeRegions(virtual_addr);
    }

    void Protect(VAddr virtual_addr, u64 size, bool read, bool write, bool execute) {
        DWORD new_flags{};

        if (write && !read) {
            // While write-only CPU protection isn't possible, write-only GPU protection is.
            LOG_WARNING(Core, "Converting write-only protection to read-write");
        }

        // All cases involving execute permissions have separate permissions.
        if (execute) {
            // If there's some form of write protection requested, provide read-write permissions.
            if (write) {
                new_flags = PAGE_EXECUTE_READWRITE;
            } else if (read && !write) {
                new_flags = PAGE_EXECUTE_READ;
            } else {
                new_flags = PAGE_EXECUTE;
            }
        } else {
            if (write) {
                new_flags = PAGE_READWRITE;
            } else if (read && !write) {
                new_flags = PAGE_READONLY;
            } else {
                new_flags = PAGE_NOACCESS;
            }
        }

        // If no flags are assigned, then something's gone wrong.
        if (new_flags == 0) {
            LOG_CRITICAL(Core,
                         "Unsupported protection flag combination for address {:#x}, size {}, "
                         "read={}, write={}, execute={}",
                         virtual_addr, size, read, write, execute);
            return;
        }

        const VAddr virtual_end = virtual_addr + size;
        auto it = --regions.upper_bound(virtual_addr);
        ASSERT_MSG(it != regions.end(), "addr {:#x} out of bounds", virtual_addr);
        for (; it->first < virtual_end; it++) {
            if (!it->second.is_mapped) {
                continue;
            }
            const auto& region = it->second;
            const u64 range_addr = std::max(region.base, virtual_addr);
            const u64 range_size = std::min(region.base + region.size, virtual_end) - range_addr;
            DWORD old_flags{};
            if (!VirtualProtectEx(process, LPVOID(range_addr), range_size, new_flags, &old_flags)) {
                UNREACHABLE_MSG(
                    "Failed to change virtual memory protection for address {:#x}, size {:#x}",
                    range_addr, range_size);
            }
        }
    }

    boost::icl::interval_set<VAddr> GetUsableRegions() {
        boost::icl::interval_set<VAddr> reserved_regions;
        for (auto region : regions) {
            reserved_regions.insert({region.second.base, region.second.base + region.second.size});
        }
        return reserved_regions;
    }

    HANDLE process{};
    HANDLE backing_handle{};
    u8* backing_base{};
    u8* virtual_base{};
    u8* system_managed_base{};
    u64 system_managed_size{};
    u8* system_reserved_base{};
    u64 system_reserved_size{};
    u8* user_base{};
    u64 user_size{};
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
    const bool read =
        True(prot & Core::MemoryProt::CpuRead) || True(prot & Core::MemoryProt::GpuRead);
    const bool write =
        True(prot & Core::MemoryProt::CpuWrite) || True(prot & Core::MemoryProt::GpuWrite);
    const bool execute = True(prot & Core::MemoryProt::CpuExec);

    if (write && !read) {
        // While write-only CPU mappings aren't possible, write-only GPU mappings are.
        LOG_WARNING(Core, "Converting write-only mapping to read-write");
    }

    // All cases involving execute permissions have separate permissions.
    if (execute) {
        if (write) {
            return PAGE_EXECUTE_READWRITE;
        } else if (read && !write) {
            return PAGE_EXECUTE_READ;
        } else {
            return PAGE_EXECUTE;
        }
    } else {
        if (write) {
            return PAGE_READWRITE;
        } else if (read && !write) {
            return PAGE_READONLY;
        } else {
            return PAGE_NOACCESS;
        }
    }
}

struct AddressSpace::Impl {
    Impl() {
        BackingSize += Config::getExtraDmemInMbytes() * 1_MB;
        // Allocate virtual address placeholder for our address space.
        system_managed_size = SystemManagedSize;
        system_reserved_size = SystemReservedSize;
        user_size = UserSize;

        constexpr int protection_flags = PROT_READ | PROT_WRITE;
        constexpr int map_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED;
#if defined(__APPLE__) && defined(ARCH_X86_64)
        // On ARM64 Macs, we run into limitations due to the commpage from 0xFC0000000 - 0xFFFFFFFFF
        // and the GPU carveout region from 0x1000000000 - 0x6FFFFFFFFF. Because this creates gaps
        // in the available virtual memory region, we map memory space using three distinct parts.
        system_managed_base =
            reinterpret_cast<u8*>(mmap(reinterpret_cast<void*>(SYSTEM_MANAGED_MIN),
                                       system_managed_size, protection_flags, map_flags, -1, 0));
        system_reserved_base =
            reinterpret_cast<u8*>(mmap(reinterpret_cast<void*>(SYSTEM_RESERVED_MIN),
                                       system_reserved_size, protection_flags, map_flags, -1, 0));
        user_base = reinterpret_cast<u8*>(
            mmap(reinterpret_cast<void*>(USER_MIN), user_size, protection_flags, map_flags, -1, 0));
#else
        const auto virtual_size = system_managed_size + system_reserved_size + user_size;
#if defined(ARCH_X86_64)
        const auto virtual_base =
            reinterpret_cast<u8*>(mmap(reinterpret_cast<void*>(SYSTEM_MANAGED_MIN), virtual_size,
                                       protection_flags, map_flags, -1, 0));
        system_managed_base = virtual_base;
        system_reserved_base = reinterpret_cast<u8*>(SYSTEM_RESERVED_MIN);
        user_base = reinterpret_cast<u8*>(USER_MIN);
#else
        // Map memory wherever possible and instruction translation can handle offsetting to the
        // base.
        const auto virtual_base =
            reinterpret_cast<u8*>(mmap(nullptr, virtual_size, protection_flags, map_flags, -1, 0));
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

    void* Map(VAddr virtual_addr, PAddr phys_addr, u64 size, PosixPageProtection prot,
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

    void Unmap(VAddr virtual_addr, u64 size, bool) {
        // Check to see if we are adjacent to any regions.
        VAddr start_address = virtual_addr;
        VAddr end_address = start_address + size;
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

    void Protect(VAddr virtual_addr, u64 size, bool read, bool write, bool execute) {
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
    u64 system_managed_size{};
    u8* system_reserved_base{};
    u64 system_reserved_size{};
    u8* user_base{};
    u64 user_size{};
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

void* AddressSpace::Map(VAddr virtual_addr, u64 size, PAddr phys_addr, bool is_exec) {
#if ARCH_X86_64
    const auto prot = is_exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
#else
    // On non-native architectures, we can simplify things by ignoring the execute flag for the
    // canonical copy of the memory and rely on the JIT to map translated code as executable.
    constexpr auto prot = PAGE_READWRITE;
#endif
    return impl->Map(virtual_addr, phys_addr, size, prot);
}

void* AddressSpace::MapFile(VAddr virtual_addr, u64 size, u64 offset, u32 prot, uintptr_t fd) {
#ifdef _WIN32
    return impl->Map(virtual_addr, offset, size,
                     ToWindowsProt(std::bit_cast<Core::MemoryProt>(prot)), fd);
#else
    return impl->Map(virtual_addr, offset, size, ToPosixProt(std::bit_cast<Core::MemoryProt>(prot)),
                     fd);
#endif
}

void AddressSpace::Unmap(VAddr virtual_addr, u64 size, bool has_backing) {
#ifdef _WIN32
    impl->Unmap(virtual_addr, size);
#else
    impl->Unmap(virtual_addr, size, has_backing);
#endif
}

void AddressSpace::Protect(VAddr virtual_addr, u64 size, MemoryPermission perms) {
    const bool read = True(perms & MemoryPermission::Read);
    const bool write = True(perms & MemoryPermission::Write);
    const bool execute = True(perms & MemoryPermission::Execute);
    return impl->Protect(virtual_addr, size, read, write, execute);
}

boost::icl::interval_set<VAddr> AddressSpace::GetUsableRegions() {
#ifdef _WIN32
    // On Windows, we need to obtain the accessible intervals from the implementation's regions.
    return impl->GetUsableRegions();
#else
    // On Linux and Mac, the memory space is fully represented by the three major regions
    boost::icl::interval_set<VAddr> reserved_regions;
    VAddr system_managed_addr = reinterpret_cast<VAddr>(system_managed_base);
    VAddr system_reserved_addr = reinterpret_cast<VAddr>(system_reserved_base);
    VAddr user_addr = reinterpret_cast<VAddr>(user_base);

    reserved_regions.insert({system_managed_addr, system_managed_addr + system_managed_size});
    reserved_regions.insert({system_reserved_addr, system_reserved_addr + system_reserved_size});
    reserved_regions.insert({user_addr, user_addr + user_size});
    return reserved_regions;
#endif
}

} // namespace Core
