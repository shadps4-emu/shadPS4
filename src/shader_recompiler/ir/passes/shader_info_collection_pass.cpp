// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/program.h"
#include "video_core/buffer_cache/buffer_cache.h"

namespace Shader::Optimization {

void Visit(Info& info, const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetAttribute:
    case IR::Opcode::GetAttributeU32:
        info.loads.Set(inst.Arg(0).Attribute(), inst.Arg(1).U32());
        break;
    case IR::Opcode::SetAttribute:
        info.stores.Set(inst.Arg(0).Attribute(), inst.Arg(2).U32());
        break;
    case IR::Opcode::GetUserData:
        info.ud_mask.Set(inst.Arg(0).ScalarReg());
        break;
    case IR::Opcode::SetPatch: {
        const auto patch = inst.Arg(0).Patch();
        if (patch <= IR::Patch::TessellationLodBottom) {
            info.stores_tess_level_outer = true;
        } else if (patch <= IR::Patch::TessellationLodInteriorV) {
            info.stores_tess_level_inner = true;
        } else {
            info.uses_patches |= 1U << IR::GenericPatchIndex(patch);
        }
        break;
    }
    case IR::Opcode::GetPatch: {
        const auto patch = inst.Arg(0).Patch();
        info.uses_patches |= 1U << IR::GenericPatchIndex(patch);
        break;
    }
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::WriteSharedU16:
        info.shared_types |= IR::Type::U16;
        break;
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::SharedAtomicIAdd32:
    case IR::Opcode::SharedAtomicISub32:
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicUMin32:
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicUMax32:
    case IR::Opcode::SharedAtomicInc32:
    case IR::Opcode::SharedAtomicDec32:
    case IR::Opcode::SharedAtomicAnd32:
    case IR::Opcode::SharedAtomicOr32:
    case IR::Opcode::SharedAtomicXor32:
        info.shared_types |= IR::Type::U32;
        break;
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU64:
    case IR::Opcode::SharedAtomicIAdd64:
        info.shared_types |= IR::Type::U64;
        break;
    case IR::Opcode::ConvertF16F32:
    case IR::Opcode::ConvertF32F16:
    case IR::Opcode::BitCastF16U16:
        info.uses_fp16 = true;
        break;
    case IR::Opcode::PackDouble2x32:
    case IR::Opcode::UnpackDouble2x32:
        info.uses_fp64 = true;
        break;
    case IR::Opcode::ImageWrite:
        info.has_storage_images = true;
        break;
    case IR::Opcode::QuadShuffle:
        info.uses_group_quad = true;
        break;
    case IR::Opcode::ReadLane:
    case IR::Opcode::ReadFirstLane:
    case IR::Opcode::WriteLane:
        info.uses_group_ballot = true;
        break;
    case IR::Opcode::Discard:
    case IR::Opcode::DiscardCond:
        info.has_discard = true;
        break;
    case IR::Opcode::ImageGather:
    case IR::Opcode::ImageGatherDref:
        info.has_image_gather = true;
        break;
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageQueryLod:
        info.has_image_query = true;
        break;
    case IR::Opcode::ImageAtomicFMax32:
    case IR::Opcode::ImageAtomicFMin32:
        info.uses_atomic_float_min_max = true;
        break;
    case IR::Opcode::LaneId:
        info.uses_lane_id = true;
        break;
    case IR::Opcode::ReadConst:
        if (info.readconst_types == Info::ReadConstType::None) {
            info.buffers.push_back({
                .used_types = IR::Type::U32,
                // We can't guarantee that flatbuf will not grow past UBO
                // limit if there are a lot of ReadConsts. (We could specialize)
                .inline_cbuf = AmdGpu::Buffer::Placeholder(std::numeric_limits<u32>::max()),
                .buffer_type = BufferType::Flatbuf,
            });
        }
        if (inst.Flags<u32>() != 0) {
            info.readconst_types |= Info::ReadConstType::Immediate;
        } else {
            info.readconst_types |= Info::ReadConstType::Dynamic;
        }
        info.dma_types |= IR::Type::U32;
        break;
    case IR::Opcode::PackUfloat10_11_11:
        info.uses_pack_10_11_11 = true;
        break;
    case IR::Opcode::UnpackUfloat10_11_11:
        info.uses_unpack_10_11_11 = true;
        break;
    default:
        break;
    }
}

void CollectShaderInfoPass(IR::Program& program) {
    for (IR::Block* const block : program.post_order_blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Visit(program.info, inst);
        }
    }

    if (program.info.dma_types != IR::Type::Void) {
        program.info.buffers.push_back({
            .used_types = IR::Type::U64,
            .inline_cbuf = AmdGpu::Buffer::Placeholder(VideoCore::BufferCache::BDA_PAGETABLE_SIZE),
            .buffer_type = BufferType::BdaPagetable,
            .is_written = true,
        });
        program.info.buffers.push_back({
            .used_types = IR::Type::U8,
            .inline_cbuf = AmdGpu::Buffer::Placeholder(VideoCore::BufferCache::FAULT_BUFFER_SIZE),
            .buffer_type = BufferType::FaultBuffer,
            .is_written = true,
        });
    }
}

} // namespace Shader::Optimization
