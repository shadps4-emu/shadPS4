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

    bool HasPhysicalBacking() const {
        return type == VmObjectType::Default || type == VmObjectType::Device ||
               type == VmObjectType::Blockpool;
    }

    bool ForEachBacking(u64 obj_offset, u64 range_size, auto&& func) const {
        const u64 end = obj_offset + range_size;
        switch (type) {
        case VmObjectType::Device:
        case VmObjectType::Vnode:
            func(obj_offset, range_size, 0);
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
                    func(phys_offset + (clip_start - cursor), clip_end - clip_start,
                         clip_start - obj_offset);
                }
                cursor = r_end;
            }
            return cursor >= end;
        }
        case VmObjectType::Blockpool: {
            const auto& blocks = blockpool.blocks;
            bool fully_backed = true;
            u64 offset = obj_offset;
            while (offset < end) {
                const u64 index = offset >> Blockpool::BLOCK_SHIFT;
                if (index >= blocks.size()) {
                    return false;
                }
                const DmemBlock first = blocks[index];
                u64 run_end = std::min<u64>((index + 1) << Blockpool::BLOCK_SHIFT, end);
                if (!first.valid) {
                    fully_backed = false;
                    offset = run_end;
                    continue;
                }
                for (u64 next = index + 1;
                     run_end < end && next < blocks.size() && blocks[next].valid &&
                     blocks[next].block - first.block == next - index;
                     ++next) {
                    run_end = std::min<u64>((next + 1) << Blockpool::BLOCK_SHIFT, end);
                }
                const u64 offset_in_block = offset & (Blockpool::BLOCK_SIZE - 1);
                func((u64(first.block) << Blockpool::BLOCK_SHIFT) + offset_in_block,
                     run_end - offset, offset - obj_offset);
                offset = run_end;
            }
            return fully_backed;
        }
        default:
            UNREACHABLE();
        }
    }
};

} // namespace Core