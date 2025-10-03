// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include "common/assert.h"
#include "common/func_traits.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/types.h"

namespace Shader::Backend::SPIRV {
namespace {

static constexpr spv::ExecutionMode GetInputPrimitiveType(AmdGpu::PrimitiveType type) {
    switch (type) {
    case AmdGpu::PrimitiveType::PointList:
        return spv::ExecutionMode::InputPoints;
    case AmdGpu::PrimitiveType::LineList:
    case AmdGpu::PrimitiveType::LineStrip:
        return spv::ExecutionMode::InputLines;
    case AmdGpu::PrimitiveType::TriangleList:
    case AmdGpu::PrimitiveType::TriangleStrip:
    case AmdGpu::PrimitiveType::RectList:
        return spv::ExecutionMode::Triangles;
    case AmdGpu::PrimitiveType::AdjTriangleList:
        return spv::ExecutionMode::InputTrianglesAdjacency;
    case AmdGpu::PrimitiveType::AdjLineList:
        return spv::ExecutionMode::InputLinesAdjacency;
    default:
        UNREACHABLE_MSG("Unknown input primitive type {}", u32(type));
    }
}

static constexpr spv::ExecutionMode GetOutputPrimitiveType(AmdGpu::GsOutputPrimitiveType type) {
    switch (type) {
    case AmdGpu::GsOutputPrimitiveType::PointList:
        return spv::ExecutionMode::OutputVertices;
    case AmdGpu::GsOutputPrimitiveType::LineStrip:
        return spv::ExecutionMode::OutputLineStrip;
    case AmdGpu::GsOutputPrimitiveType::TriangleStrip:
        return spv::ExecutionMode::OutputTriangleStrip;
    default:
        UNREACHABLE_MSG("Unknown output primitive type {}", u32(type));
    }
}

template <auto func, typename... Args>
void SetDefinition(EmitContext& ctx, IR::Inst* inst, Args... args) {
    inst->SetDefinition<Id>(func(ctx, std::forward<Args>(args)...));
}

template <typename ArgType>
ArgType Arg(EmitContext& ctx, const IR::Value& arg) {
    if constexpr (std::is_same_v<ArgType, Id>) {
        return ctx.Def(arg);
    } else if constexpr (std::is_same_v<ArgType, const IR::Value&>) {
        return arg;
    } else if constexpr (std::is_same_v<ArgType, u32>) {
        return arg.U32();
    } else if constexpr (std::is_same_v<ArgType, u64>) {
        return arg.U64();
    } else if constexpr (std::is_same_v<ArgType, bool>) {
        return arg.U1();
    } else if constexpr (std::is_same_v<ArgType, IR::Attribute>) {
        return arg.Attribute();
    } else if constexpr (std::is_same_v<ArgType, IR::ScalarReg>) {
        return arg.ScalarReg();
    } else if constexpr (std::is_same_v<ArgType, IR::VectorReg>) {
        return arg.VectorReg();
    } else if constexpr (std::is_same_v<ArgType, const char*>) {
        return arg.StringLiteral();
    } else if constexpr (std::is_same_v<ArgType, IR::Patch>) {
        return arg.Patch();
    }
    UNREACHABLE();
}

template <auto func, bool is_first_arg_inst, size_t... I>
void Invoke(EmitContext& ctx, IR::Inst* inst, std::index_sequence<I...>) {
    using Traits = Common::FuncTraits<decltype(func)>;
    if constexpr (std::is_same_v<typename Traits::ReturnType, Id>) {
        if constexpr (is_first_arg_inst) {
            SetDefinition<func>(
                ctx, inst, inst,
                Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        } else {
            SetDefinition<func>(
                ctx, inst, Arg<typename Traits::template ArgType<I + 1>>(ctx, inst->Arg(I))...);
        }
    } else {
        if constexpr (is_first_arg_inst) {
            func(ctx, inst, Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        } else {
            func(ctx, Arg<typename Traits::template ArgType<I + 1>>(ctx, inst->Arg(I))...);
        }
    }
}

template <auto func>
void Invoke(EmitContext& ctx, IR::Inst* inst) {
    using Traits = Common::FuncTraits<decltype(func)>;
    static_assert(Traits::NUM_ARGS >= 1, "Insufficient arguments");
    if constexpr (Traits::NUM_ARGS == 1) {
        Invoke<func, false>(ctx, inst, std::make_index_sequence<0>{});
    } else {
        using FirstArgType = typename Traits::template ArgType<1>;
        static constexpr bool is_first_arg_inst = std::is_same_v<FirstArgType, IR::Inst*>;
        using Indices = std::make_index_sequence<Traits::NUM_ARGS - (is_first_arg_inst ? 2 : 1)>;
        Invoke<func, is_first_arg_inst>(ctx, inst, Indices{});
    }
}

void EmitInst(EmitContext& ctx, IR::Inst* inst) {
    switch (inst->GetOpcode()) {
#define OPCODE(name, result_type, ...)                                                             \
    case IR::Opcode::name:                                                                         \
        return Invoke<&Emit##name>(ctx, inst);
#include "shader_recompiler/ir/opcodes.inc"
#undef OPCODE
    }
    UNREACHABLE_MSG("Invalid opcode {}", inst->GetOpcode());
}

Id TypeId(const EmitContext& ctx, IR::Type type) {
    switch (type) {
    case IR::Type::U1:
        return ctx.U1[1];
    case IR::Type::U32:
        return ctx.U32[1];
    default:
        throw NotImplementedException("Phi node type {}", type);
    }
}

void Traverse(EmitContext& ctx, const IR::Program& program) {
    IR::Block* current_block{};
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::Block: {
            const Id label{node.data.block->Definition<Id>()};
            if (current_block) {
                ctx.OpBranch(label);
            }
            current_block = node.data.block;
            ctx.AddLabel(label);
            for (IR::Inst& inst : node.data.block->Instructions()) {
                EmitInst(ctx, &inst);
            }
            ctx.first_to_last_label_map[label.value] = ctx.last_label;
            break;
        }
        case IR::AbstractSyntaxNode::Type::If: {
            const Id if_label{node.data.if_node.body->Definition<Id>()};
            const Id endif_label{node.data.if_node.merge->Definition<Id>()};
            ctx.OpSelectionMerge(endif_label, spv::SelectionControlMask::MaskNone);
            ctx.OpBranchConditional(ctx.Def(node.data.if_node.cond), if_label, endif_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Loop: {
            const Id body_label{node.data.loop.body->Definition<Id>()};
            const Id continue_label{node.data.loop.continue_block->Definition<Id>()};
            const Id endloop_label{node.data.loop.merge->Definition<Id>()};

            ctx.OpLoopMerge(endloop_label, continue_label, spv::LoopControlMask::MaskNone);
            ctx.OpBranch(body_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Break: {
            const Id break_label{node.data.break_node.merge->Definition<Id>()};
            const Id skip_label{node.data.break_node.skip->Definition<Id>()};
            ctx.OpBranchConditional(ctx.Def(node.data.break_node.cond), break_label, skip_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::EndIf:
            if (current_block) {
                ctx.OpBranch(node.data.end_if.merge->Definition<Id>());
            }
            break;
        case IR::AbstractSyntaxNode::Type::Repeat: {
            Id cond{ctx.Def(node.data.repeat.cond)};
            const Id loop_header_label{node.data.repeat.loop_header->Definition<Id>()};
            const Id merge_label{node.data.repeat.merge->Definition<Id>()};
            ctx.OpBranchConditional(cond, loop_header_label, merge_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Return:
            ctx.OpReturn();
            break;
        case IR::AbstractSyntaxNode::Type::Unreachable:
            ctx.OpUnreachable();
            break;
        }
        if (node.type != IR::AbstractSyntaxNode::Type::Block) {
            current_block = nullptr;
        }
    }
}

Id DefineMain(EmitContext& ctx, const IR::Program& program) {
    const Id void_function{ctx.TypeFunction(ctx.void_id)};
    const Id main{ctx.OpFunction(ctx.void_id, spv::FunctionControlMask::MaskNone, void_function)};
    for (IR::Block* const block : program.blocks) {
        block->SetDefinition(ctx.OpLabel());
    }
    Traverse(ctx, program);
    ctx.OpFunctionEnd();
    return main;
}

spv::ExecutionMode ExecutionMode(AmdGpu::TessellationType primitive) {
    switch (primitive) {
    case AmdGpu::TessellationType::Isoline:
        return spv::ExecutionMode::Isolines;
    case AmdGpu::TessellationType::Triangle:
        return spv::ExecutionMode::Triangles;
    case AmdGpu::TessellationType::Quad:
        return spv::ExecutionMode::Quads;
    }
    UNREACHABLE_MSG("Tessellation primitive {}", primitive);
}

spv::ExecutionMode ExecutionMode(AmdGpu::TessellationPartitioning spacing) {
    switch (spacing) {
    case AmdGpu::TessellationPartitioning::Integer:
        return spv::ExecutionMode::SpacingEqual;
    case AmdGpu::TessellationPartitioning::FracOdd:
        return spv::ExecutionMode::SpacingFractionalOdd;
    case AmdGpu::TessellationPartitioning::FracEven:
        return spv::ExecutionMode::SpacingFractionalEven;
    default:
        break;
    }
    UNREACHABLE_MSG("Tessellation spacing {}", spacing);
}

void SetupCapabilities(const Info& info, const Profile& profile, const RuntimeInfo& runtime_info,
                       EmitContext& ctx) {
    ctx.AddCapability(spv::Capability::Image1D);
    ctx.AddCapability(spv::Capability::Sampled1D);
    ctx.AddCapability(spv::Capability::ImageQuery);
    ctx.AddCapability(spv::Capability::Int8);
    ctx.AddCapability(spv::Capability::Int16);
    ctx.AddCapability(spv::Capability::Int64);
    ctx.AddCapability(spv::Capability::UniformAndStorageBuffer8BitAccess);
    ctx.AddCapability(spv::Capability::UniformAndStorageBuffer16BitAccess);
    if (info.uses_fp16) {
        ctx.AddCapability(spv::Capability::Float16);
    }
    if (info.uses_fp64) {
        ctx.AddCapability(spv::Capability::Float64);
    }
    if (info.has_storage_images) {
        ctx.AddCapability(spv::Capability::StorageImageExtendedFormats);
        ctx.AddCapability(spv::Capability::StorageImageReadWithoutFormat);
        ctx.AddCapability(spv::Capability::StorageImageWriteWithoutFormat);
        if (profile.supports_image_load_store_lod) {
            ctx.AddExtension("SPV_AMD_shader_image_load_store_lod");
            ctx.AddCapability(spv::Capability::ImageReadWriteLodAMD);
        }
    }
    if (info.has_image_gather) {
        ctx.AddCapability(spv::Capability::ImageGatherExtended);
    }
    if (info.has_image_query) {
        ctx.AddCapability(spv::Capability::ImageQuery);
    }
    if ((info.uses_image_atomic_float_min_max && profile.supports_image_fp32_atomic_min_max) ||
        (info.uses_buffer_atomic_float_min_max && profile.supports_buffer_fp32_atomic_min_max)) {
        ctx.AddExtension("SPV_EXT_shader_atomic_float_min_max");
        ctx.AddCapability(spv::Capability::AtomicFloat32MinMaxEXT);
    }
    if (info.uses_lane_id) {
        ctx.AddCapability(spv::Capability::GroupNonUniform);
    }
    if (info.uses_group_quad) {
        ctx.AddCapability(spv::Capability::GroupNonUniformQuad);
    }
    if (info.uses_group_ballot) {
        ctx.AddCapability(spv::Capability::GroupNonUniformBallot);
    }
    const auto stage = info.l_stage;
    if (stage == LogicalStage::Vertex) {
        ctx.AddExtension("SPV_KHR_shader_draw_parameters");
        ctx.AddCapability(spv::Capability::DrawParameters);
    }
    if (stage == LogicalStage::Geometry) {
        ctx.AddCapability(spv::Capability::Geometry);
    }
    if (info.stage == Stage::Fragment) {
        if (profile.supports_amd_shader_explicit_vertex_parameter) {
            ctx.AddExtension("SPV_AMD_shader_explicit_vertex_parameter");
        } else if (profile.supports_fragment_shader_barycentric) {
            ctx.AddExtension("SPV_KHR_fragment_shader_barycentric");
            ctx.AddCapability(spv::Capability::FragmentBarycentricKHR);
        }
        if (info.loads.Get(IR::Attribute::SampleIndex) ||
            runtime_info.fs_info.addr_flags.linear_sample_ena ||
            runtime_info.fs_info.addr_flags.persp_sample_ena) {
            ctx.AddCapability(spv::Capability::SampleRateShading);
        }
        if (info.loads.GetAny(IR::Attribute::RenderTargetIndex)) {
            ctx.AddCapability(spv::Capability::Geometry);
        }
    }
    if (stage == LogicalStage::TessellationControl || stage == LogicalStage::TessellationEval) {
        ctx.AddCapability(spv::Capability::Tessellation);
    }
    if (stage == LogicalStage::Vertex || stage == LogicalStage::TessellationControl ||
        stage == LogicalStage::TessellationEval) {
        if (info.stores.GetAny(IR::Attribute::RenderTargetIndex)) {
            ctx.AddCapability(spv::Capability::ShaderLayer);
        }
        if (info.stores.GetAny(IR::Attribute::ViewportIndex)) {
            ctx.AddCapability(spv::Capability::ShaderViewportIndex);
        }
    } else if (stage == LogicalStage::Geometry &&
               info.stores.GetAny(IR::Attribute::ViewportIndex)) {
        ctx.AddCapability(spv::Capability::MultiViewport);
    }
    if (info.uses_dma) {
        ctx.AddCapability(spv::Capability::PhysicalStorageBufferAddresses);
        ctx.AddExtension("SPV_KHR_physical_storage_buffer");
    }
    const auto shared_type_count = std::popcount(static_cast<u32>(info.shared_types));
    if (shared_type_count > 1 && profile.supports_workgroup_explicit_memory_layout) {
        ctx.AddExtension("SPV_KHR_workgroup_memory_explicit_layout");
        ctx.AddCapability(spv::Capability::WorkgroupMemoryExplicitLayoutKHR);
        ctx.AddCapability(spv::Capability::WorkgroupMemoryExplicitLayout16BitAccessKHR);
    }
    if (info.uses_buffer_int64_atomics || info.uses_shared_int64_atomics) {
        if (info.uses_buffer_int64_atomics) {
            ASSERT_MSG(ctx.profile.supports_buffer_int64_atomics,
                       "Shader requires support for atomic Int64 buffer operations that your "
                       "Vulkan instance does not advertise");
        }
        if (info.uses_shared_int64_atomics) {
            ASSERT_MSG(ctx.profile.supports_shared_int64_atomics,
                       "Shader requires support for atomic Int64 shared memory operations that "
                       "your Vulkan instance does not advertise");
        }
        ctx.AddCapability(spv::Capability::Int64Atomics);
    }
}

void DefineEntryPoint(const Info& info, EmitContext& ctx, Id main) {
    const std::span interfaces(ctx.interfaces.data(), ctx.interfaces.size());
    spv::ExecutionModel execution_model{};
    switch (info.l_stage) {
    case LogicalStage::Compute: {
        const std::array<u32, 3> workgroup_size{ctx.runtime_info.cs_info.workgroup_size};
        execution_model = spv::ExecutionModel::GLCompute;
        ctx.AddExecutionMode(main, spv::ExecutionMode::LocalSize, workgroup_size[0],
                             workgroup_size[1], workgroup_size[2]);
        break;
    }
    case LogicalStage::Vertex:
        execution_model = spv::ExecutionModel::Vertex;
        break;
    case LogicalStage::TessellationControl:
        execution_model = spv::ExecutionModel::TessellationControl;
        ctx.AddCapability(spv::Capability::Tessellation);
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputVertices,
                             ctx.runtime_info.hs_info.NumOutputControlPoints());
        break;
    case LogicalStage::TessellationEval: {
        execution_model = spv::ExecutionModel::TessellationEvaluation;
        const auto& vs_info = ctx.runtime_info.vs_info;
        ctx.AddExecutionMode(main, ExecutionMode(vs_info.tess_type));
        ctx.AddExecutionMode(main, ExecutionMode(vs_info.tess_partitioning));
        ctx.AddExecutionMode(main,
                             vs_info.tess_topology == AmdGpu::TessellationTopology::TriangleCcw
                                 ? spv::ExecutionMode::VertexOrderCcw
                                 : spv::ExecutionMode::VertexOrderCw);
        break;
    }
    case LogicalStage::Fragment:
        execution_model = spv::ExecutionModel::Fragment;
        if (ctx.profile.lower_left_origin_mode) {
            ctx.AddExecutionMode(main, spv::ExecutionMode::OriginLowerLeft);
        } else {
            ctx.AddExecutionMode(main, spv::ExecutionMode::OriginUpperLeft);
        }
        if (info.has_discard) {
            ctx.AddCapability(spv::Capability::DemoteToHelperInvocation);
        }
        if (info.stores.GetAny(IR::Attribute::Depth)) {
            ctx.AddExecutionMode(main, spv::ExecutionMode::DepthReplacing);
        }
        break;
    case LogicalStage::Geometry:
        execution_model = spv::ExecutionModel::Geometry;
        ctx.AddExecutionMode(main, GetInputPrimitiveType(ctx.runtime_info.gs_info.in_primitive));
        ctx.AddExecutionMode(main,
                             GetOutputPrimitiveType(ctx.runtime_info.gs_info.out_primitive[0]));
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputVertices,
                             ctx.runtime_info.gs_info.output_vertices);
        ctx.AddExecutionMode(main, spv::ExecutionMode::Invocations,
                             ctx.runtime_info.gs_info.num_invocations);
        break;
    default:
        UNREACHABLE_MSG("Stage {}", u32(info.stage));
    }
    ctx.AddEntryPoint(execution_model, main, "main", interfaces);
}

void SetupFloatMode(EmitContext& ctx, const Profile& profile, const RuntimeInfo& runtime_info,
                    Id main_func) {
    ctx.AddExtension("SPV_KHR_float_controls");
    const auto fp_denorm_mode = runtime_info.fp_denorm_mode32;
    if (fp_denorm_mode == AmdGpu::FpDenormMode::InOutFlush) {
        if (profile.support_fp32_denorm_flush) {
            ctx.AddCapability(spv::Capability::DenormFlushToZero);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::DenormFlushToZero, 32U);
        }
    } else {
        LOG_WARNING(Render_Vulkan, "Unknown FP denorm mode {}", u32(fp_denorm_mode));
    }
    const auto fp_round_mode = runtime_info.fp_round_mode32;
    if (fp_round_mode == AmdGpu::FpRoundMode::ToZero) {
        if (profile.support_fp32_round_to_zero) {
            ctx.AddCapability(spv::Capability::RoundingModeRTZ);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::RoundingModeRTZ, 32U);
        }
    } else if (fp_round_mode != AmdGpu::FpRoundMode::NearestEven) {
        LOG_WARNING(Render_Vulkan, "Unknown FP rounding mode {}", u32(fp_round_mode));
    }
}

void PatchPhiNodes(const IR::Program& program, EmitContext& ctx) {
    auto inst{program.blocks.front()->begin()};
    size_t block_index{0};
    ctx.PatchDeferredPhi([&](u32 phi_arg, Id first_parent) {
        if (phi_arg == 0) {
            ++inst;
            if (inst == program.blocks[block_index]->end() ||
                inst->GetOpcode() != IR::Opcode::Phi) {
                do {
                    ++block_index;
                    inst = program.blocks[block_index]->begin();
                } while (inst->GetOpcode() != IR::Opcode::Phi);
            }
        }
        const Id arg = ctx.Def(inst->Arg(phi_arg));
        const Id parent = ctx.first_to_last_label_map[first_parent.value];
        return std::make_pair(arg, parent);
    });
}
} // Anonymous namespace

std::vector<u32> EmitSPIRV(const Profile& profile, const RuntimeInfo& runtime_info,
                           const IR::Program& program, Bindings& binding) {
    EmitContext ctx{profile, runtime_info, program.info, binding};
    const Id main{DefineMain(ctx, program)};
    DefineEntryPoint(program.info, ctx, main);
    SetupCapabilities(program.info, profile, runtime_info, ctx);
    SetupFloatMode(ctx, profile, runtime_info, main);
    PatchPhiNodes(program, ctx);
    binding.user_data += program.info.ud_mask.NumRegs();
    return ctx.Assemble();
}

Id EmitPhi(EmitContext& ctx, IR::Inst* inst) {
    const size_t num_args{inst->NumArgs()};
    boost::container::small_vector<Id, 32> blocks;
    blocks.reserve(num_args);
    for (size_t index = 0; index < num_args; ++index) {
        blocks.push_back(inst->PhiBlock(index)->Definition<Id>());
    }
    // The type of a phi instruction is stored in its flags
    const Id result_type{TypeId(ctx, inst->Flags<IR::Type>())};
    return ctx.DeferredOpPhi(result_type, std::span(blocks.data(), blocks.size()));
}

void EmitVoid(EmitContext&) {}

Id EmitIdentity(EmitContext& ctx, const IR::Value& value) {
    throw NotImplementedException("Forward identity declaration");
}

Id EmitConditionRef(EmitContext& ctx, const IR::Value& value) {
    const Id id{ctx.Def(value)};
    if (!Sirit::ValidId(id)) {
        throw NotImplementedException("Forward identity declaration");
    }
    return id;
}

void EmitReference(EmitContext&) {}

void EmitPhiMove(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetScc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetExec(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVcc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetSccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVccHi(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetM0(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetScc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetExec(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVcc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetSccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVccHi(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetM0(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

} // namespace Shader::Backend::SPIRV
