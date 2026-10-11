// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <span>
#include <unordered_map>

#include "common/assert.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/resource.h"

namespace Shader::Optimization {

static bool IsSharpSource(const IR::Inst* inst) {
    return inst->GetOpcode() == IR::Opcode::GetUserData ||
           inst->GetOpcode() == IR::Opcode::ReadConst ||
           inst->GetOpcode() == IR::Opcode::ReadConstBuffer;
}

struct StridePatchResult {
    IR::Value vsharp_dw1;
    u32 dw1_mask;
    bool found;
};
StridePatchResult CheckStridePatchPattern(IR::Value value) {
    // s_or_b32        s37, s101, 0x400000
    // s_mov_b32       s36, s100
    // s_mov_b32       s38, -1
    // s_mov_b32       s39, 0x2000c004
    // s_movk_i32      s0, 0x6d0
    // buffer_load_dwordx4 v[17:20], v59, s[36:39], s0 idxen

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, 0, false};
    }
    if (inst->GetOpcode() != IR::Opcode::BitwiseOr32 || !inst->Arg(1).IsImmediate()) {
        return {value, 0, false};
    }
    return {inst->Arg(0), inst->Arg(1).U32(), true};
}

struct ClearStrideAndPatchRecordsResult {
    IR::Value vsharp_dw1;
    IR::Value vsharp_dw2;
    u32 records_shift;
    bool found;
};
ClearStrideAndPatchRecordsResult CheckClearStrideAndPatchRecordsPattern(IR::Value dword1,
                                                                        IR::Value dword2) {
    // s_and_b32       s21, s21, 0xffff
    // s_lshl_b32      s22, s22, <imm>
    // s_buffer_load_dwordx4 s[20:23], s[20:23], 0x0

    auto* dw1_inst = dword1.TryInst();
    if (!dw1_inst) {
        return {dword1, dword2, false};
    }
    if (dw1_inst->GetOpcode() != IR::Opcode::BitwiseAnd32 || !dw1_inst->Arg(1).IsImmediate() ||
        dw1_inst->Arg(1).U32() != 0xffffu) {
        return {dword1, dword2, false};
    }
    auto* dw2_inst = dword2.TryInst();
    if (!dw2_inst) {
        return {dword1, dword2, false};
    }
    if (dw2_inst->GetOpcode() != IR::Opcode::ShiftLeftLogical32 ||
        !dw2_inst->Arg(1).IsImmediate()) {
        return {dword1, dword2, false};
    }

    return {dw1_inst->Arg(0), dw2_inst->Arg(0), dw2_inst->Arg(1).U32(), true};
}

struct InlineCbufResult {
    IR::Value vsharp_dw0;
    bool found;
};
InlineCbufResult CheckInlineCbufPattern(IR::Value value) {
    // Assuming V# is in s[32:35]
    // The next pattern:
    //  s_getpc_b64 s[32:33]
    //  s_add_u32   s32, <const>, s32
    //  s_addc_u32  s33, 0, s33
    //  s_mov_b32   s35, <const>
    //  s_movk_i32  s34, <const>
    //  buffer_load_format_xyz v[8:10], v1, s[32:35], 0 ...
    // is used to define an inline constant buffer

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, false};
    }
    if (inst->GetOpcode() != IR::Opcode::IAdd32 ||
        inst->Arg(0).IsImmediate() == inst->Arg(1).IsImmediate()) {
        return {value, false};
    }
    const u32 offset = inst->Arg(0).IsImmediate() ? inst->Arg(0).U32() : inst->Arg(1).U32();
    auto* prod = inst->Arg(0).IsImmediate() ? inst->Arg(1).Inst() : inst->Arg(0).Inst();
    if (prod->GetOpcode() != IR::Opcode::GetPcLo) {
        return {value, false};
    }
    return {IR::Value{prod->Arg(0).U32() + offset}, true};
}

struct CubeTo2DArrayResult {
    IR::Value tsharp_dw3;
    IR::Value tsharp_dw4;
    bool found;
};
CubeTo2DArrayResult CheckCubeTo2DArrayPattern(IR::Value dword3, IR::Value dword4) {
    // Assuming T# is in s[12:19]
    // The next pattern:
    //  s_bfe_u32       vcc_lo, s16, 0xd0000
    //  s_mul_i32       vcc_lo, vcc_lo, 6
    //  s_add_i32       vcc_lo, vcc_lo, 5
    //  s_and_b32       vcc_hi, 0x1fff, vcc_lo
    //  s_and_b32       vcc_lo, s16, 0xffffe000
    //  s_or_b32        s16, vcc_hi, vcc_lo
    //  s_and_b32       vcc_lo, s15, 0xfffffff
    //  s_or_b32        s15, 0xd0000000, vcc_lo
    //  image_load_mip  v[4:7], v[0:3], s[12:19] dmask:15 da
    // is used to convert the resource from cubemap to 2d-array

    // Check pattern that forces type to Color2DArray
    auto* dw3_inst = dword3.TryInst();
    if (!dw3_inst) {
        return {dword3, dword4, false};
    }

    if (dw3_inst->GetOpcode() != IR::Opcode::BitwiseOr32 || dw3_inst->Arg(1).IsImmediate() ||
        !dw3_inst->Arg(0).IsImmediate() || dw3_inst->Arg(0).U32() != 0xd0000000u) {
        return {dword3, dword4, false};
    }

    auto* dw3_and = dw3_inst->Arg(1).Inst();
    if (dw3_and->GetOpcode() != IR::Opcode::BitwiseAnd32 || !dw3_and->Arg(1).IsImmediate() ||
        dw3_and->Arg(1).U32() != 0xfffffffu) {
        return {dword3, dword4, false};
    }

    // Check pattern that translates cubemap depth to slices
    auto* dw4_inst = dword4.TryInst();
    if (!dw4_inst) {
        return {dword3, dword4, false};
    }

    if (dw4_inst->GetOpcode() != IR::Opcode::BitwiseOr32 || dw4_inst->Arg(0).IsImmediate() ||
        dw4_inst->Arg(1).IsImmediate()) {
        return {dword3, dword4, false};
    }

    auto* dw4_rhs = dw4_inst->Arg(1).Inst();
    if (dw4_rhs->GetOpcode() != IR::Opcode::BitwiseAnd32 || !dw4_rhs->Arg(1).IsImmediate() ||
        dw4_rhs->Arg(1).U32() != 0xffffe000u) {
        return {dword3, dword4, false};
    }

    return {dw3_and->Arg(0), dw4_rhs->Arg(0), true};
}

struct AnisoLod0Result {
    IR::Value ssharp_dw0;
    IR::Value tsharp_dw3;
    bool found;
};
AnisoLod0Result CheckDisableAnisoLod0Pattern(IR::Value value) {
    // Assuming S# is in s[12:15] and T# is in s[4:11]
    // The next pattern:
    //  s_bfe_u32     s0, s7,  $0x0008000c
    //  s_and_b32     s1, s12, $0xfffff1ff
    //  s_cmp_eq_u32  s0, 0
    //  s_cselect_b32 s0, s1, s12
    //  s_mov_b32     s12, s0
    // is used to disable anisotropy in the sampler if the sampled texture doesn't have mips

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, {}, false};
    }

    if (inst->GetOpcode() != IR::Opcode::SelectU32) {
        return {value, {}, false};
    }

    // Select should be based on zero check
    const auto* prod0 = inst->Arg(0).Inst();
    if (prod0->GetOpcode() != IR::Opcode::IEqual32 ||
        !(prod0->Arg(1).IsImmediate() && prod0->Arg(1).U32() == 0u)) {
        return {value, {}, false};
    }

    auto* prod0_arg0 = prod0->Arg(0).Inst();
    ASSERT(prod0_arg0->GetOpcode() != IR::Opcode::Phi);

    // The bits range is for lods (note that constants are changed after constant propagation pass)
    if (prod0_arg0->GetOpcode() != IR::Opcode::BitFieldUExtract ||
        !(prod0_arg0->Arg(1).IsImmediate() && prod0_arg0->Arg(1).U32() == 12) ||
        !(prod0_arg0->Arg(2).IsImmediate() && prod0_arg0->Arg(2).U32() == 8)) {
        return {value, {}, false};
    }

    // Make sure mask is masking out anisotropy
    const auto* prod1 = inst->Arg(1).Inst();
    if (prod1->GetOpcode() != IR::Opcode::BitwiseAnd32 || prod1->Arg(1).U32() != 0xfffff1ff) {
        return {value, {}, false};
    }

    // We're working on the first dword of S#
    auto* prod2 = inst->Arg(2).Inst();
    ASSERT(prod2->GetOpcode() != IR::Opcode::Phi);
    return {inst->Arg(2), prod0_arg0->Arg(0), true};
}

struct SamplerPatchResult {
    IR::Value ssharp_dw0;
    bool found;
};
SamplerPatchResult CheckForceClampToWrapPattern(IR::Value value) {
    // s_and_b32 s12, 0xfffffe00, s12
    // is used to force clamp_x/y/z to wrap

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, false};
    }

    if (inst->GetOpcode() != IR::Opcode::BitwiseAnd32 || !inst->Arg(0).IsImmediate() ||
        inst->Arg(0).U32() != 0xfffffe00u) {
        return {value, false};
    }

    return {inst->Arg(1), true};
}

SamplerPatchResult CheckForceClampToLastTexelPattern(IR::Value value) {
    // s_and_b32 s12, 0xfffffe00, s12
    // s_or_b32  0x12, s12
    // is used to force clamp_x/y to last texel

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, false};
    }

    if (inst->GetOpcode() != IR::Opcode::BitwiseOr32 || inst->Arg(1).IsImmediate() ||
        !inst->Arg(0).IsImmediate() || inst->Arg(0).U32() != 0x12u) {
        return {value, false};
    }

    auto* prod = inst->Arg(1).Inst();
    if (prod->GetOpcode() != IR::Opcode::BitwiseAnd32 || !prod->Arg(0).IsImmediate() ||
        prod->Arg(0).U32() != 0xfffffe00u) {
        return {value, false};
    }

    return {prod->Arg(1), true};
}

SamplerPatchResult CheckClearAnisoRatioAndThresholdPattern(IR::Value value) {
    // s_and_b32 s12, s12, 0xfff8f1ff
    // is used to clear anisotropy ratio and threshold fields

    auto* inst = value.TryInst();
    if (!inst) {
        return {value, false};
    }

    if (inst->GetOpcode() != IR::Opcode::BitwiseAnd32 || !inst->Arg(1).IsImmediate() ||
        inst->Arg(1).U32() != 0xfff8f1ffu) {
        return {value, false};
    }

    return {inst->Arg(0), true};
}

IR::Inst* FindSharpSource(IR::Inst* handle) {
    ASSERT(IsSharpSource(handle));
    return handle;
}

void MarkReadConstBufferSharpSources(const SharpReference& sharp) {
    // In cases of bindless sharp fetches mark all producer instructions
    // so the extended userdata flattening pass will include them.
    for (size_t i = 0; i < sharp.num_dwords; i++) {
        IR::Inst* source = sharp.dwords[i].TryInst();
        if (!source) {
            continue;
        }
        ASSERT(IsSharpSource(source));
        if (source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
            auto flags = source->Flags<IR::BufferInstInfo>();
            flags.sharp_source.Assign(1u);
            source->SetFlags(flags);
        }
    }
}

void DiscoverBufferSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& resources) {
    IR::Inst* handle = inst.Arg(0).Inst();
    auto& resource = resources.emplace_back(&inst);
    auto& vsharp = resource.sharps[0];

    // Gather V# dwords
    vsharp.num_dwords = handle->NumArgs();
    for (size_t i = 0; i < handle->NumArgs(); ++i) {
        vsharp.dwords[i] = handle->Arg(i);
        if (auto* inst = vsharp.dwords[i].TryInst();
            inst && inst->GetOpcode() == IR::Opcode::ReadFirstLane) {
            vsharp.dwords[i] = inst->Arg(0);
        }
    }

    // Attempt to "see through" various V# access patterns and have binding reproduce them
    if (auto [dword0, found] = CheckInlineCbufPattern(vsharp.dwords[0]); found) {
        vsharp.post_op = SharpFetchPostOp::OffsetByProgramBase;
        vsharp.dwords[0] = dword0;
        vsharp.dwords[1] = IR::Value{0U};
    } else if (auto [dword1, mask, found] = CheckStridePatchPattern(handle->Arg(1)); found) {
        vsharp.post_op = SharpFetchPostOp::BitwiseOrDw1WithImm;
        vsharp.post_op_data.dw1_mask = mask;
        vsharp.dwords[1] = dword1;
    } else if (auto [dword1, dword2, shift, found] =
                   CheckClearStrideAndPatchRecordsPattern(handle->Arg(1), handle->Arg(2));
               found) {
        vsharp.post_op = SharpFetchPostOp::ClearStrideAndBitshiftDw2WithImm;
        vsharp.post_op_data.dw2_shift = shift;
        vsharp.dwords[1] = dword1;
        vsharp.dwords[2] = dword2;
    }

    MarkReadConstBufferSharpSources(vsharp);
}

void DiscoverImageSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& resources) {
    IR::Inst* image_handle = inst.Arg(0).Inst();
    ASSERT(image_handle->GetOpcode() == IR::Opcode::ImageHandle);
    auto& resource = resources.emplace_back(&inst);
    auto& tsharp = resource.sharps[0];

    // Gather T# dwords
    IR::Inst* tsharp_low = image_handle->Arg(0).Inst();
    for (size_t i = 0; i < tsharp_low->NumArgs(); ++i) {
        tsharp.dwords[tsharp.num_dwords++] = tsharp_low->Arg(i);
    }
    if (auto tsharp_high = image_handle->Arg(1).TryInst()) {
        for (size_t i = 0; i < tsharp_high->NumArgs(); ++i) {
            tsharp.dwords[tsharp.num_dwords++] = tsharp_high->Arg(i);
        }
    }

    if (auto [tsharp_dw3, tsharp_dw4, found] =
            CheckCubeTo2DArrayPattern(tsharp.dwords[3], tsharp.dwords[4]);
        found) {
        tsharp.post_op = SharpFetchPostOp::ConvertCubeTo2DArray;
        tsharp.dwords[3] = tsharp_dw3;
        tsharp.dwords[4] = tsharp_dw4;
    }

    MarkReadConstBufferSharpSources(tsharp);

    if (inst.GetOpcode() != IR::Opcode::ImageSampleRaw) {
        return;
    }

    // Gather S# dwords
    const IR::Inst* sampler = inst.Arg(1).Inst();
    auto& ssharp = resource.sharps[1];
    for (size_t i = 0; i < sampler->NumArgs(); ++i) {
        ssharp.dwords[ssharp.num_dwords++] = sampler->Arg(i);
    }
    if (auto [ssharp_dw0, tsharp_dw3, found] = CheckDisableAnisoLod0Pattern(ssharp.dwords[0]);
        found) {
        ssharp.post_op = SharpFetchPostOp::DisableAnisoIfSingleLod;
        ssharp.post_op_data.lod_prod = tsharp_dw3;
        ssharp.dwords[0] = ssharp_dw0;
    } else if (auto [ssharp_dw0, found] = CheckForceClampToWrapPattern(ssharp.dwords[0]); found) {
        ssharp.post_op = SharpFetchPostOp::ForceRepeatXyzClamp;
        ssharp.dwords[0] = ssharp_dw0;
    } else if (auto [ssharp_dw0, found] = CheckForceClampToLastTexelPattern(ssharp.dwords[0]);
               found) {
        ssharp.post_op = SharpFetchPostOp::ForceLastTexelXyClamp;
        ssharp.dwords[0] = ssharp_dw0;
    } else if (auto [ssharp_dw0, found] = CheckClearAnisoRatioAndThresholdPattern(ssharp.dwords[0]);
               found) {
        ssharp.post_op = SharpFetchPostOp::ClearAnisoRatioAndThreshold;
        ssharp.dwords[0] = ssharp_dw0;
    }

    MarkReadConstBufferSharpSources(ssharp);
}

static bool IsSelectableBufferLoad(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::LoadBufferF32:
    case IR::Opcode::LoadBufferF32x2:
    case IR::Opcode::LoadBufferF32x3:
    case IR::Opcode::LoadBufferF32x4:
    case IR::Opcode::LoadBufferFormatF32:
        return true;
    default:
        return false;
    }
}

static bool IsEntryValue(const IR::Value& value) {
    return value.IsImmediate() || value.Inst()->GetParent()->ImmPredecessors().empty();
}

static bool CanRebuildSharpValue(const IR::Value& value, u32 depth) {
    if (IsEntryValue(value)) {
        return true;
    }
    const IR::Inst* const inst = value.Inst();
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::ReadConst:
    case IR::Opcode::CompositeConstructU32x2:
        break;
    default:
        return false;
    }
    if (depth == 0) {
        return false;
    }
    for (size_t i = 0; i < inst->NumArgs(); ++i) {
        if (!CanRebuildSharpValue(inst->Arg(i), depth - 1)) {
            return false;
        }
    }
    return true;
}

static IR::Value RebuildSharpValue(IR::Block& block, IR::Block::iterator insert_point,
                                   const IR::Value& value,
                                   std::unordered_map<const IR::Inst*, IR::Value>& rebuilt) {
    if (IsEntryValue(value)) {
        return value;
    }
    const IR::Inst* const inst = value.Inst();
    if (const auto it = rebuilt.find(inst); it != rebuilt.end()) {
        return it->second;
    }
    boost::container::small_vector<IR::Value, 2> args;
    for (size_t i = 0; i < inst->NumArgs(); ++i) {
        args.push_back(RebuildSharpValue(block, insert_point, inst->Arg(i), rebuilt));
    }
    IR::Inst* const copy = &*block.PrependNewInst(insert_point, *inst);
    for (size_t i = 0; i < args.size(); ++i) {
        copy->SetArg(i, args[i]);
    }
    return rebuilt.emplace(inst, IR::Value{copy}).first->second;
}

static IR::Value SelectSideResult(IR::IREmitter& ir, const IR::U1& condition,
                                  const IR::Value& true_value, const IR::Value& false_value) {
    size_t num_components = 0;
    switch (true_value.Type()) {
    case IR::Type::U32x2:
    case IR::Type::F32x2:
        num_components = 2;
        break;
    case IR::Type::U32x3:
    case IR::Type::F32x3:
        num_components = 3;
        break;
    case IR::Type::U32x4:
    case IR::Type::F32x4:
        num_components = 4;
        break;
    default:
        return ir.Select(condition, true_value, false_value);
    }
    boost::container::small_vector<IR::Value, 4> components;
    for (size_t i = 0; i < num_components; ++i) {
        components.push_back(ir.Select(condition, ir.CompositeExtract(true_value, i),
                                       ir.CompositeExtract(false_value, i)));
    }
    return ir.CompositeConstruct(std::span<const IR::Value>{components.data(), components.size()});
}

static void SplitBufferLoadBySharpPhi(IR::Block& block, IR::Inst& inst) {
    IR::Inst* const handle = inst.Arg(0).TryInst();
    if (!handle || handle->GetOpcode() != IR::Opcode::CompositeConstructU32x4) {
        return;
    }
    std::array<IR::Value, 4> dwords;
    std::array<bool, 4> from_phi{};
    const IR::Inst* first_phi{};
    for (size_t i = 0; i < dwords.size(); ++i) {
        dwords[i] = handle->Arg(i);
        if (const IR::Inst* const rfl = dwords[i].TryInst();
            rfl && rfl->GetOpcode() == IR::Opcode::ReadFirstLane) {
            dwords[i] = rfl->Arg(0);
        }
        const IR::Inst* const source = dwords[i].TryInst();
        if (!source || IsSharpSource(source)) {
            continue;
        }
        if (source->GetOpcode() != IR::Opcode::Phi) {
            return;
        }
        if (!first_phi) {
            first_phi = source;
        } else if (source->GetParent() != first_phi->GetParent() ||
                   source->NumArgs() != first_phi->NumArgs()) {
            return;
        }
        from_phi[i] = true;
    }
    if (!first_phi || first_phi->NumArgs() < 2) {
        return;
    }
    const size_t num_sides = first_phi->NumArgs();
    boost::container::small_vector<std::array<IR::Value, 4>, 4> sides(num_sides);
    for (size_t side = 0; side < num_sides; ++side) {
        IR::Block* const pred = first_phi->PhiBlock(side);
        for (size_t other = 0; other < side; ++other) {
            if (first_phi->PhiBlock(other) == pred) {
                return;
            }
        }
        for (size_t i = 0; i < dwords.size(); ++i) {
            if (!from_phi[i]) {
                sides[side][i] = dwords[i];
                continue;
            }
            const IR::Inst* const phi = dwords[i].Inst();
            size_t arg = 0;
            while (arg < phi->NumArgs() && phi->PhiBlock(arg) != pred) {
                ++arg;
            }
            if (arg == phi->NumArgs()) {
                return;
            }
            const IR::Value value = phi->Arg(arg);
            const IR::Inst* const side_source = value.TryInst();
            if ((side_source && !IsSharpSource(side_source)) || !CanRebuildSharpValue(value, 4)) {
                return;
            }
            sides[side][i] = value;
        }
    }
    IR::Block* const phi_block = first_phi->GetParent();
    IR::Inst* const side_index{&*phi_block->PrependNewInst(phi_block->begin(), IR::Opcode::Phi)};
    side_index->SetFlags(IR::Type::U32);
    for (size_t side = 0; side < num_sides; ++side) {
        side_index->AddPhiOperand(first_phi->PhiBlock(side), IR::Value{static_cast<u32>(side)});
    }
    const auto insert_point = IR::Block::InstructionList::s_iterator_to(inst);
    std::unordered_map<const IR::Inst*, IR::Value> rebuilt;
    boost::container::small_vector<IR::Value, 4> loads;
    for (size_t side = 0; side < num_sides; ++side) {
        std::array<IR::Value, 4> values;
        for (size_t i = 0; i < dwords.size(); ++i) {
            values[i] = from_phi[i]
                            ? RebuildSharpValue(block, insert_point, sides[side][i], rebuilt)
                            : sides[side][i];
        }
        IR::Inst* const side_handle = &*block.PrependNewInst(insert_point, *handle);
        for (size_t i = 0; i < values.size(); ++i) {
            side_handle->SetArg(i, values[i]);
        }
        IR::Inst* const load = &*block.PrependNewInst(insert_point, inst);
        load->SetArg(0, IR::Value{side_handle});
        loads.push_back(IR::Value{load});
    }
    IR::IREmitter ir{block, insert_point};
    IR::Value result = loads.back();
    for (size_t side = num_sides - 1; side-- > 0;) {
        const IR::U1 taken = ir.IEqual(IR::U32{side_index}, ir.Imm32(static_cast<u32>(side)));
        result = SelectSideResult(ir, taken, loads[side], result);
    }
    inst.ReplaceUsesWithAndRemove(result);
}

ResourceDiscoveryList ResourceDiscoverPass(IR::Program& program, const Profile& profile) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsSelectableBufferLoad(inst)) {
                SplitBufferLoadBySharpPhi(*block, inst);
            }
        }
    }
    ResourceDiscoveryList sharp_usages;

    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferInstruction(inst)) {
                DiscoverBufferSharp(*block, inst, sharp_usages);
            } else if (IsImageInstruction(inst)) {
                DiscoverImageSharp(*block, inst, sharp_usages);
            } else if (IsDataRingInstruction(inst)) {
                sharp_usages.emplace_back(&inst);
            }
        }
    }
    return sharp_usages;
}

} // namespace Shader::Optimization