// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include <numeric>
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

// TODO delelte
#include "common/io_file.h"
#include "common/path_util.h"

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

// Bad pattern matching attempt
template <typename Derived>
struct MatchObject {
    inline bool DoMatch(IR::Value v) {
        return static_cast<Derived*>(this)->DoMatch(v);
    }
};

struct MatchValue : MatchObject<MatchValue> {
    MatchValue(IR::Value& return_val_) : return_val(return_val_) {}

    inline bool DoMatch(IR::Value v) {
        return_val = v;
        return true;
    }

private:
    IR::Value& return_val;
};

struct MatchIgnore : MatchObject<MatchIgnore> {
    MatchIgnore() {}

    inline bool DoMatch(IR::Value v) {
        return true;
    }
};

struct MatchImm : MatchObject<MatchImm> {
    MatchImm(IR::Value& v) : return_val(v) {}

    inline bool DoMatch(IR::Value v) {
        if (!v.IsImmediate()) {
            return false;
        }

        return_val = v;
        return true;
    }

private:
    IR::Value& return_val;
};

// Specific
struct MatchAttribute : MatchObject<MatchAttribute> {
    MatchAttribute(IR::Attribute attribute_) : attribute(attribute_) {}

    inline bool DoMatch(IR::Value v) {
        return v.Type() == IR::Type::Attribute && v.Attribute() == attribute;
    }

private:
    IR::Attribute attribute;
};

// Specific
struct MatchU32 : MatchObject<MatchU32> {
    MatchU32(u32 imm_) : imm(imm_) {}

    inline bool DoMatch(IR::Value v) {
        return v.Type() == IR::Type::U32 && v.U32() == imm;
    }

private:
    u32 imm;
};

template <IR::Opcode opcode, typename... Args>
struct MatchInstObject : MatchObject<MatchInstObject<opcode>> {
    static_assert(sizeof...(Args) == IR::NumArgsOf(opcode));
    MatchInstObject(Args&&... args) : pattern(std::forward_as_tuple(args...)) {}

    inline bool DoMatch(IR::Value v) {
        IR::Inst* inst = v.TryInstRecursive();
        if (!inst || inst->GetOpcode() != opcode) {
            return false;
        }

        bool matched = true;

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((matched = matched && std::get<Is>(pattern).DoMatch(inst->Arg(Is))), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});

        return matched;
    }

private:
    using MatchArgs = std::tuple<Args&...>;
    MatchArgs pattern;
};

template <IR::Opcode opcode, typename... Args>
auto MakeInstPattern(Args&&... args) {
    return MatchInstObject<opcode, Args...>(std::forward<Args>(args)...);
}

struct MatchFoldImm : MatchObject<MatchFoldImm> {
    MatchFoldImm(IR::Value& v) : return_val(v) {}

    inline bool DoMatch(IR::Value v);

private:
    IR::Value& return_val;
};

// Represent address as sum of products
// Input control point:
//     PrimitiveId * input_cp_stride * #cp_per_input_patch + index * input_cp_stride + (attr# * 16 +
//     component)
// Output control point
//    #patches * input_cp_stride * #cp_per_input_patch + PrimitiveId * output_patch_stride +
//    InvocationID * output_cp_stride + (attr# * 16 + component)
// Per patch output:
//    #patches * input_cp_stride * #cp_per_input_patch + #patches * output_patch_stride +
//    + PrimitiveId * per_patch_output_stride + (attr# * 16 + component)

// Sort terms left to right

namespace {

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

    if (MakeInstPattern<IR::Opcode::CompositeConstructU32x4>(
            MakeInstPattern<IR::Opcode::GetUserData>(MatchImm(sharp_dword_offset)), MatchIgnore(),
            MatchIgnore(), MatchIgnore())
            .DoMatch(handle)) {
        return TessSharpLocation{.ptr_base = IR::ScalarReg::Max,
                                 .dword_off = static_cast<u32>(sharp_dword_offset.ScalarReg())};
    } else if (MakeInstPattern<IR::Opcode::CompositeConstructU32x4>(
                   MakeInstPattern<IR::Opcode::ReadConst>(
                       MakeInstPattern<IR::Opcode::CompositeConstructU32x2>(
                           MakeInstPattern<IR::Opcode::GetUserData>(MatchImm(sharp_ptr_base)),
                           MatchIgnore()),
                       MatchImm(sharp_dword_offset)),
                   MatchIgnore(), MatchIgnore(), MatchIgnore())
                   .DoMatch(handle)) {
        return TessSharpLocation{.ptr_base = sharp_ptr_base.ScalarReg(),
                                 .dword_off = sharp_dword_offset.U32()};
    }
    UNREACHABLE_MSG("failed to match tess constants sharp buf");
    return {};
}

static IR::Program* g_program; // TODO delete

enum AttributeRegion { InputCP, OutputCP, PatchConst, Unknown };

struct RingAddressInfo {
    AttributeRegion region{};
    u32 attribute_byte_offset{};
    // For InputCP and OutputCP, offset from the start of the patch's memory (including
    // attribute_byte_offset) For PatchConst, not relevant
    IR::U32 offset_in_patch{IR::Value(0u)};
};

class Pass {
public:
    Pass(Info& info_, RuntimeInfo& runtime_info_) : info(info_), runtime_info(runtime_info_) {
        InitTessConstants(info.tess_consts_ptr_base, info.tess_consts_dword_offset, info,
                          runtime_info, tess_constants);
    }

    RingAddressInfo WalkRingAccess(IR::Inst* access, IR::IREmitter& insert_point) {
        Reset();
        RingAddressInfo address_info{};

        IR::Value addr;
        switch (access->GetOpcode()) {
        case IR::Opcode::LoadSharedU32:
        case IR::Opcode::LoadSharedU64:
        case IR::Opcode::LoadSharedU128:
        case IR::Opcode::WriteSharedU32:
        case IR::Opcode::WriteSharedU64:
        case IR::Opcode::WriteSharedU128:
            addr = access->Arg(0);
            break;
        case IR::Opcode::StoreBufferU32:
        case IR::Opcode::StoreBufferU32x2:
        case IR::Opcode::StoreBufferU32x3:
        case IR::Opcode::StoreBufferU32x4:
            addr = access->Arg(1);
            break;
        default:
            UNREACHABLE();
        }

        products.emplace_back(addr);
        Visit(addr);

        FindIndexInfo(address_info, insert_point);

        return address_info;
    }

private:
    void Reset() {
        within_mul = false;
        products.clear();
    }

    void Visit(IR::Value node) {
        IR::Value a, b, c;

        if (MakeInstPattern<IR::Opcode::IMul32>(MatchValue(a), MatchValue(b)).DoMatch(node)) {
            bool saved_within_mul = within_mul;
            within_mul = true;
            Visit(a);
            Visit(b);
            within_mul = saved_within_mul;
        } else if (MakeInstPattern<IR::Opcode::IAdd32>(MatchValue(a), MatchValue(b))
                       .DoMatch(node)) {
            if (within_mul) {
                UNREACHABLE_MSG("Test");
                products.back().as_factors.emplace_back(IR::U32{node});
            } else {
                products.back().as_nested_value = IR::U32{a};
                Visit(a);
                products.emplace_back(b);
                Visit(b);
            }
        } else if (MakeInstPattern<IR::Opcode::ShiftLeftLogical32>(MatchValue(a), MatchImm(b))
                       .DoMatch(node)) {
            products.back().as_factors.emplace_back(IR::Value(u32(2 << (b.U32() - 1))));
            Visit(a);
        } else if (MakeInstPattern<IR::Opcode::ReadConstBuffer>(MatchIgnore(), MatchValue(b))
                       .DoMatch(node)) {
            IR::Inst* read_const_buffer = node.InstRecursive();
            IR::Value index = read_const_buffer->Arg(1);

            if (index.IsImmediate()) {
                u32 offset = index.U32();
                if (offset < static_cast<u32>(IR::Attribute::TcsFirstEdgeTessFactorIndex) -
                                 static_cast<u32>(IR::Attribute::TcsLsStride) + 1) {
                    IR::Attribute tess_constant_attr = static_cast<IR::Attribute>(
                        static_cast<u32>(IR::Attribute::TcsLsStride) + offset);
                    IR::IREmitter ir{*read_const_buffer->GetParent(),
                                     IR::Block::InstructionList::s_iterator_to(*read_const_buffer)};

                    ASSERT(tess_constant_attr !=
                           IR::Attribute::TcsOffChipTessellationFactorThreshold);
                    IR::U32 replacement = ir.GetAttributeU32(tess_constant_attr);

                    read_const_buffer->ReplaceUsesWithAndRemove(replacement);
                    // Unwrap the attribute from the GetAttribute Inst and push back as a factor
                    // (more convenient for scanning the factors later)
                    node = IR::Value{tess_constant_attr};

                    if (IR::Value{read_const_buffer} == products.back().as_nested_value) {
                        products.back().as_nested_value = replacement;
                    }
                }
            }
            products.back().as_factors.emplace_back(node);
        } else if (MakeInstPattern<IR::Opcode::GetAttributeU32>(MatchValue(a), MatchU32(0))
                       .DoMatch(node)) {
            products.back().as_factors.emplace_back(a);
        } else if (MakeInstPattern<IR::Opcode::BitFieldSExtract>(MatchValue(a), MatchIgnore(),
                                                                 MatchIgnore())
                       .DoMatch(node)) {
            Visit(a);
        } else if (MakeInstPattern<IR::Opcode::BitFieldUExtract>(MatchValue(a), MatchIgnore(),
                                                                 MatchIgnore())
                       .DoMatch(node)) {
            Visit(a);
        } else if (MakeInstPattern<IR::Opcode::BitCastF32U32>(MatchValue(a)).DoMatch(node)) {
            return Visit(a);
        } else if (MakeInstPattern<IR::Opcode::BitCastU32F32>(MatchValue(a)).DoMatch(node)) {
            return Visit(a);
        } else if (node.TryInstRecursive() &&
                   node.InstRecursive()->GetOpcode() == IR::Opcode::Phi) {
            DEBUG_ASSERT(false && "Phi test");
            products.back().as_factors.emplace_back(node);
        } else {
            products.back().as_factors.emplace_back(node);
        }
    }

    void FindIndexInfo(RingAddressInfo& address_info, IR::IREmitter& ir) {
        // infer which attribute base the address is indexing
        // by how many addends are multiplied by TessellationDataConstantBuffer::m_hsNumPatch.
        // Also handle m_hsOutputBase or m_patchConstBase
        u32 region_count = 0;

        // Remove addends except for the attribute offset and possibly the
        // control point index calc
        std::erase_if(products, [&](Product& p) {
            for (IR::Value& value : p.as_factors) {
                if (value.Type() == IR::Type::Attribute) {
                    if (value.Attribute() == IR::Attribute::TcsNumPatches ||
                        value.Attribute() == IR::Attribute::TcsOutputBase) {
                        ++region_count;
                        return true;
                    } else if (value.Attribute() == IR::Attribute::TcsPatchConstBase) {
                        region_count += 2;
                        return true;
                    } else if (value.Attribute() == IR::Attribute::TessPatchIdInVgt) {
                        return true;
                    }
                }
            }
            return false;
        });

        // DumpIR(*g_program, "before_crash");

        // Look for some term with a dynamic index (should be the control point index)
        for (auto i = 0; i < products.size(); i++) {
            auto& factors = products[i].as_factors;
            // Remember this as the index term
            if (std::any_of(factors.begin(), factors.end(), [&](const IR::Value& v) {
                    return !v.IsImmediate() || v.Type() == IR::Type::Attribute;
                })) {
                address_info.offset_in_patch =
                    ir.IAdd(address_info.offset_in_patch, products[i].as_nested_value);
            } else {
                ASSERT_MSG(factors.size() == 1, "factors all const but not const folded");
                // Otherwise assume it contributes to the attribute
                address_info.offset_in_patch =
                    ir.IAdd(address_info.offset_in_patch, IR::U32{factors[0]});
                address_info.attribute_byte_offset += factors[0].U32();
            }
        }

        if (region_count == 0) {
            address_info.region = AttributeRegion::InputCP;
        } else if (info.l_stage == LogicalStage::TessellationControl &&
                   runtime_info.hs_info.IsPassthrough()) {
            ASSERT(region_count <= 1);
            address_info.region = AttributeRegion::PatchConst;
        } else {
            ASSERT(region_count <= 2);
            address_info.region = AttributeRegion(region_count);
        }
    }

    Info& info;
    RuntimeInfo& runtime_info;

    TessellationDataConstantBuffer tess_constants;
    bool within_mul{};

    // One product in the sum of products making up an address
    struct Product {
        Product(IR::Value val_) : as_nested_value(val_), as_factors() {}
        Product(const Product& other) = default;
        ~Product() = default;

        // IR value used as an addend in address calc
        IR::U32 as_nested_value;
        // all the leaves that feed the multiplication, linear
        // TODO small_vector
        // boost::container::small_vector<IR::Value, 4> as_factors;
        std::vector<IR::Value> as_factors;
    };

    std::vector<Product> products;
};

} // namespace

void HullShaderTransform(IR::Program& program, RuntimeInfo& runtime_info) {
    g_program = &program; // TODO delete
    Info& info = program.info;
    Pass pass(info, runtime_info);

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
                // TODO: rename struct
                RingAddressInfo address_info = pass.WalkRingAccess(&inst, ir);

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
                const u32 gcn_factor_idx =
                    (info.inst_offset.Value() + address_info.attribute_byte_offset) >> 2;

                const IR::Value data = inst.Arg(2);
                auto get_factor_attr = [&](u32 gcn_factor_idx) -> IR::Patch {
                    ASSERT(gcn_factor_idx * 4 < runtime_info.hs_info.tess_factor_stride);

                    switch (runtime_info.hs_info.tess_factor_stride) {
                    case 24:
                        return IR::PatchFactor(gcn_factor_idx);
                    case 16:
                        if (gcn_factor_idx == 3) {
                            return IR::Patch::TessellationLodInteriorU;
                        }
                        return IR::PatchFactor(gcn_factor_idx);

                    default:
                        UNREACHABLE_MSG("Unhandled tess factor stride");
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

            // case IR::Opcode::WriteSharedU128: // TODO
            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::WriteSharedU64: {
                // DumpIR(program, "before_walk");
                RingAddressInfo address_info = pass.WalkRingAccess(&inst, ir);

                const u32 num_dwords = opcode == IR::Opcode::WriteSharedU32
                                           ? 1
                                           : (opcode == IR::Opcode::WriteSharedU64 ? 2 : 4);
                const IR::Value data = inst.Arg(1);
                const auto [data_lo, data_hi] = [&] -> std::pair<IR::U32, IR::U32> {
                    if (num_dwords == 1) {
                        return {IR::U32{data}, IR::U32{}};
                    }
                    const auto* prod = data.InstRecursive();
                    return {IR::U32{prod->Arg(0)}, IR::U32{prod->Arg(1)}};
                }();

                const auto SetOutput = [&](IR::U32 value, u32 offset_dw,
                                           AttributeRegion output_kind) {
                    const IR::F32 data = ir.BitCast<IR::F32, IR::U32>(value);
                    if (output_kind == AttributeRegion::OutputCP) {
                        const u32 param = offset_dw >> 2;
                        const u32 comp = offset_dw & 3;
                        // Invocation ID array index is implicit, handled by SPIRV backend
                        ir.SetAttribute(IR::Attribute::Param0 + param, data, comp);
                    } else {
                        ASSERT(output_kind == AttributeRegion::PatchConst);
                        ir.SetPatch(IR::PatchGeneric(offset_dw), data);
                    }
                };

                u32 offset_dw = address_info.attribute_byte_offset >> 2;
                SetOutput(data_lo, offset_dw, address_info.region);
                if (num_dwords > 1) {
                    // TODO handle WriteSharedU128
                    SetOutput(data_hi, offset_dw + 1, address_info.region);
                }
                inst.Invalidate();

                break;
            }

            case IR::Opcode::LoadSharedU32: {
                // case IR::Opcode::LoadSharedU64:
                // case IR::Opcode::LoadSharedU128:
                RingAddressInfo address_info = pass.WalkRingAccess(&inst, ir);

                ASSERT(address_info.region == AttributeRegion::InputCP ||
                       address_info.region == AttributeRegion::OutputCP);
                switch (address_info.region) {
                case AttributeRegion::InputCP: {
                    u32 offset_dw =
                        (address_info.attribute_byte_offset % runtime_info.hs_info.ls_stride) >> 2;
                    const u32 param = offset_dw >> 2;
                    const u32 comp = offset_dw & 3;
                    IR::Value control_point_index =
                        ir.IDiv(IR::U32{address_info.offset_in_patch},
                                ir.Imm32(runtime_info.hs_info.ls_stride));
                    IR::Value get_attrib =
                        ir.GetAttribute(IR::Attribute::Param0 + param, comp, control_point_index);
                    get_attrib = ir.BitCast<IR::U32>(IR::F32{get_attrib});
                    inst.ReplaceUsesWithAndRemove(get_attrib);
                    break;
                }
                case AttributeRegion::OutputCP: {
                    UNREACHABLE_MSG("Unhandled output control point read");
                    break;
                }
                default:
                    break;
                }
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
        for (u32 i = 0; i < num_attributes; i++) {
            for (u32 j = 0; j < 4; j++) {
                const auto input_attr =
                    ir.GetAttribute(IR::Attribute::Param0 + i, j, invocation_id);
                // InvocationId is implicit index for output control point writes
                ir.SetAttribute(IR::Attribute::Param0 + i, input_attr, j);
            }
        }
        // TODO: wrap rest of program with if statement when passthrough?
        // copy passthrough attributes ...
        // if (InvocationId == 0) {
        //    program ...
        // }
    }
}

// TODO refactor
void DomainShaderTransform(IR::Program& program, RuntimeInfo& runtime_info) {
    Info& info = program.info;
    Pass pass(info, runtime_info);

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            const auto opcode = inst.GetOpcode();
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32: {
                // case IR::Opcode::LoadSharedU64:
                // case IR::Opcode::LoadSharedU128: // TODO
                RingAddressInfo address_info = pass.WalkRingAccess(&inst, ir);

                ASSERT(address_info.region == AttributeRegion::OutputCP ||
                       address_info.region == AttributeRegion::PatchConst);
                switch (address_info.region) {
                case AttributeRegion::OutputCP: {
                    u32 offset_dw = (address_info.attribute_byte_offset %
                                     runtime_info.vs_info.hs_output_cp_stride) >>
                                    2;
                    const u32 param = offset_dw >> 2;
                    const u32 comp = offset_dw & 3;
                    IR::Value control_point_index =
                        ir.IDiv(IR::U32{address_info.offset_in_patch},
                                ir.Imm32(runtime_info.vs_info.hs_output_cp_stride));
                    IR::Value get_attrib =
                        ir.GetAttribute(IR::Attribute::Param0 + param, comp, control_point_index);
                    get_attrib = ir.BitCast<IR::U32>(IR::F32{get_attrib});
                    inst.ReplaceUsesWithAndRemove(get_attrib);
                    break;
                }
                case AttributeRegion::PatchConst: {
                    u32 offset_dw = address_info.attribute_byte_offset >> 2;
                    IR::Value get_patch = ir.GetPatch(IR::PatchGeneric(offset_dw));
                    inst.ReplaceUsesWithAndRemove(get_patch);
                    break;
                }
                default:
                    break;
                }

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
                        // break; TODO
                        continue;
                    }
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

    if (info.l_stage == LogicalStage::TessellationControl) {
        // Replace the BFEs on V1 (packed with patch id and output cp id) for easier pattern
        // matching
        for (IR::Block* block : program.blocks) {
            for (auto it = block->Instructions().begin(); it != block->Instructions().end(); it++) {
                IR::Inst& inst = *it;
                if (MakeInstPattern<IR::Opcode::BitFieldUExtract>(
                        MakeInstPattern<IR::Opcode::GetAttributeU32>(
                            MatchAttribute(IR::Attribute::PackedHullInvocationInfo), MatchIgnore()),
                        MatchU32(0), MatchU32(8))
                        .DoMatch(IR::Value{&inst})) {
                    IR::IREmitter emit(*block, it);
                    IR::Value replacement = emit.GetAttributeU32(IR::Attribute::TessPatchIdInVgt);
                    inst.ReplaceUsesWithAndRemove(replacement);
                } else if (MakeInstPattern<IR::Opcode::BitFieldUExtract>(
                               MakeInstPattern<IR::Opcode::GetAttributeU32>(
                                   MatchAttribute(IR::Attribute::PackedHullInvocationInfo),
                                   MatchIgnore()),
                               MatchU32(8), MatchU32(5))
                               .DoMatch(IR::Value{&inst})) {
                    IR::IREmitter ir(*block, it);
                    IR::Value replacement;
                    if (runtime_info.hs_info.IsPassthrough()) {
                        // Deal with annoying pattern in BB where InvocationID use makes no sense
                        // (in addr calculation for patchconst write)
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

void TessellationPostprocess(IR::Program& program, RuntimeInfo& runtime_info) {
    Shader::Info& info = program.info;
    TessellationDataConstantBuffer tess_constants;
    InitTessConstants(info.tess_consts_ptr_base, info.tess_consts_dword_offset, info, runtime_info,
                      tess_constants);

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::GetAttributeU32) {
                switch (inst.Arg(0).Attribute()) {
                case IR::Attribute::TcsLsStride:
                    ASSERT(info.l_stage == LogicalStage::TessellationControl);
                    inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.m_lsStride));
                    break;
                case IR::Attribute::TcsCpStride:
                    inst.ReplaceUsesWithAndRemove(IR::Value(tess_constants.m_hsCpStride));
                    break;
                case IR::Attribute::TcsNumPatches:
                case IR::Attribute::TcsOutputBase:
                case IR::Attribute::TcsPatchConstSize:
                case IR::Attribute::TcsPatchConstBase:
                case IR::Attribute::TcsPatchOutputSize:
                case IR::Attribute::TcsFirstEdgeTessFactorIndex:
                default:
                    break;
                }
            }
        }
    }

    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32:
            case IR::Opcode::LoadSharedU64:
            case IR::Opcode::LoadSharedU128:
            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::WriteSharedU64:
            case IR::Opcode::WriteSharedU128:
                UNREACHABLE_MSG("Remaining DS instruction. {} transform failed",
                                info.l_stage == LogicalStage::TessellationControl ? "Hull"
                                                                                  : "Domain");
            default:
                break;
            }
        }
    }
}

} // namespace Shader::Optimization
