// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include "common/enum.h"
#include "common/types.h"
#include "core/memory/budget.h"
#include "core/memory/splay_tree.h"
#include "core/memory/sx_lock.h"
#include "core/memory/vm_object.h"

namespace Vulkan {
class Rasterizer;
}

namespace Core {

enum class MemoryProt : u8 {
    NoAccess = 0,
    CpuRead = 1,
    CpuWrite = 2,
    CpuReadWrite = 3,
    CpuExec = 4,
    CpuAll = 7,
    GpuRead = 16,
    GpuWrite = 32,
    GpuReadWrite = 48,
    All = 0x37,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryProt)

enum class MemoryMapFlags : u32 {
    NoFlags = 0,
    Shared = 1,
    Private = 2,
    Fixed = 0x10,
    NoOverwrite = 0x80,
    Void = 0x100,
    Stack = 0x400,
    NoSync = 0x800,
    Anon = 0x1000,
    System = 0x2000,
    NoCore = 0x20000,
    Align2MB = 0x100000,
    Sanitizer = 0x200000,
    NoCoalesce = 0x400000,
    WriteWbGarlic = 0x800000,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryMapFlags)

enum class VmInherit : u8 {
    Share = 0,
    Copy = 1,
    None = 2,
};

enum class VmMapWireFlags : u32 {
    System = 0,
    User = 1,
    NoHoles = 0,
    HolesOk = 2,
    Write = 4,
};
DECLARE_ENUM_FLAG_OPERATORS(VmMapWireFlags)

enum class VmEntryFlags : u32 {
    None = 0,
    CopyOnWrite = 1,
    NeedsCopy = 2,
    NoFault = 4,
    NoSync = 0x20,
    NoCoalesce = 0x200,
    NoCoredump = 0x400,
    InTransition = 0x800,
    InTransition2 = 0x1000,
    NeedsWakeup = 0x2000,
    IsSubMap = 0x4000,
    Dmem2 = 0x80000,
    WireBudget = 0x100000,
    InBudget = 0x200000,
    Kernel = 0x80000000,
};
DECLARE_ENUM_FLAG_OPERATORS(VmEntryFlags)

enum class VmExtFlags : u32 {
    None = 0,
    Hide = 1,
    GpuOnly = 2,
    Kernel = 4,
    Page2MB = 8,
};
DECLARE_ENUM_FLAG_OPERATORS(VmExtFlags)

struct VmMapEntry : SplayTreeNode {
    VAddr start{};
    VAddr end{};
    std::shared_ptr<VmObject> object{};
    u64 offset{};
    MemoryProt protection{MemoryProt::NoAccess};
    MemoryProt max_protection{MemoryProt::NoAccess};
    VmEntryFlags eflags{VmEntryFlags::None};
    VmExtFlags ext_flags{VmExtFlags::None};
    VmInherit inheritance{VmInherit::Copy};
    BudgetPtype budget_ptype{BudgetPtype::BigApp};
    s32 wired_count{};
    u64 obj_entry_id{};
    char name[32]{};

    u64 Size() const {
        return end - start;
    }

    bool IsFree() const {
        return max_protection == MemoryProt::NoAccess && object == nullptr &&
               protection == MemoryProt::NoAccess;
    }

    bool IsBlockpool() const {
        return False(eflags & VmEntryFlags::IsSubMap) && object && object->IsBlockpool();
    }

    bool IsDmem() const {
        return object && object->IsDmem();
    }

    bool IsDmem2() const {
        return True(eflags & VmEntryFlags::Dmem2);
    }

    bool IsGpuMapping() const {
        constexpr VAddr max_gpu_address{0x10000000000};
        return True(protection & MemoryProt::GpuReadWrite) && end < max_gpu_address;
    }
};

struct VmMapSplayTraits {
    using EntryType = VmMapEntry;

    static SplayTreeNode* GetNode(VmMapEntry* entry) {
        return static_cast<SplayTreeNode*>(entry);
    }
    static VmMapEntry* FromNode(SplayTreeNode* node) {
        return static_cast<VmMapEntry*>(node);
    }
    static u64 GetStart(const VmMapEntry* entry) {
        return entry->start;
    }
    static u64 GetEnd(const VmMapEntry* entry) {
        return entry->end;
    }
};

class Blockpool;
class DmemManager;
class FlexibleMemoryPool;
enum class DmemMemoryType : s32;

class AddressSpace;

class VmMap {
public:
    using Tree = SplayTree<VmMapSplayTraits>;

    explicit VmMap(AddressSpace& impl_, DmemManager& dmem_, Blockpool& blockpool_,
                   BudgetState& budget_, FlexibleMemoryPool& flex_pool_)
        : impl{impl_}, dmem{dmem_}, blockpool{blockpool_}, budget{budget_}, flex_pool{flex_pool_} {}
    ~VmMap() = default;

    VmMap(const VmMap&) = delete;
    VmMap& operator=(const VmMap&) = delete;

    void SetRasterizer(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
    }

    void Init(VAddr min_offset, VAddr max_offset, u32 sdk_version);

    Tree::iterator Insert(Tree::iterator prev, VAddr start, VAddr end,
                          std::shared_ptr<VmObject> object, u64 offset, MemoryProt prot,
                          MemoryProt max_prot, VmEntryFlags eflags, std::string_view name);

    Tree::iterator ClipStart(Tree::iterator entry, VAddr addr);

    Tree::iterator ClipEnd(Tree::iterator entry, VAddr addr);

    s32 MapMemory(VAddr* out_addr, u64 size, MemoryProt prot, MemoryProt max_prot,
                  MemoryMapFlags flags, std::shared_ptr<VmObject> object, u64 offset,
                  std::string_view name);

    s32 FindAddress(VAddr* addr, u64 size, MemoryMapFlags flags, VmObject* object);

    s32 Delete(VAddr start, VAddr end);

    s32 Protect(DmemManager& dmem, VAddr start, VAddr end, MemoryProt new_prot, bool set_max);

    s32 ProtectType(DmemManager& dmem, VAddr start, VAddr end, DmemMemoryType mtype,
                    MemoryProt new_prot);

    s32 Wire(VAddr start, VAddr end, VmMapWireFlags flags);

    void UnwireRange(Tree::iterator first_entry, VAddr upto_addr, VmMapWireFlags flags);

    void SimplifyEntry(Tree::iterator entry);

    void NameRange(VAddr start, VAddr end, std::string_view name);

    auto FindSpace(VAddr start, u64 length) {
        return m_tree.FindSpace(start, length);
    }

    auto LookupEntry(VAddr addr) {
        return m_tree.LookupEntry(addr);
    }

    auto LookupEntryReadOnly(VAddr addr) {
        return m_tree.LookupEntryReadOnly(addr);
    }

    bool IsEmpty() const {
        return m_tree.IsEmpty();
    }
    u32 Count() const {
        return m_tree.Count();
    }
    u64 Size() const {
        return m_size;
    }
    VAddr MinOffset() const {
        return m_min_offset;
    }
    VAddr MaxOffset() const {
        return m_max_offset;
    }

    Tree& GetTree() {
        return m_tree;
    }

    std::shared_mutex lock;

private:
    void EnsureObject(VmMapEntry* entry) {
        ASSERT(entry->object);
        if (entry->object) {
            return;
        }
        entry->object = std::make_shared<VmObject>();
        entry->object->type = VmObjectType::Default;
        entry->offset = 0;
    }

    void BlockpoolNameSplit(VAddr start, VAddr end);

    void BlockpoolSetName(VAddr start, VAddr end, const char* name);

    AddressSpace& impl;
    DmemManager& dmem;
    Vulkan::Rasterizer* rasterizer;
    Blockpool& blockpool;
    BudgetState& budget;
    FlexibleMemoryPool& flex_pool;
    Tree m_tree;
    VmMapEntry m_header;
    VAddr m_min_offset{};
    VAddr m_max_offset{};
    u64 m_size{};
    u64 m_obj_entry_count{};
    u32 m_sdk_version;
    bool m_wire_on_map{true};
};

} // namespace Core