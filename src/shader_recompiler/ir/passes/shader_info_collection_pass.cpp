// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void Visit(Info& info, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetAttribute:
    case IR::Opcode::GetAttributeU32: {
        info.loads.Set(inst.Arg(0).Attribute(), inst.Arg(1).U32());
        break;
    }
    case IR::Opcode::SetAttribute: {
        info.stores.Set(inst.Arg(0).Attribute(), inst.Arg(2).U32());
        break;
    }
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::WriteSharedU64:
        info.uses_shared = true;
        break;
    case IR::Opcode::ConvertF16F32:
    case IR::Opcode::ConvertF32F16:
    case IR::Opcode::BitCastF16U16:
        info.uses_fp16 = true;
        break;
    case IR::Opcode::BitCastU64F64:
        info.uses_fp64 = true;
        break;
    case IR::Opcode::ImageWrite:
        info.has_storage_images = true;
        break;
    case IR::Opcode::LoadBufferFormatF32:
        info.has_texel_buffers = true;
        break;
    case IR::Opcode::StoreBufferFormatF32:
        info.has_image_buffers = true;
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
    case IR::Opcode::LaneId:
        info.uses_lane_id = true;
        break;
    default:
        break;
    }
}

void CollectShaderInfoPass(IR::Program& program) {
    Info& info{program.info};
    for (IR::Block* const block : program.post_order_blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Visit(info, inst);
        }
    }
}

} // namespace Shader::Optimization
