// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/enum.h"
#include "common/types.h"
#include "core/file_sys/fs.h"
#include "core/memory/blockpool.h"
#include "core/memory/budget.h"
#include "core/memory/flexible_pool.h"

namespace Core {

enum class VmObjectType : u32 {
    Default = 0,
    Swap = 1,
    Vnode = 2,
    Device = 3,
    Phys = 4,
    JitShm = 7,
    Self = 8,
    PhysShm = 10,
    Blockpool = 11
};

enum class VmObjectFlags : u16 {
    None = 0,
    OneMapping = 1 << 0,
    Dead = 1 << 1,
    NoSplit = 1 << 2,
    WireBudget = 1 << 7,
    Dmem = 1 << 15,
};
DECLARE_ENUM_FLAG_OPERATORS(VmObjectFlags)

struct VmObject {
    VmObjectType type;
    u64 size;
    VmObjectFlags flags{VmObjectFlags::OneMapping};
    BudgetPtype budget_ptype{BudgetPtype::Invalid};
    struct {
        std::vector<PhysRange> backing;
    } anon;
    struct {
        FileSys::File* file;
    } vnode;
    struct {
        std::vector<DmemBlock> blocks;
    } blockpool;

    bool IsBlockpool() const {
        return type == VmObjectType::Blockpool;
    }

    bool IsDmem() const {
        return type == VmObjectType::Device && True(flags & VmObjectFlags::Dmem);
    }

    void ExtendSize(u64 new_size) {
        size = std::max(size, new_size);
    }

    bool IsPageable() const {
        return type == VmObjectType::Default || type == VmObjectType::Swap ||
               type == VmObjectType::Vnode || type == VmObjectType::JitShm ||
               type == VmObjectType::Self;
    }

    bool ForEachBacking(u64 obj_offset, u64 range_size, auto&& func) const {
        const u64 end = obj_offset + range_size;
        switch (type) {
        case VmObjectType::Device:
        case VmObjectType::Vnode:
            func(obj_offset, range_size);
            return true;
        case VmObjectType::Default: {
            u64 cursor = 0;
            for (const auto& [phys_offset, size] : anon.backing) {
                if (cursor >= end) {
                    break;
                }
                const u64 r_end = cursor + size;
                if (r_end > obj_offset) {
                    const u64 clip_start = std::max(cursor, obj_offset);
                    const u64 clip_end = std::min(r_end, end);
                    func(phys_offset + (clip_start - cursor), clip_end - clip_start);
                }
                cursor = r_end;
            }
            return cursor >= end;
        }
        default:
            UNREACHABLE();
        }
    }
};

} // namespace Core