// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include "common/assert.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/operand_helper.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/pattern_matching.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Optimization {

/**
 * Tessellation shaders pass outputs to the next shader using LDS.
 * The Hull shader stage receives input control points stored in LDS.
 *
 * These passes attempt to resolve LDS accesses to attribute accesses and correctly
 * write to the tessellation factor tables.
 *
 * The LDS layout is:
 * - TCS inputs for patch 0
 * - TCS inputs for patch 1
 * - TCS inputs for patch 2
 * - ...
 * - TCS outputs for patch 0
 * - TCS outputs for patch 1
 * - TCS outputs for patch 2
 * - ...
 * - PatchConst TCS outputs for patch 0
 * - PatchConst TCS outputs for patch 1
 * - PatchConst TCS outputs for patch 2
 *
 *
 * If the Hull stage does not write any new control points the driver will
 * optimize LDS layout so input and output control point spaces overlap.
 * (Passthrough)
 *
 * The gnm driver requires a V# holding special constants to be bound
 * for reads by the shader.
 * The Hull and Domain shaders read values from this buffer which
 * contain size and offset information required to address input, output,
 * or PatchConst attributes within the current patch.
 * See the TessellationDataConstantBuffer struct to see the layout of this V#.
 *
 * Tessellation factors are stored to a special tessellation factor V# that is automatically bound
 * by the driver. This is the input to the fixed function tessellator that actually subdivides the
 * domain. We translate these to writes to SPIR-V builtins for tessellation factors in the Hull
 * shader.
 * The offset into the tess factor buffer determines which factor the shader is writing.
 * Additionally, most hull shaders seem to redundantly write tess factors to PatchConst
 * attributes, even if dead in the domain shader. We just treat these as generic PatchConst writes.
 *
 * LDS reads in the Hull shader can be from input control points, and in the the Domain shader can
 * be hs output control points (output from the perspective of the Hull shader) and patchconst
 * values.
 * LDS stores in the Hull shader can either be output control point writes or per-patch
 * (PatchConst) data writes. The Domain shader exports attributes using EXP instructions, unless its
 * followed by the geometry stage (but we havent seen this yet), so nothing special there.
 * The address calculations can vary significantly and can't be easily pattern matched. We are at
 * the mercy of instruction selection the ps4 compiler wanted to use.
 * Generally though, they could look something like this:
 * Input control point:
 *     addr = PatchIdInVgt * input_cp_stride * #input_cp_per_patch + index * input_cp_stride
 *          + attr# * 16 + component
 * Output control point:
 *     addr = #patches * input_cp_stride * #input_cp_per_patch
 *          + PatchIdInVgt * output_patch_stride + InvocationID * output_cp_stride
            + attr# * 16 + component
 * Per patch output:
 *     addr = #patches * input_cp_stride * #cp_per_input_patch
 *          + #patches * output_patch_stride
 *          + PatchIdInVgt * per_patch_output_stride + attr# * 16 + component
 *
 * output_patch_stride and output_cp_stride are usually compile time constants in the gcn
 *
 * Hull shaders can also read output control points corresponding to other threads.
 * In HLSL style, this should only be possible in the Patch Constant function.
 * TODO we may need to insert additional barriers if sync is free/more permissive
 * on AMD LDS HW

 * They should also be able to read output PatchConst values,
 * although not sure if this happens in practice.
 *
 * To determine which type of attribute (input, output, patchconst) we the check the users of
 * TessConstants V# reads to deduce which type of attribute a given load/store to LDS
 * is touching.
 *
 * In the Hull shader, both the PatchId within the VGT group (PatchIdInVgt) and the output control
 * point id (InvocationId) are packed in VGPR1 by the driver like
 * V1 = InvocationId << 8 | PatchIdInVgt
 * The shader typically uses V_BFE_(U|S)32 to extract them. We use the starting bit_pos to determine
 * which is which.
 *
 * This pass does not attempt to deduce the exact attribute referenced in a LDS load/store.
 * Instead, it feeds the address in the LDS load/store to the get/set Insts we use for TCS in/out's,
 * TES in's, and PatchConst in/out's.
 *
 * TCS/TES Input attributes:
 * We define input attributes using an array in the shader roughly like this:
 * // equivalent GLSL in TCS
 * layout (location = 0) in vec4 in_attrs[][NUM_INPUT_ATTRIBUTES];
 *
 * Here the NUM_INPUT_ATTRIBUTES is derived from the ls_stride member of the TessConstants V#.
 * We take ALIGN_UP(ls_stride, 16) / 16 to get the number of vec4 attributes.
 * For TES, NUM_INPUT_ATTRIBUTES is ALIGN_UP(hs_cp_stride, 16) / 16.
 * The first (outer) dimension is unsized but corresponds to the number of vertices in the hs input
 * patch (for Hull) or the hs output patch (for Domain).
 *
 * For input reads in TCS or TES, we emit SPIR-V like:
 * float value = in_attrs[addr / ls_stride][(addr % ls_stride) >> 4][(addr % ls_stride) >> 2];
 *
 * For output writes, we assume the control point index is InvocationId, since high level languages
 * impose that restriction (although maybe it's technically possible on hardware). So SPIR-V looks
 * like this:
 * layout (location = 0) in vec4 in_attrs[][NUM_OUTPUT_ATTRIBUTES];
 * out_attrs[InvocationId][(addr % hs_cp_stride) >> 4][(addr % hs_cp_stride) >> 2] = value;
 *
 * NUM_OUTPUT_ATTRIBUTES is derived by ALIGN_UP(hs_cp_stride, 16) / 16, so it matches
 * NUM_INPUT_ATTRIBUTES of the TES.
 *
 * Another challenge is the fact that the GCN shader needs to address attributes from LDS as a whole
 * which contains the attributes from many patches. On the other hand, higher level shading
 * languages restrict attribute access to the patch of the current thread, which is naturally a
 * restriction in SPIR-V also.
 * The addresses the ps4 compiler generates for loads/stores and the fact that LDS holds many
 * patches' attributes are just implementation details of the ps4 driver/compiler. To deal with
 * this, we can replace certain TessConstant V# reads with 0, which only contribute to the base
 * address of the current patch's attributes in LDS and not the indexes within the local patch.
 *
 * (A perfect implementation might need emulation of the VGTs in mesh/compute, loading/storing
 * attributes to buffers and not caring about whether they are hs input, hs output, or patchconst
 * attributes)
 *
 */

namespace {

using namespace Shader::Optimiation::PatternMatching;

static void InitTessConstants(IR::ScalarReg sharp_ptr_base, s32 sharp_dword_offset,
                              Shader::Info& info, Shader::RuntimeInfo& runtime_info,
                              TessellationDataConstantBuffer& tess_constants) {
    info.tess_consts_ptr_base = sharp_ptr_base;
    info.tess_consts_dword_offset = sharp_dword_offset;
    info.ReadTessConstantBuffer(tess_constants);
    if (info.l_stage == LogicalStage::TessellationControl) {
        runtime_info.hs_info.InitFromTessConstants(tess_constants);
    } else {
        runtime_info.vs_info.InitFromTessConstants(tess_constants);
    }

    return;
}

struct TessSharpLocation {
    IR::ScalarReg ptr_base;
    u32 dword_off;
};

std::optional<TessSharpLocation> FindTessConstantSharp(IR::Inst* read_const_buffer) {
    IR::Value sharp_ptr_base;
    IR::Value sharp_dword_offset;

    IR::Value rv = IR::Value{read_const_buffer};
    IR::Value handle = read_const_buffer->Arg(0);

    if (M_COMPOSITECONSTRUCTU32X4(M_GETUSERDATA(MatchImm(sharp_dword_offset)), MatchIgnore(),
                                  MatchIgnore(), MatchIgnore())
            .Match(handle)) {
        return TessSharpLocation{.ptr_base = IR::ScalarReg::Max,
                                 .dword_off = static_cast<u32>(sharp_dword_offset.ScalarReg())};
    } else if (M_COMPOSITECONSTRUCTU32X4(
                   M_READCONST(M_COMPOSITECONSTRUCTU32X2(M_GETUSERDATA(MatchImm(sharp_ptr_base)),
                                                         MatchIgnore()),
                               MatchImm(sharp_dword_offset)),
                   MatchIgnore(), MatchIgnore(), MatchIgnore())
                   .Match(handle)) {
        return TessSharpLocation{.ptr_base = sharp_ptr_base.ScalarReg(),
                                 .dword_off = sharp_dword_offset.U32()};
    }
    return {};
}

// Walker that helps deduce what type of attribute a DS instruction is reading
// or writing, which could be an input control point, output control point,
// or per-patch constant (PatchConst).
// For certain ReadConstBuffer instructions using the tess constants V#, we visit the users
// recursively and increment a counter on the Load/WriteShared users.
// Namely NumPatch (from m_hsNumPatch), HsOutputBase (m_hsOutputBase),
// and PatchConstBase (m_patchConstBase).
// In addr calculations, the term NumPatch * ls_stride * #input_cp_in_patch
// is used as an addend to skip the region for input control points, and similarly
// NumPatch * hs_cp_stride * #output_cp_in_patch is used to skip the region
// for output control points.
//
// TODO: this will break if AMD compiler used distributive property like
// TcsNumPatches * (ls_stride * #input_cp_in_patch + hs_cp_stride * #output_cp_in_patch)
//
// Assert if the region is ambiguous due to phi nodes in the addr calculation for a DS instruction,
class TessConstantUseWalker {
public:
    void WalkUsersOfTessConstant(IR::Inst* read_const_buffer, TessConstantAttribute attr) {
        u32 inc;
        switch (attr) {
        case TessConstantAttribute::HsNumPatch:
        case TessConstantAttribute::HsOutputBase:
            inc = 1;
            break;
        case TessConstantAttribute::PatchConstBase:
            inc = 2;
            break;
        default:
            UNREACHABLE();
        }

        for (IR::Use use : read_const_buffer->Uses()) {
            WalkUsersOfTessConstantHelper(use, inc, false);
        }

        ++seq_num;
    }

private:
    struct PhiInfo {
        u32 seq_num;
        u32 unique_edge;
    };

    void WalkUsersOfTessConstantHelper(IR::Use use, u32 inc, bool propagateError) {
        IR::Inst* inst = use.user;

        switch (use.user->GetOpcode()) {
        case IR::Opcode::LoadSharedU32:
        case IR::Opcode::LoadSharedU64:
        case IR::Opcode::WriteSharedU32:
        case IR::Opcode::WriteSharedU64: {
            bool is_addr_operand = use.operand == 0;
            if (is_addr_operand) {
                u32 counter = inst->Flags<u32>();
                inst->SetFlags<u32>(counter + inc);
                ASSERT_MSG(!propagateError, "LDS instruction {} accesses ambiguous attribute type",
                           fmt::ptr(use.user));
                // Stop here
                return;
            }
        }
        case IR::Opcode::Phi: {
            auto it = phi_infos.find(use.user);
            // the point of seq_num is to tell us if we've already traversed this
            // phi on the current walk to handle phi cycles
            if (it == phi_infos.end()) {
                // First time we've encountered this phi
                // Mark the phi as having been traversed originally through this edge
                phi_infos[inst] = {.seq_num = seq_num,
                                   .unique_edge = static_cast<u16>(use.operand)};
            } else if (it->second.seq_num < seq_num) {
                it->second.seq_num = seq_num;
                // For now, assume we are visiting this phi via the same edge
                // as on other walks. If not, some dataflow analysis might be necessary
                if (it->second.unique_edge != use.operand) {
                    propagateError = true;
                }
            } else {
                ASSERT(it->second.seq_num == seq_num);
                // there's a cycle, and we've already been here on this walk
                return;
            }
            break;
        }
        default:
            break;
        }

        for (IR::Use use : inst->Uses()) {
            WalkUsersOfTessConstantHelper(use, inc, propagateError);
        }
    }

    std::unordered_map<const IR::Inst*, PhiInfo> phi_infos;
    u32 seq_num{1u};
};

enum class AttributeRegion : u32 { InputCP, OutputCP, PatchConst };

static AttributeRegion GetAttributeRegionKind(IR::Inst* ring_access, const Shader::Info& info,
                                              const Shader::RuntimeInfo& runtime_info) {
    u32 count = ring_access->Flags<u32>();
    if (count == 0) {
        return AttributeRegion::InputCP;
    } else if (info.l_stage == LogicalStage::TessellationControl &&
               runtime_info.hs_info.IsPassthrough()) {
        ASSERT(count <= 1);
        return AttributeRegion::PatchConst;
    } else {
        ASSERT(count <= 2);
        return AttributeRegion(count);
    }
}

static bool IsDivisibleByStride(IR::Value term, u32 stride) {
    IR::Value a, b;
    if (MatchU32(stride).Match(term)) {
        return true;
    } else if (M_BITFIELDUEXTRACT(MatchValue(a), MatchU32(0), MatchU32(24)).Match(term) ||
               M_BITFIELDSEXTRACT(MatchValue(a), MatchU32(0), MatchU32(24)).Match(term)) {
        return IsDivisibleByStride(a, stride);
    } else if (M_IMUL32(MatchValue(a), MatchValue(b)).Match(term)) {
        return IsDivisibleByStride(a, stride) || IsDivisibleByStride(b, stride);
    }
    return false;
}

// Return true if we can eliminate any addends
static bool TryOptimizeAddendInModulo(IR::Value addend, u32 stride, std::vector<IR::U32>& addends) {
    IR::Value a, b;
    if (M_IADD32(MatchValue(a), MatchValue(b)).Match(addend)) {
        bool ret = false;
        ret = TryOptimizeAddendInModulo(a, stride, addends);
        ret |= TryOptimizeAddendInModulo(b, stride, addends);
        return ret;
    } else if (!IsDivisibleByStride(addend, stride)) {
        addends.push_back(IR::U32{addend});
        return false;
    } else {
        return true;
    }
}

// In calculation (a + b + ...) % stride
// Use this fact
// (a + b) mod N = (a mod N + b mod N) mod N
// If any addend is divisible by stride, then we can replace it with 0 in the attribute
// or component index calculation
static IR::U32 TryOptimizeAddressModulo(IR::U32 addr, u32 stride, IR::IREmitter& ir) {
    std::vector<IR::U32> addends;
    if (TryOptimizeAddendInModulo(addr, stride, addends)) {
        addr = ir.Imm32(0);
        for (auto& addend : addends) {
            addr = ir.IAdd(addr, addend);
        }
    }
    return addr;
}

// TODO: can optimize div in control point index similarly to mod

// Read a TCS input (InputCP region) or TES input (OutputCP region)
static IR::F32 ReadTessControlPointAttribute(IR::U32 addr, const u32 stride, IR::IREmitter& ir,
                                             u32 off_dw, bool is_output_read_in_tcs) {
    if (off_dw > 0) {
        addr = ir.IAdd(addr, ir.Imm32(off_dw));
    }
    const IR::U32 control_point_index = ir.IDiv(addr, ir.Imm32(stride));
    const IR::U32 opt_addr = TryOptimizeAddressModulo(addr, stride, ir);
    const IR::U32 offset = ir.IMod(opt_addr, ir.Imm32(stride));
    const IR::U32 attr_index = ir.ShiftRightLogical(offset, ir.Imm32(4u));
    const IR::U32 comp_index =
        ir.ShiftRightLogical(ir.BitwiseAnd(offset, ir.Imm32(0xFU)), ir.Imm32(2u));
    if (is_output_read_in_tcs) {
        return ir.ReadTcsGenericOuputAttribute(control_point_index, attr_index, comp_index);
    } else {
        return ir.GetTessGenericAttribute(control_point_index, attr_index, comp_index);
    }
}

} // namespace

void HullShaderTransform(IR::Program& program, const RuntimeInfo& runtime_info) {
    const Info& info = program.info;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::StoreBufferU32:
            case IR::Opcode::StoreBufferU32x2:
            case IR::Opcode::StoreBufferU32x3:
            case IR::Opcode::StoreBufferU32x4: {
                IR::Value soffset = IR::GetBufferSOffsetArg(&inst);
                if (!M_GETATTRIBUTEU32(MatchAttribute(IR::Attribute::TessFactorsBufferBase),
                                       MatchIgnore())
                         .Match(soffset)) {
                    break;
                }

                const auto info = inst.Flags<IR::BufferInstInfo>();
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};

                IR::Value voffset;
                bool success =
                    M_COMPOSITECONSTRUCTU32X3(MatchU32(0), MatchImm(voffset), MatchIgnore())
                        .Match(inst.Arg(IR::StoreBufferArgs::Address));
                ASSERT_MSG(success, "unhandled pattern in tess factor store");

                const u32 gcn_factor_idx = (info.inst_offset.Value() + voffset.U32()) >> 2;
                const IR::Value data = inst.Arg(IR::StoreBufferArgs::Data);

                const u32 num_dwords = u32(opcode) - u32(IR::Opcode::StoreBufferU32) + 1;

                const auto GetValue = [&](IR::Value data) -> IR::F32 {
                    if (auto* inst = data.TryInstRecursive();
                        inst && inst->GetOpcode() == IR::Opcode::BitCastU32F32) {
                        return IR::F32{inst->Arg(0)};
                    }
                    return ir.BitCast<IR::F32, IR::U32>(IR::U32{data});
                };

                auto get_factor_attr = [&](u32 gcn_factor_idx) -> IR::Patch {
                    // The hull outputs tess factors in different formats depending on the shader.
                    // For triangle domains, it seems to pack the entries into 4 consecutive floats,
                    // with the 3 edge factors followed by the 1 interior factor.
                    // For quads, it does 4 edge factors then 2 interior.
                    // There is a tess factor stride member of the GNMX hull constants struct in
                    // a hull program shader binary archive, but this doesn't seem to be
                    // communicated to the driver.
                    // The layout seems to be implied by the type of the abstract domain.
                    switch (runtime_info.hs_info.tess_type) {
                    case AmdGpu::TessellationType::Isoline:
                        ASSERT(gcn_factor_idx < 2);
                        return IR::PatchFactor(gcn_factor_idx);
                    case AmdGpu::TessellationType::Triangle:
                        ASSERT(gcn_factor_idx < 4);
                        if (gcn_factor_idx == 3) {
                            return IR::Patch::TessellationLodInteriorU;
                        }
                        return IR::PatchFactor(gcn_factor_idx);
                    case AmdGpu::TessellationType::Quad:
                        ASSERT(gcn_factor_idx < 6);
                        return IR::PatchFactor(gcn_factor_idx);
                    default:
                        UNREACHABLE();
                    }
                };

                inst.Invalidate();
                if (num_dwords == 1) {
                    ir.SetPatch(get_factor_attr(gcn_factor_idx), GetValue(data));
                    break;
                }
                auto* inst = data.TryInstRecursive();
                ASSERT(inst && (inst->GetOpcode() == IR::Opcode::CompositeConstructU32x2 ||
                                inst->GetOpcode() == IR::Opcode::CompositeConstructU32x3 ||
                                inst->GetOpcode() == IR::Opcode::CompositeConstructU32x4));
                for (s32 i = 0; i < num_dwords; i++) {
                    ir.SetPatch(get_factor_attr(gcn_factor_idx + i), GetValue(inst->Arg(i)));
                }
                break;
            }

            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::WriteSharedU64: {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                const u32 num_dwords = opcode == IR::Opcode::WriteSharedU32 ? 1 : 2;
                const IR::U32 addr{inst.Arg(0)};
                const IR::Value data = num_dwords == 2
                                           ? ir.UnpackUint2x32(IR::U64{inst.Arg(1).Resolve()})
                                           : inst.Arg(1).Resolve();

                const auto SetOutput = [&](IR::U32 addr, IR::U32 value, AttributeRegion output_kind,
                                           u32 off_dw) {
                    const IR::F32 data_component = ir.BitCast<IR::F32, IR::U32>(value);

                    if (output_kind == AttributeRegion::OutputCP) {
                        if (off_dw > 0) {
                            addr = ir.IAdd(addr, ir.Imm32(off_dw));
                        }
                        const u32 stride = runtime_info.hs_info.hs_output_cp_stride;
                        // Invocation ID array index is implicit, handled by SPIRV backend
                        const IR::U32 opt_addr = TryOptimizeAddressModulo(addr, stride, ir);
                        const IR::U32 offset = ir.IMod(opt_addr, ir.Imm32(stride));
                        const IR::U32 attr_index = ir.ShiftRightLogical(offset, ir.Imm32(4u));
                        const IR::U32 comp_index = ir.ShiftRightLogical(
                            ir.BitwiseAnd(offset, ir.Imm32(0xFU)), ir.Imm32(2u));
                        ir.SetTcsGenericAttribute(data_component, attr_index, comp_index);
                    } else {
                        ASSERT(output_kind == AttributeRegion::PatchConst);
                        ASSERT_MSG(addr.IsImmediate(), "patch addr non imm, inst {}",
                                   fmt::ptr(addr.Inst()));
                        ir.SetPatch(IR::PatchGeneric((addr.U32() >> 2) + off_dw), data_component);
                    }
                };

                AttributeRegion region = GetAttributeRegionKind(&inst, info, runtime_info);
                if (num_dwords == 1) {
                    SetOutput(addr, IR::U32{data}, region, 0);
                } else {
                    for (auto i = 0; i < num_dwords; i++) {
                        SetOutput(addr, IR::U32{ir.CompositeExtract(data, i)}, region, i);
                    }
                }
                inst.Invalidate();
                break;
            }

            case IR::Opcode::LoadSharedU32:
            case IR::Opcode::LoadSharedU64: {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                const IR::U32 addr{inst.Arg(0)};
                const AttributeRegion region = GetAttributeRegionKind(&inst, info, runtime_info);
                const u32 num_dwords = opcode == IR::Opcode::LoadSharedU32 ? 1 : 2;
                ASSERT_MSG(region == AttributeRegion::InputCP ||
                               region == AttributeRegion::OutputCP,
                           "Unhandled read of patchconst attribute in hull shader");
                const bool is_tcs_output_read = region == AttributeRegion::OutputCP;
                const u32 stride = is_tcs_output_read ? runtime_info.hs_info.hs_output_cp_stride
                                                      : runtime_info.hs_info.ls_stride;
                IR::Value attr_read;
                if (num_dwords == 1) {
                    attr_read = ir.BitCast<IR::U32>(
                        ReadTessControlPointAttribute(addr, stride, ir, 0, is_tcs_output_read));
                } else {
                    boost::container::static_vector<IR::Value, 4> read_components;
                    for (auto i = 0; i < num_dwords; i++) {
                        const IR::F32 component =
                            ReadTessControlPointAttribute(addr, stride, ir, i, is_tcs_output_read);
                        read_components.push_back(ir.BitCast<IR::U32>(component));
                    }
                    attr_read = ir.PackUint2x32(ir.CompositeConstruct(read_components));
                }
                inst.ReplaceUsesWithAndRemove(attr_read);
                break;
            }

            default:
                break;
            }
        }
    }

    if (runtime_info.hs_info.IsPassthrough()) {
        // Copy input attributes to output attributes, indexed by InvocationID
        // Passthrough should imply that input and output patches have same number of vertices
        IR::Block* entry_block = *program.blocks.begin();
        auto it = std::ranges::find_if(entry_block->Instructions(), [](IR::Inst& inst) {
            return inst.GetOpcode() == IR::Opcode::Prologue;
        });
        ASSERT(it != entry_block->end());
        ++it;
        ASSERT(it != entry_block->end());
        ++it;
        // Prologue
        // SetExec #true
        // <- insert here
        // ...
        IR::IREmitter ir{*entry_block, it};

        u32 num_attributes = Common::AlignUp(runtime_info.hs_info.ls_stride, 16) >> 4;
        const auto invocation_id = ir.GetAttributeU32(IR::Attribute::InvocationId);
        for (u32 attr_no = 0; attr_no < num_attributes; attr_no++) {
            for (u32 comp = 0; comp < 4; comp++) {
                IR::F32 attr_read =
                    ir.GetTessGenericAttribute(invocation_id, ir.Imm32(attr_no), ir.Imm32(comp));
                // InvocationId is implicit index for output control point writes
                ir.SetTcsGenericAttribute(attr_read, ir.Imm32(attr_no), ir.Imm32(comp));
            }
        }
        // We could wrap the rest of the program in an if stmt
        // CopyInputAttrsToOutputs(); // psuedocode
        // if (InvocationId == 0) {
        //     PatchConstFunction();
        // }
        // But as long as we treat invocation ID as 0 for all threads, shouldn't matter functionally
    }
}

void DomainShaderTransform(const IR::Program& program, const RuntimeInfo& runtime_info) {
    const Info& info = program.info;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            const auto opcode = inst.GetOpcode();
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32:
            case IR::Opcode::LoadSharedU64: {
                const IR::U32 addr{inst.Arg(0)};
                AttributeRegion region = GetAttributeRegionKind(&inst, info, runtime_info);
                const u32 num_dwords = opcode == IR::Opcode::LoadSharedU32 ? 1 : 2;
                const auto GetInput = [&](IR::U32 addr, u32 off_dw) -> IR::F32 {
                    if (region == AttributeRegion::OutputCP) {
                        return ReadTessControlPointAttribute(
                            addr, runtime_info.vs_info.hs_output_cp_stride, ir, off_dw, false);
                    } else {
                        ASSERT(region == AttributeRegion::PatchConst);
                        return ir.GetPatch(IR::PatchGeneric((addr.U32() >> 2) + off_dw));
                    }
                };
                IR::Value attr_read;
                if (num_dwords == 1) {
                    attr_read = ir.BitCast<IR::U32>(GetInput(addr, 0));
                } else {
                    boost::container::static_vector<IR::Value, 4> read_components;
                    for (auto i = 0; i < num_dwords; i++) {
                        const IR::F32 component = GetInput(addr, i);
                        read_components.push_back(ir.BitCast<IR::U32>(component));
                    }
                    attr_read = ir.PackUint2x32(ir.CompositeConstruct(read_components));
                }
                inst.ReplaceUsesWithAndRemove(attr_read);
                break;
            }
            default:
                break;
            }
        }
    }
}

// Run before either hull or domain transform
void TessellationPreprocess(IR::Program& program, RuntimeInfo& runtime_info) {
    TessellationDataConstantBuffer tess_constants;
    Shader::Info& info = program.info;
    // Find the TessellationDataConstantBuffer V#
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            auto found_tess_consts_sharp = [&]() -> bool {
                switch (inst.GetOpcode()) {
                case IR::Opcode::LoadSharedU32:
                case IR::Opcode::LoadSharedU64:
                case IR::Opcode::WriteSharedU32:
                case IR::Opcode::WriteSharedU64: {
                    IR::Value addr = inst.Arg(0);
                    auto read_const_buffer = IR::BreadthFirstSearch(
                        addr, [](IR::Inst* maybe_tess_const) -> std::optional<IR::Inst*> {
                            if (maybe_tess_const->GetOpcode() == IR::Opcode::ReadConstBuffer) {
                                return maybe_tess_const;
                            }
                            return std::nullopt;
                        });
                    if (read_const_buffer) {
                        auto sharp_location = FindTessConstantSharp(read_const_buffer.value());
                        if (sharp_location) {
                            if (info.tess_consts_dword_offset >= 0) {
                                // Its possible theres a readconstbuffer that contributes to an
                                // LDS address and isnt a TessConstant V# read. Could improve on
                                // this somehow
                                ASSERT_MSG(static_cast<s32>(sharp_location->dword_off) ==
                                                   info.tess_consts_dword_offset &&
                                               sharp_location->ptr_base ==
                                                   info.tess_consts_ptr_base,
                                           "TessConstants V# is ambiguous");
                            }
                            InitTessConstants(sharp_location->ptr_base,
                                              static_cast<s32>(sharp_location->dword_off), info,
                                              runtime_info, tess_constants);
                            return true;
                        }
                        UNREACHABLE_MSG("Failed to match tess constant sharp");
                    }
                    return false;
                }
                default:
                    return false;
                }
            }();

            if (found_tess_consts_sharp) {
                break;
            }
        }
    }

    ASSERT(info.tess_consts_dword_offset >= 0);

    TessConstantUseWalker walker;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
                auto sharp_location = FindTessConstantSharp(&inst);
                if (sharp_location && sharp_location->ptr_base == info.tess_consts_ptr_base &&
                    sharp_location->dword_off == info.tess_consts_dword_offset) {
                    // The shader is reading from the TessConstants V#
                    IR::Value index = inst.Arg(1);

                    ASSERT_MSG(index.IsImmediate(),
                               "Tessellation constant read with dynamic index");
                    u32 off_dw = index.U32();
                    ASSERT(off_dw <=
                           static_cast<u32>(TessConstantAttribute::FirstEdgeTessFactorIndex));

                    auto tess_const_attr = static_cast<TessConstantAttribute>(off_dw);
                    switch (tess_const_attr) {
                    case TessConstantAttribute::LsStride:
                        // If not, we may need to make this runtime state for TES
                        ASSERT(info.l_stage == LogicalStage::TessellationControl);
                        inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.ls_stride));
                        break;
                    case TessConstantAttribute::HsCpStride:
                        inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.hs_cp_stride));
                        break;
                    case TessConstantAttribute::HsNumPatch:
                    case TessConstantAttribute::HsOutputBase:
                    case TessConstantAttribute::PatchConstBase:
                        walker.WalkUsersOfTessConstant(&inst, tess_const_attr);
                        // We should be able to safely set these to 0 so that indexing happens only
                        // within the local patch in the recompiled Vulkan shader. This assumes
                        // these values only contribute to address calculations for in/out
                        // attributes in the original gcn shader.
                        // See the explanation for why we set V2 to 0 when emitting the prologue.
                        inst.ReplaceUsesWithAndRemove(IR::Value(0u));
                        break;
                    case Shader::TessConstantAttribute::PatchConstSize:
                    case Shader::TessConstantAttribute::PatchOutputSize:
                    case Shader::TessConstantAttribute::OffChipTessellationFactorThreshold:
                    case Shader::TessConstantAttribute::FirstEdgeTessFactorIndex:
                        // May need to replace PatchConstSize and PatchOutputSize with 0
                        break;
                    default:
                        UNREACHABLE_MSG("Read past end of TessConstantsBuffer");
                    }
                }
            }
        }
    }

    // These pattern matching are neccessary for now unless we support dynamic indexing of
    // PatchConst attributes and tess factors. PatchConst should be easy, turn those into a single
    // vec4 array like in/out attrs. Not sure about tess factors.
    if (info.l_stage == LogicalStage::TessellationControl) {
        // Replace the BFEs on V1 (packed with patch id within VGT and output cp id)
        for (IR::Block* block : program.blocks) {
            for (auto it = block->Instructions().begin(); it != block->Instructions().end(); it++) {
                IR::Inst& inst = *it;
                if (M_BITFIELDUEXTRACT(
                        M_GETATTRIBUTEU32(MatchAttribute(IR::Attribute::PackedHullInvocationInfo),
                                          MatchIgnore()),
                        MatchU32(0), MatchU32(8))
                        .Match(IR::Value{&inst})) {
                    IR::IREmitter emit(*block, it);
                    // This is the patch id within the VGT, not the actual PrimitiveId
                    // in the draw
                    IR::Value replacement(0u);
                    inst.ReplaceUsesWithAndRemove(replacement);
                } else if (M_BITFIELDUEXTRACT(
                               M_GETATTRIBUTEU32(
                                   MatchAttribute(IR::Attribute::PackedHullInvocationInfo),
                                   MatchIgnore()),
                               MatchU32(8), MatchU32(5))
                               .Match(IR::Value{&inst})) {
                    IR::IREmitter ir(*block, it);
                    IR::Value replacement;
                    if (runtime_info.hs_info.IsPassthrough()) {
                        // Deal with annoying pattern in BB where InvocationID use makes no
                        // sense (in addr calculation for patchconst or tess factor write)
                        replacement = ir.Imm32(0);
                    } else {
                        replacement = ir.GetAttributeU32(IR::Attribute::InvocationId);
                    }
                    inst.ReplaceUsesWithAndRemove(replacement);
                }
            }
        }
    }

    ConstantPropagationPass(program.post_order_blocks);
}

} // namespace Shader::Optimization
