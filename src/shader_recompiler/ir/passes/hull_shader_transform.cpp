// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include <numeric>
#include "common/assert.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/pattern_matching.h"
#include "shader_recompiler/ir/program.h"

// TODO delelte
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Optimization {

static void DumpIR(IR::Program& program, std::string phase) {
    std::string s = IR::DumpProgram(program);
    using namespace Common::FS;
    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename =
        fmt::format("{}_{:#018x}.{}.ir.txt", program.info.stage, program.info.pgm_hash, phase);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
    file.WriteString(s);
};

/**
 * Tessellation shaders pass outputs to the next shader using LDS.
 * The Hull shader stage receives input control points stored in LDS.
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
 * - Per-patch TCS outputs for patch 0
 * - Per-patch TCS outputs for patch 1
 * - Per-patch TCS outputs for patch 2
 *
 * If the Hull stage does not write any new control points the driver will
 * optimize LDS layout so input and output control point spaces overlap.
 *
 * Tessellation factors are stored in the per-patch TCS output block
 * as well as a factor V# that is automatically bound by the driver.
 *
 * This pass attempts to resolve LDS accesses to attribute accesses and correctly
 * write to the tessellation factor tables. For the latter we replace the
 * buffer store instruction to factor writes according to their offset.
 *
 * LDS stores can either be output control point writes or per-patch data writes.
 * This is detected by looking at how the address is formed. In any case the calculation
 * will be of the form a * b + c. For output control points a = output_control_point_id
 * while for per-patch writes a = patch_id.
 *
 * Both patch_id and output_control_point_id are packed in VGPR1 by the driver and shader
 * uses V_BFE_U32 to extract them. We use the starting bit_pos to determine which is which.
 *
 * LDS reads are more tricky as amount of different calculations performed can vary.
 * The final result, if output control point space is distinct, is of the form:
 * patch_id * input_control_point_stride * num_control_points_per_input_patch + a
 * The value "a" can be anything in the range of [0, input_control_point_stride]
 *
 * This pass does not attempt to deduce the exact attribute referenced by "a" but rather
 * only using "a" itself index into input attributes. Those are defined as an array in the shader
 * layout (location = 0) in vec4[num_control_points_per_input_patch] attr[];
 * ...
 * float value = attr[a / in_stride][(a % in_stride) >> 4][(a & 0xF) >> 2];
 *
 * This requires knowing in_stride which is not provided to us by the guest.
 * To deduce it we perform a breadth first search on the arguments of a DS_READ*
 * looking for a buffer load with offset = 0. This will be the buffer holding tessellation
 * constants and it contains the value of in_stride we can read at compile time.
 *
 * NOTE: This pass must be run before constant propagation as it relies on relatively specific
 * pattern matching that might be mutated that that optimization pass.
 *
 * TODO: need to be careful about reading from output arrays at idx other than InvocationID
 * Need SPIRV OpControlBarrier
 * "Wait for all active invocations within the specified Scope to reach the current point of
 * execution."
 * Must be placed in uniform control flow
 */

// Addr calculations look something like this, but can vary wildly due to decisions made by
// the ps4 compiler (instruction selection, etc)
// Input control point:
//     PrimitiveId * input_cp_stride * #cp_per_input_patch + index * input_cp_stride + (attr# * 16 +
//     component)
// Output control point
//    #patches * input_cp_stride * #cp_per_input_patch + PrimitiveId * output_patch_stride +
//    InvocationID * output_cp_stride + (attr# * 16 + component)
// Per patch output:
//    #patches * input_cp_stride * #cp_per_input_patch + #patches * output_patch_stride +
//    + PrimitiveId * per_patch_output_stride + (attr# * 16 + component)

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
// For certain ReadConstBuffer instructions using the tess constants V#,, we visit the users
// recursively and increment a counter on the Load/WriteShared users.
// Namely NumPatch (from m_hsNumPatch), HsOutputBase (m_hsOutputBase),
// and PatchConstBase (m_patchConstBase).
// In addr calculations, the term NumPatch * ls_stride * #input_cp_in_patch
// is used as an addend to skip the region for input control points, and similarly
// NumPatch * hs_cp_stride * #output_cp_in_patch is used to skip the region
// for output control points.
// The Input CP, Output CP, and PatchConst regions are laid out in that order for the
// entire thread group, so seeing the TcsNumPatches attribute used in an addr calc should
// increment the "region counter" by 1 for the given Load/WriteShared
//
// TODO this will break if AMD compiler used distributive property like
// TcsNumPatches * (ls_stride * #input_cp_in_patch + hs_cp_stride * #output_cp_in_patch)
//
// TODO can we just look at address post-constant folding, pull out all the constants
// and find the interval it's inside of? (phis are still a problem here)
class TessConstantUseWalker {
public:
    void MarkTessAttributeUsers(IR::Inst* read_const_buffer, TessConstantAttribute attr) {
        uint inc;
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
            MarkTessAttributeUsersHelper(use, inc);
        }

        ++seq_num;
    }

private:
    void MarkTessAttributeUsersHelper(IR::Use use, uint inc) {
        IR::Inst* inst = use.user;

        switch (use.user->GetOpcode()) {
        case IR::Opcode::LoadSharedU32:
        case IR::Opcode::LoadSharedU64:
        case IR::Opcode::LoadSharedU128:
        case IR::Opcode::WriteSharedU32:
        case IR::Opcode::WriteSharedU64:
        case IR::Opcode::WriteSharedU128: {
            uint counter = inst->Flags<u32>();
            inst->SetFlags<u32>(counter + inc);
            // Stop here
            return;
        }
        case IR::Opcode::Phi: {
            struct PhiCounter {
                u16 seq_num;
                u8 unique_edge;
                u8 counter;
            };

            PhiCounter count = inst->Flags<PhiCounter>();
            ASSERT_MSG(count.counter == 0 || count.unique_edge == use.operand);
            // the point of seq_num is to tell us if we've already traversed this
            // phi on the current walk. Alternatively we could keep a set of phi's
            // seen on the current walk. This is to handle phi cycles
            if (count.seq_num == 0) {
                // First time we've encountered this phi
                count.seq_num = seq_num;
                // Mark the phi as having been traversed originally through this edge
                count.unique_edge = use.operand;
                count.counter = inc;
            } else if (count.seq_num < seq_num) {
                count.seq_num = seq_num;
                // For now, assume we are visiting this phi via the same edge
                // as on other walks. If not, some dataflow analysis might be necessary
                ASSERT(count.unique_edge == use.operand);
                count.counter += inc;
            } else {
                // count.seq_num == seq_num
                // there's a cycle, and we've already been here on this walk
                return;
            }
            inst->SetFlags<PhiCounter>(count);
            break;
        }
        default:
            break;
        }

        for (IR::Use use : inst->Uses()) {
            MarkTessAttributeUsersHelper(use, inc);
        }
    }

    uint seq_num{1u};
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
#if 1
    std::vector<IR::U32> addends;
    if (TryOptimizeAddendInModulo(addr, stride, addends)) {
        addr = ir.Imm32(0);
        for (auto& addend : addends) {
            addr = ir.IAdd(addr, addend);
        }
    }
#endif
    return addr;
}

// Read a TCS input (InputCP region) or TES input (OutputCP region)
static IR::F32 ReadTessInputComponent(IR::U32 addr, const u32 stride, IR::IREmitter& ir,
                                      u32 off_dw) {
    if (off_dw > 0) {
        addr = ir.IAdd(addr, ir.Imm32(off_dw));
    }
    const IR::U32 control_point_index = ir.IDiv(addr, ir.Imm32(stride));
    const IR::U32 addr_for_attrs = TryOptimizeAddressModulo(addr, stride, ir);
    const IR::U32 attr_index =
        ir.ShiftRightLogical(ir.IMod(addr_for_attrs, ir.Imm32(stride)), ir.Imm32(4u));
    const IR::U32 comp_index =
        ir.ShiftRightLogical(ir.BitwiseAnd(addr_for_attrs, ir.Imm32(0xFU)), ir.Imm32(2u));
    return ir.GetTessGenericAttribute(control_point_index, attr_index, comp_index);
}

} // namespace

void HullShaderTransform(IR::Program& program, RuntimeInfo& runtime_info) {
    const Info& info = program.info;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            IR::IREmitter ir{*block,
                             IR::Block::InstructionList::s_iterator_to(inst)}; // TODO sink this
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::StoreBufferU32:
            case IR::Opcode::StoreBufferU32x2:
            case IR::Opcode::StoreBufferU32x3:
            case IR::Opcode::StoreBufferU32x4: {
                const auto info = inst.Flags<IR::BufferInstInfo>();
                if (!info.globally_coherent) {
                    break;
                }
                const auto GetValue = [&](IR::Value data) -> IR::F32 {
                    if (auto* inst = data.TryInstRecursive();
                        inst && inst->GetOpcode() == IR::Opcode::BitCastU32F32) {
                        return IR::F32{inst->Arg(0)};
                    }
                    return ir.BitCast<IR::F32, IR::U32>(IR::U32{data});
                };
                const u32 num_dwords = u32(opcode) - u32(IR::Opcode::StoreBufferU32) + 1;
                IR::U32 index = IR::U32{inst.Arg(1)};
                ASSERT(index.IsImmediate());
                const u32 gcn_factor_idx = (info.inst_offset.Value() + index.U32()) >> 2;

                const IR::Value data = inst.Arg(2);
                auto get_factor_attr = [&](u32 gcn_factor_idx) -> IR::Patch {
                    // The hull outputs tess factors in different formats depending on the shader.
                    // For triangle domains, it seems to pack the entries into 4 consecutive floats,
                    // with the 3 edge factors followed by the 1 interior factor.
                    // For quads, it does the expected 4 edge factors then 2 interior.
                    // There is a tess factor stride member of the GNMX hull constants struct in
                    // a hull program shader binary archive, but this doesn't seem to be
                    // communicated to the driver. The fixed function tessellator would need to know
                    // this somehow. It's probably implied by the type of the abstract domain. If
                    // this is causing problems, good idea to check the hs_regs argument to
                    // sceGnmSetHsShader. The memory containing the tess factor stride probably
                    // follows the memory for hs_regs if the app is providing a pointer into the
                    // program they loaded from disk
                    switch (runtime_info.hs_info.tess_type) {
                    case AmdGpu::TessellationType::Quad:
                        ASSERT(gcn_factor_idx < 6);
                        return IR::PatchFactor(gcn_factor_idx);
                    case AmdGpu::TessellationType::Triangle:
                        ASSERT(gcn_factor_idx < 4);
                        if (gcn_factor_idx == 3) {
                            return IR::Patch::TessellationLodInteriorU;
                        }
                        return IR::PatchFactor(gcn_factor_idx);
                    default:
                        // TODO point domain types haven't been seen so far
                        UNREACHABLE_MSG("Unhandled tess type");
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
            case IR::Opcode::WriteSharedU64:
            case IR::Opcode::WriteSharedU128: {
                const u32 num_dwords = opcode == IR::Opcode::WriteSharedU32
                                           ? 1
                                           : (opcode == IR::Opcode::WriteSharedU64 ? 2 : 4);
                const IR::U32 addr{inst.Arg(0)};
                const IR::U32 data{inst.Arg(1).Resolve()};

                const auto SetOutput = [&](IR::U32 addr, IR::U32 value, AttributeRegion output_kind,
                                           u32 off_dw) {
                    const IR::F32 data_component = ir.BitCast<IR::F32, IR::U32>(value);

                    if (output_kind == AttributeRegion::OutputCP) {
                        if (off_dw > 0) {
                            addr = ir.IAdd(addr, ir.Imm32(off_dw));
                        }
                        u32 stride = runtime_info.hs_info.hs_output_cp_stride;
                        // Invocation ID array index is implicit, handled by SPIRV backend
                        const IR::U32 addr_for_attrs = TryOptimizeAddressModulo(addr, stride, ir);
                        const IR::U32 attr_index = ir.ShiftRightLogical(
                            ir.IMod(addr_for_attrs, ir.Imm32(stride)), ir.Imm32(4u));
                        const IR::U32 comp_index = ir.ShiftRightLogical(
                            ir.BitwiseAnd(addr_for_attrs, ir.Imm32(0xFU)), ir.Imm32(2u));
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
                    SetOutput(addr, data, region, 0);
                } else {
                    for (auto i = 0; i < num_dwords; i++) {
                        SetOutput(addr, IR::U32{data.Inst()->Arg(i)}, region, i);
                    }
                }
                inst.Invalidate();
                break;
            }

            case IR::Opcode::LoadSharedU32: {
            case IR::Opcode::LoadSharedU64:
            case IR::Opcode::LoadSharedU128:
                const IR::U32 addr{inst.Arg(0)};
                AttributeRegion region = GetAttributeRegionKind(&inst, info, runtime_info);
                const u32 num_dwords = opcode == IR::Opcode::LoadSharedU32
                                           ? 1
                                           : (opcode == IR::Opcode::LoadSharedU64 ? 2 : 4);
                ASSERT_MSG(region == AttributeRegion::InputCP,
                           "Unhandled read of output or patchconst attribute in hull shader");
                IR::Value attr_read;
                if (num_dwords == 1) {
                    attr_read = ir.BitCast<IR::U32>(
                        ReadTessInputComponent(addr, runtime_info.hs_info.ls_stride, ir, 0));
                } else {
                    boost::container::static_vector<IR::Value, 4> read_components;
                    for (auto i = 0; i < num_dwords; i++) {
                        const IR::F32 component =
                            ReadTessInputComponent(addr, runtime_info.hs_info.ls_stride, ir, i);
                        read_components.push_back(ir.BitCast<IR::U32>(component));
                    }
                    attr_read = ir.CompositeConstruct(read_components);
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

        ASSERT(runtime_info.hs_info.ls_stride % 16 == 0);
        u32 num_attributes = runtime_info.hs_info.ls_stride / 16;
        const auto invocation_id = ir.GetAttributeU32(IR::Attribute::InvocationId);
        for (u32 attr_no = 0; attr_no < num_attributes; attr_no++) {
            for (u32 comp = 0; comp < 4; comp++) {
                IR::F32 attr_read =
                    ir.GetTessGenericAttribute(invocation_id, ir.Imm32(attr_no), ir.Imm32(comp));
                // InvocationId is implicit index for output control point writes
                ir.SetTcsGenericAttribute(attr_read, ir.Imm32(attr_no), ir.Imm32(comp));
            }
        }
        // TODO: wrap rest of program with if statement when passthrough?
        // copy passthrough attributes ...
        // if (InvocationId == 0) {
        //    program ...
        // }
        // But as long as we treat invocation ID as 0 for all threads, shouldn't matter functionally
    }
}

// TODO refactor
void DomainShaderTransform(IR::Program& program, RuntimeInfo& runtime_info) {
    Info& info = program.info;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            const auto opcode = inst.GetOpcode();
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32: {
            case IR::Opcode::LoadSharedU64:
            case IR::Opcode::LoadSharedU128:
                const IR::U32 addr{inst.Arg(0)};
                AttributeRegion region = GetAttributeRegionKind(&inst, info, runtime_info);
                const u32 num_dwords = opcode == IR::Opcode::LoadSharedU32
                                           ? 1
                                           : (opcode == IR::Opcode::LoadSharedU64 ? 2 : 4);
                const auto GetInput = [&](IR::U32 addr, u32 off_dw) -> IR::F32 {
                    if (region == AttributeRegion::OutputCP) {
                        return ReadTessInputComponent(
                            addr, runtime_info.vs_info.hs_output_cp_stride, ir, off_dw);
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
                    attr_read = ir.CompositeConstruct(read_components);
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

// Run before copy prop
void TessellationPreprocess(IR::Program& program, RuntimeInfo& runtime_info) {
    TessellationDataConstantBuffer tess_constants;
    Shader::Info& info = program.info;
    // Find the TessellationDataConstantBuffer V#
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32:
            case IR::Opcode::LoadSharedU64:
            case IR::Opcode::LoadSharedU128:
            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::WriteSharedU64:
            case IR::Opcode::WriteSharedU128: {
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
                        if (info.FoundTessConstantsSharp()) {
                            ASSERT(static_cast<s32>(sharp_location->dword_off) ==
                                       info.tess_consts_dword_offset &&
                                   sharp_location->ptr_base == info.tess_consts_ptr_base);
                        }
                        InitTessConstants(sharp_location->ptr_base,
                                          static_cast<s32>(sharp_location->dword_off), info,
                                          runtime_info, tess_constants);
                        break; // break out of switch and loop
                    }
                    UNREACHABLE_MSG("Failed to match tess constant sharp");
                }
                continue;
            }
            default:
                continue;
            }

            break;
        }
    }

    ASSERT(info.FoundTessConstantsSharp());

    TessConstantUseWalker walker;

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
                auto sharp_location = FindTessConstantSharp(&inst);
                if (sharp_location && sharp_location->ptr_base == info.tess_consts_ptr_base &&
                    sharp_location->dword_off == info.tess_consts_dword_offset) {
                    // Replace the load with a special attribute load (for readability and
                    // easier pattern matching)
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
                        inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.m_lsStride));
                        break;
                    case TessConstantAttribute::HsCpStride:
                        inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.m_hsCpStride));
                        break;
                    case TessConstantAttribute::HsNumPatch:
                    case TessConstantAttribute::HsOutputBase:
                    case TessConstantAttribute::PatchConstBase:
                        walker.MarkTessAttributeUsers(&inst, tess_const_attr);
                        // We should be able to safely set these to 0 so that indexing happens only
                        // within the local patch in the recompiled Vulkan shader. This assumes
                        // these values only contribute to address calculations for in/out
                        // attributes in the original gcn shader.
                        // See the explanation for why we set V2 to 0 when emitting the prologue.
                        inst.ReplaceUsesWithAndRemove(IR::Value(0u));
                        break;
                    // PatchConstSize:
                    // PatchOutputSize:
                    // OffChipTessellationFactorThreshold:
                    // FirstEdgeTessFactorIndex:
                    default:
                        break;
                    }
                }
            }
        }
    }

    // These pattern matching are neccessary for now unless we support dynamic indexing of
    // PatchConst attributes and tess factors. PatchConst should be easy, turn those into a single
    // vec4 array like in/out attrs. Not sure about tess factors.
    if (info.l_stage == LogicalStage::TessellationControl) {
        // Replace the BFEs on V1 (packed with patch id and output cp id) for easier pattern
        // matching
        for (IR::Block* block : program.blocks) {
            for (auto it = block->Instructions().begin(); it != block->Instructions().end(); it++) {
                IR::Inst& inst = *it;
                if (M_BITFIELDUEXTRACT(
                        M_GETATTRIBUTEU32(MatchAttribute(IR::Attribute::PackedHullInvocationInfo),
                                          MatchIgnore()),
                        MatchU32(0), MatchU32(8))
                        .Match(IR::Value{&inst})) {
                    IR::IREmitter emit(*block, it);
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
                        // sense (in addr calculation for patchconst write)
                        replacement = ir.Imm32(0);
                    } else {
                        replacement = ir.GetAttributeU32(IR::Attribute::InvocationId);
                    }
                    inst.ReplaceUsesWithAndRemove(replacement);
                }
            }
        }
    }
}

} // namespace Shader::Optimization
