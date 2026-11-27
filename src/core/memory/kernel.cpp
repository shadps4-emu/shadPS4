// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/elf_info.h"
#include "core/emulator_settings.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/memory/kernel.h"
#include "core/tls.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Core {

static s32 ResourceShortageError(u32 sdk_version) {
    return sdk_version >= Common::ElfInfo::FW_350 ? ORBIS_KERNEL_ERROR_ENOMEM
                                                  : ORBIS_KERNEL_ERROR_EINVAL;
}

MemoryManager::MemoryManager()
    : blockpool{impl}, dmem{impl}, vm_map{impl, dmem, blockpool, budget, flex_pool} {
    ASSERT_MSG(Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_version) == 0,
               "Failed to get compiled SDK version");

    // Construct vma_map using the regions reserved by the address space
    const auto regions = impl.GetUsableRegions();
    const VAddr min_offset = regions.begin()->lower();
    const VAddr max_offset = regions.rbegin()->upper();
    vm_map.Init(min_offset, max_offset, sdk_version);
    flex_pool.Init(ORBIS_KERNEL_TOTAL_MEM, 8_GB - ORBIS_KERNEL_TOTAL_MEM);
    budget.mlock_limit.fill(ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE);
}

MemoryManager::~MemoryManager() = default;

void MemoryManager::SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1,
                                       bool use_extended_mem2) {
    // Calculate actual direct and flexible memory sizes
    const bool is_neo = ::Libraries::Kernel::sceKernelIsNeoMode();
    auto total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_PRO : ORBIS_KERNEL_TOTAL_MEM;
    if (EmulatorSettings.IsDevKit()) {
        total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_DEV_PRO : ORBIS_KERNEL_TOTAL_MEM_DEV;
    }
    s32 extra_dmem = EmulatorSettings.GetExtraDmemInMBytes();
    if (extra_dmem != 0) {
        LOG_WARNING(Kernel_Vmm,
                    "extraDmemInMbytes is {} MB! Old Direct Size: {:#x} -> New Direct Size: {:#x}",
                    extra_dmem, total_size, total_size + extra_dmem * 1_MB);
        total_size += extra_dmem * 1_MB;
    }
    s32 extra_fmem = EmulatorSettings.GetExtraFmemInMBytes();
    if (extra_fmem != 0) {
        LOG_WARNING(Kernel_Vmm, "extraFmemInMbytes is {} MB! Old Size: {:#x} -> New Size: {:#x}",
                    extra_fmem, ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE,
                    ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE + extra_fmem * 1_MB);
        total_size += extra_fmem * 1_MB;
        flexible_size += extra_fmem * 1_MB;
    }
    if (!use_extended_mem1 && is_neo) {
        total_size -= 256_MB;
    }
    if (!use_extended_mem2 && !is_neo) {
        total_size -= 128_MB;
    }

    // Update stored totals
    total_flexible_size = flexible_size - ORBIS_KERNEL_FLEXIBLE_MEMORY_BASE;
    total_direct_size = total_size - flexible_size;

    // Initialize dmem and flexible allocators to match limits
    dmem.Init(total_direct_size);
    budget.mlock_limit.fill(total_flexible_size);

    LOG_INFO(Kernel_Vmm, "Configured memory regions: flexible size = {:#x}, direct size = {:#x}",
             total_flexible_size, total_direct_size);
}

void MemoryManager::SetPrtArea(u32 id, VAddr address, u64 size) {
    PrtArea& area = prt_areas[id];
    if (area.mapped) {
        rasterizer->UnmapMemory(area.start, area.end - area.start);
    }

    area.start = address;
    area.end = address + size;
    area.mapped = true;

    // Pretend the entire PRT area is mapped to avoid GPU tracking errors.
    // The caches will use CopySparseMemory to fetch data which avoids unmapped areas.
    rasterizer->MapMemory(address, size);
}

void MemoryManager::CopySparseMemory(VAddr virtual_addr, u8* dest, u64 size) {
    const auto& backing_pages = impl.BackingPages();
    while (size) {
        const u64 page = virtual_addr >> GUEST_PAGE_BITS;
        const u64 offset_in_page = virtual_addr % GUEST_PAGE_SIZE;
        const u64 copy_size = std::min<u64>(GUEST_PAGE_SIZE - offset_in_page, size);
        if (auto* entry = backing_pages.find(page); entry && *entry) {
            std::memcpy(dest, *entry + offset_in_page, copy_size);
        } else {
            std::memset(dest, 0, copy_size);
        }
        size -= copy_size;
        virtual_addr += copy_size;
        dest += copy_size;
    }
}

bool MemoryManager::TryWriteBacking(void* address, const void* data, u64 size) {
    const auto& backing_pages = impl.BackingPages();
    VAddr virtual_addr = std::bit_cast<VAddr>(address);
    const auto* src = static_cast<const u8*>(data);
    bool written = false;
    while (size) {
        const u64 page = virtual_addr >> GUEST_PAGE_BITS;
        const u64 offset_in_page = virtual_addr % GUEST_PAGE_SIZE;
        const u64 copy_size = std::min<u64>(GUEST_PAGE_SIZE - offset_in_page, size);
        if (auto* entry = backing_pages.find(page); entry && *entry) {
            std::memcpy(*entry + offset_in_page, src, copy_size);
            written = true;
        }
        size -= copy_size;
        virtual_addr += copy_size;
        src += copy_size;
    }
    return written;
}

s32 MemoryManager::MapDirectMemory(VAddr* out_addr, u64 size, DmemMemoryType mtype, MemoryProt prot,
                                   MemoryMapFlags flags, PAddr phys_addr, u32 alignment_shift) {
    return dmem.MapDirectMemory(vm_map, out_addr, size, mtype, prot, flags, phys_addr,
                                alignment_shift);
}

s32 MemoryManager::PoolMap(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype) {
    if (!Common::Is64KBAligned(virtual_addr) || !Common::Is64KBAligned(size)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::shared_lock lk{vm_map.lock};

    auto [entry, found] = vm_map.LookupEntryReadOnly(virtual_addr);
    if (!found || !entry->IsBlockpool() || virtual_addr + size > entry->end) {
        LOG_ERROR(Kernel_Vmm, "Attempting to commit non-pooled memory at {:#x}", virtual_addr);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (blockpool.HasCommittedBlocks(*entry, virtual_addr, size)) {
        return ORBIS_KERNEL_ERROR_EBUSY;
    }

    if (size && !blockpool.Map(*entry, virtual_addr, size, prot, mtype)) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    return ORBIS_OK;
}

s32 MemoryManager::PoolUnmap(VAddr virtual_addr, u64 size) {
    if (!Common::Is64KBAligned(virtual_addr) || !Common::Is64KBAligned(size)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::shared_lock lk{vm_map.lock};

    auto [entry, found] = vm_map.LookupEntryReadOnly(virtual_addr);
    if (!found || !entry->IsBlockpool() || virtual_addr + size > entry->end) {
        LOG_ERROR(Kernel_Vmm, "Attempting to decommit non-pooled memory at {:#x}", virtual_addr);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (size) {
        if (entry->IsGpuMapping()) {
            rasterizer->UnmapMemory(virtual_addr, size);
        }
        blockpool.Unmap(*entry, virtual_addr, size);
    }

    return ORBIS_OK;
}

s32 MemoryManager::MapMemory(VAddr* out_addr, u64 size, MemoryProt prot, MemoryMapFlags flags,
                             s32 fd, u64 offset, std::string_view name) {
    if (True(flags & MemoryMapFlags::Sanitizer) && !EmulatorSettings.IsDevKit()) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (size == 0) {
        return ORBIS_OK;
    }

    if (True(flags & (MemoryMapFlags::Anon | MemoryMapFlags::Void)) && (offset != 0 || fd != -1)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    if (True(flags & MemoryMapFlags::Stack)) {
        if ((prot & (MemoryProt::CpuReadWrite)) != MemoryProt::CpuReadWrite || fd != -1) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        flags |= MemoryMapFlags::Anon;
        offset = 0;
    }

    VAddr addr = *out_addr;
    const u64 page_offset = offset & PAGE_MASK;
    offset -= page_offset;

    size += page_offset;
    size = Common::AlignUpPow2(size, PAGE_SIZE);

    // Resolve the address hint.
    if (True(flags & MemoryMapFlags::Fixed)) {
        addr -= page_offset;
        if ((addr & PAGE_MASK) != 0) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        if (addr < vm_map.MinOffset() || addr + size > vm_map.MaxOffset()) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    } else {
        addr = (addr + PAGE_MASK) & ~PAGE_MASK;
        if (addr == 0) {
            addr = DEFAULT_MAPPING_BASE;
        }
    }

    MemoryProt max_prot = MemoryProt::NoAccess;
    std::shared_ptr<VmObject> object = nullptr;
    std::unique_lock<std::mutex> vnode_lock;

    if (True(flags & MemoryMapFlags::Void)) {
        prot = MemoryProt::NoAccess;
        max_prot = MemoryProt::NoAccess;
    } else if (True(flags & MemoryMapFlags::Anon)) {
        max_prot = MemoryProt::All;
        const BudgetPtype budget_ptype =
            True(flags & (MemoryMapFlags::System | MemoryMapFlags::Sanitizer))
                ? BudgetPtype::System
                : BudgetPtype::BigApp;
        object = std::make_shared<VmObject>();
        object->type = VmObjectType::Default;
        object->budget_ptype = budget_ptype;
        object->size = size;
        if (!flex_pool.Allocate(size, object->anon.backing)) {
            LOG_ERROR(
                Kernel_Vmm,
                "Out of flexible memory, available flexible memory = {:#x} requested size = {:#x}",
                flex_pool.GetAvailableSize(), size);
            return ResourceShortageError(vm_map.SdkVersion());
        }
    } else {
        ASSERT(fd != -1);
        auto* table = Common::Singleton<Core::FileSys::HandleTable>::Instance();
        auto* file = table->GetFile(fd);
        if (!file) {
            return ORBIS_KERNEL_ERROR_EBADF;
        }
        switch (file->type) {
        case Core::FileSys::FileType::Device: {
            max_prot = MemoryProt::All;
            if (s32 ret = file->device->mmap(offset, size, prot, &max_prot, flags, &object)) {
                return ret;
            }
            break;
        }
        case Core::FileSys::FileType::Regular: {
            Core::FileSys::FileStat stat{};
            file->handle->Stat(stat);
            if (offset + size > Common::AlignUpPow2(stat.size, PAGE_SIZE)) {
                return ORBIS_KERNEL_ERROR_EACCES;
            }
            max_prot = MemoryProt::CpuReadWrite | MemoryProt::GpuReadWrite;
            auto* host_file = file->GetHostFile();
            if (True(flags & MemoryMapFlags::Shared) &&
                (!host_file ||
                 (False(host_file->GetAccessMode() & Common::FS::FileAccessMode::Write) &&
                  False(host_file->GetAccessMode() & Common::FS::FileAccessMode::Append)))) {
                // If the file does not have write access, ensure prot does not contain write
                // permissions. On real hardware, shared mappings succeed, but the memory cannot be
                // written to. Private mappings are writable copies.
                max_prot &= ~(MemoryProt::CpuWrite | MemoryProt::GpuWrite);
            }
            // On real hardware, protections outside of max_prot are silently removed.
            prot &= max_prot;
            // Files in system folders don't count towards the budget.
            const bool is_app_file =
                file->m_guest_name.contains("app0/") || file->m_guest_name.contains("download0/");
            vnode_lock = std::unique_lock{vnode_mutex};
            auto& vnode_object = vnode_objects[file->m_host_name.generic_string()];
            object = vnode_object.lock();
            if (!object) {
                object = std::make_shared<VmObject>();
                object->type = VmObjectType::Vnode;
                object->budget_ptype = is_app_file ? BudgetPtype::BigApp : BudgetPtype::Invalid;
                vnode_object = object;
            }
            object->ExtendSize(offset + size);
            object->vnode.file = file;
            break;
        }
        case Core::FileSys::FileType::Blockpool: {
            constexpr u32 BlockpoolMinAlignment = 21;
            const u32 alignment = GetMapAlignment(flags);
            if (alignment == 0) {
                flags |= MapAlignment(BlockpoolMinAlignment);
            } else if (alignment < BlockpoolMinAlignment) {
                return ORBIS_KERNEL_ERROR_EINVAL;
            }
            constexpr auto BlockpoolProt = MemoryProt::CpuReadWrite | MemoryProt::GpuReadWrite;
            if (offset != 0 || page_offset != 0 || !Common::Is2MBAligned(size) ||
                (flags & (MemoryMapFlags::Shared | MemoryMapFlags::Private)) !=
                    MemoryMapFlags::Shared ||
                (prot & BlockpoolProt) != BlockpoolProt) {
                return ORBIS_KERNEL_ERROR_EINVAL;
            }
            if (True(flags & MemoryMapFlags::Fixed) &&
                (!Common::Is2MBAligned(addr) ||
                 (addr < 0xff0000000ULL && addr + size > 0x7f0000000ULL))) {
                return ORBIS_KERNEL_ERROR_EINVAL;
            }
            max_prot = MemoryProt::All;
            object = std::make_shared<VmObject>();
            object->type = VmObjectType::Blockpool;
            object->size = size;
            object->blockpool.blocks.resize(Blockpool::ToBlocks(size));
            break;
        }
        case Core::FileSys::FileType::Directory:
            return ORBIS_KERNEL_ERROR_EINVAL;
        default:
            max_prot = MemoryProt::All;
            break;
        }
    }

    if (s32 ret = vm_map.MapMemory(&addr, size, prot, max_prot, flags, object, offset, name)) {
        if (True(flags & MemoryMapFlags::Anon)) {
            flex_pool.Free(object->anon.backing);
        }
        return ret;
    }

    if (True(flags & MemoryMapFlags::Shared)) {
        vm_map.Inherit(addr, addr + size, VmInherit::Share);
    }

    if (vm_map.IsWireFuture() &&
        False(flags & (MemoryMapFlags::Void | MemoryMapFlags::Sanitizer))) {
        if (s32 ret =
                vm_map.Wire(addr, addr + size, VmMapWireFlags::User | VmMapWireFlags::HolesOk)) {
            UnmapMemory(addr, size);
            return ret == ORBIS_KERNEL_ERROR_EINVAL ? ResourceShortageError(vm_map.SdkVersion())
                                                    : ret;
        }
    }

    if (const VAddr caller = Core::g_guest_caller_address; caller != 0) {
        vm_map.NameRange(addr, addr + size, fmt::format("anon:{:012x}", caller));
    }

    *out_addr = addr + page_offset;
    return ORBIS_OK;
}

s32 MemoryManager::UnmapMemory(VAddr addr, u64 size) {
    std::scoped_lock lk{vm_map.lock};
    size = Common::AlignUpPow2((addr & (PAGE_SIZE - 1)) + size, PAGE_SIZE);
    addr = Common::AlignDownPow2(addr, PAGE_SIZE);
    return vm_map.Delete(addr, addr + size);
}

s32 MemoryManager::QueryProtection(VAddr addr, VAddr* out_start, VAddr* out_end, u32* out_prot) {
    addr = Common::AlignDownPow2(addr, PAGE_SIZE);
    if (addr > vm_map.MaxOffset()) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::shared_lock lk{vm_map.lock};

    auto [entry, found] = vm_map.LookupEntryReadOnly(addr);
    if (!found) {
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    VAddr start, end;
    u32 prot;

    if (!entry->IsBlockpool() || sdk_version < Common::ElfInfo::FW_100) {
        start = entry->start;
        end = entry->end;
        prot = static_cast<u32>(entry->max_protection & entry->protection);
    } else {
        ::Libraries::Kernel::OrbisVirtualQueryInfo info;
        blockpool.Query(addr, entry->start, entry->Size(), entry->object->blockpool.blocks.data(),
                        &info);
        start = info.start;
        end = info.end;
        prot = info.protection;
    }

    if (out_start) {
        *out_start = start;
    }
    if (out_end) {
        *out_end = end;
    }
    if (out_prot) {
        *out_prot = prot;
    }

    return ORBIS_OK;
}

s32 MemoryManager::Protect(VAddr start, u64 size, MemoryProt new_prot) {
    size = Common::AlignUpPow2((start & (PAGE_SIZE - 1)) + size, PAGE_SIZE);
    start = Common::AlignDownPow2(start, PAGE_SIZE);
    s32 result = vm_map.Protect(dmem, start, start + size, new_prot, 0);
    if (result == ORBIS_OK) {
        InvalidateMemory(start, size);
    }
    return result;
}

s32 MemoryManager::ProtectType(VAddr start, u64 size, DmemMemoryType new_mtype,
                               MemoryProt new_prot) {
    if (new_mtype > DmemMemoryType::WbGarlic || new_mtype < DmemMemoryType::WbOnion) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (True(new_prot & ~MemoryProt::All)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const VAddr end = Common::AlignUpPow2(start + size, PAGE_SIZE);
    start = Common::AlignDownPow2(start, PAGE_SIZE);
    s32 result = vm_map.ProtectType(dmem, start, end, new_mtype, new_prot);
    if (result == ORBIS_OK) {
        InvalidateMemory(start, size);
    }
    return result;
}

s32 MemoryManager::VirtualQuery(VAddr addr, s32 flags,
                                ::Libraries::Kernel::OrbisVirtualQueryInfo* info) {
    std::shared_lock lk{vm_map.lock};

    auto [entry, found] = vm_map.LookupEntryReadOnly(addr);
    if (!found) {
        if ((flags & 1) == 0 || ++entry == vm_map.GetTree().end()) {
            return ORBIS_KERNEL_ERROR_EACCES;
        }
        addr = entry->start;
    }

    memset(info, 0, sizeof(*info));
    strncpy(info->name, entry->name, ::Libraries::Kernel::ORBIS_KERNEL_MAXIMUM_NAME_LENGTH);

    if (entry->IsBlockpool()) {
        blockpool.Query(addr, entry->start, entry->Size(), entry->object->blockpool.blocks.data(),
                        info);
        return ORBIS_OK;
    }

    info->start = entry->start;
    info->end = entry->end;
    info->protection = static_cast<s32>(entry->max_protection & entry->protection);
    info->memory_type = ::Libraries::Kernel::ORBIS_KERNEL_WB_ONION;

    if (entry->object) {
        if (entry->IsDmem()) {
            info->is_direct = 1;
            info->is_committed = 1;

            const u64 relofs = entry->offset - entry->start;
            addr = std::max(addr, entry->start);

            s32 memory_type;
            PAddr dmem_start, dmem_end;
            ASSERT(dmem.GetDirectMemoryType(addr + relofs, &memory_type, &dmem_start, &dmem_end) ==
                   0);

            info->start = std::max<u64>(dmem_start - relofs, entry->start);
            info->end = std::min<u64>(dmem_end - relofs, entry->end);
            info->memory_type = memory_type;
            info->offset = info->start + relofs;
            LOG_DEBUG(Kernel_Vmm,
                      "start={:#x}, end={:#x}, offset={:#x}, prot={:#x}, mtype={}, is_flexible={}, "
                      "is_direct={}, is_stack={}, is_pooled={}, is_commited={}, name={}",
                      info->start, info->end, info->offset, info->protection, info->memory_type,
                      u32(info->is_flexible), u32(info->is_direct), u32(info->is_stack),
                      u32(info->is_pooled), u32(info->is_committed), info->name);
            return ORBIS_OK;
        }
        if (!entry->object->IsPageable()) {
            ASSERT(entry->object->type != VmObjectType::PhysShm);
            return ORBIS_OK;
        }
    }

    if (entry->max_protection != MemoryProt::NoAccess) {
        info->is_flexible = 1;
        info->is_committed = entry->wired_count > 0;
        info->is_stack = True(entry->eflags & (VmEntryFlags::GrowsUp | VmEntryFlags::GrowsDown));
    }

    return ORBIS_OK;
}

void MemoryManager::NameVirtualRange(VAddr start, u64 size, std::string_view name) {
    size = Common::AlignUpPow2((start & (PAGE_SIZE - 1)) + size, PAGE_SIZE);
    start = Common::AlignDownPow2(start, PAGE_SIZE);
    return vm_map.NameRange(start, start + size, name);
}

void MemoryManager::InvalidateMemory(const VAddr addr, const u64 size) const {
    if (rasterizer) {
        rasterizer->InvalidateMemory(addr, size);
    }
}

} // namespace Core
