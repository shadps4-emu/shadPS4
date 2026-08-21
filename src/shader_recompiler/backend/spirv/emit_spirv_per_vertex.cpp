// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sirit/sirit.h>
#include "common/hash.h"
#include "shader_recompiler/backend/spirv/emit_spirv_per_vertex.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Backend::SPIRV {

using Sirit::Id;

constexpr u32 SPIRV_VERSION_1_5 = 0x00010500;

u32 UnsupportedBarycentricMask(const RuntimeInfo& runtime_info) {
    const auto& af = runtime_info.fs_info.addr_flags;
    u32 mask = 0;
    if (af.persp_sample_ena) {
        mask |= 1u << 0;
    }
    if (af.persp_centroid_ena) {
        mask |= 1u << 1;
    }
    if (af.persp_pull_model_ena) {
        mask |= 1u << 2;
    }
    if (af.linear_sample_ena) {
        mask |= 1u << 3;
    }
    if (af.linear_center_ena) {
        mask |= 1u << 4;
    }
    if (af.linear_centroid_ena) {
        mask |= 1u << 5;
    }
    return mask;
}

bool HasUnsupportedBarycentricVariant(const RuntimeInfo& runtime_info) {
    return UnsupportedBarycentricMask(runtime_info) != 0;
}

bool CanPerVertexInterp(const Profile& profile, const RuntimeInfo& runtime_info) {
    const auto& fs_info = runtime_info.fs_info;
    // Only GPUs without per-vertex interpolation support (neither AMD explicit vertex parameter
    // nor KHR barycentric) on a
    // direct VS->FS triangle pipeline with no unsupported barycentric variant and no clip-distance
    // emulation can use the per-vertex expansion fallback. All other barycentric variants
    // (sample/centroid/nopersp/pull) are not covered by the fallback and degrade to the existing
    // flat path. Clip-distance emulation occupies input location 0, which the auxiliary GS does not
    // pass through, so such pipelines must also keep the legacy flat path.
    return fs_info.is_vs_direct && fs_info.prim_type == AmdGpu::PrimitiveType::TriangleList &&
           !profile.supports_amd_shader_explicit_vertex_parameter &&
           !profile.supports_fragment_shader_barycentric &&
           !HasUnsupportedBarycentricVariant(runtime_info) && !fs_info.clip_distance_emulation;
}

bool NeedsPerVertexAuxGS(const Profile& profile, const Info& info, const RuntimeInfo& runtime_info) {
    if (!CanPerVertexInterp(profile, runtime_info)) {
        return false;
    }
    const auto& fs_info = runtime_info.fs_info;
    // The aux GS is needed if the FS reads the persp-center barycentric, or any per-vertex
    // attribute (restored to PerVertex during translation).
    if (fs_info.addr_flags.persp_center_ena) {
        return true;
    }
    for (u32 i = 0; i < fs_info.num_inputs; i++) {
        if (info.fs_interpolation[i].primary == Qualifier::PerVertex) {
            return true;
        }
    }
    return false;
}

PerVertexLocations ComputePerVertexLocations(const Profile& profile, const Info& info,
                                             const RuntimeInfo& runtime_info) {
    const auto& fs_info = runtime_info.fs_info;
    const bool can_expand = CanPerVertexInterp(profile, runtime_info);
    PerVertexLocations locs;
    const bool has_clip = fs_info.clip_distance_emulation;
    const u32 num_inputs = fs_info.num_inputs - (has_clip ? 1 : 0);
    u32 loc = has_clip ? 1 : 0;
    for (u32 i = 0; i < num_inputs; i++) {
        const auto& input = fs_info.inputs[i];
        if (input.IsDefault()) {
            continue;
        }
        locs.slot_loc[i] = loc;
        const bool per_vertex = info.fs_interpolation[i].primary == Qualifier::PerVertex;
        loc += (per_vertex && can_expand) ? 3 : 1;
    }
    locs.bary_coord_loc = loc;
    return locs;
}

u64 ComputePerVertexAuxGSSignature(const Info& info, const RuntimeInfo& runtime_info) {
    const auto& fs_info = runtime_info.fs_info;
    const bool has_clip = fs_info.clip_distance_emulation;
    const u32 num_inputs = fs_info.num_inputs - (has_clip ? 1 : 0);
    // Non-zero seed so 0 can be the "no aux GS" sentinel (HashCombine's constant already keeps
    // the result non-zero, but seed explicitly). Covers the FS input interface the aux GS binds
    // to: clip offset (defensive, always 0 here), the persp-center barycentric flag, and the
    // ordered (param_index, is-per-vertex) list of non-default inputs — which jointly determine
    // the GS input locations (param_index) and the output counter layout (order + per-vertex).
    u64 signature = 0x9e3779b97f4a7c15ULL;
    signature = HashCombine(signature, u64(has_clip));
    signature = HashCombine(signature, u64(fs_info.addr_flags.persp_center_ena));
    for (u32 i = 0; i < num_inputs; i++) {
        const auto& input = fs_info.inputs[i];
        if (input.IsDefault()) {
            continue;
        }
        signature = HashCombine(signature, u64(input.param_index));
        signature = HashCombine(signature,
                                u64(info.fs_interpolation[i].primary == Qualifier::PerVertex));
    }
    return signature;
}

namespace {

class PerVertexAuxGSEmitter : public Sirit::Module {
public:
    PerVertexAuxGSEmitter(const Profile& profile_, const Info& info_,
                         const RuntimeInfo& runtime_info_)
        : Sirit::Module{SPIRV_VERSION_1_5}, info{info_}, fs_info{runtime_info_.fs_info} {
        void_id = TypeVoid();
        float_id = TypeFloat(32);
        uint_id = TypeUInt(32U);
        int_id = TypeInt(32U, true);
        vec2_id = TypeVector(float_id, 2);
        vec4_id = TypeVector(float_id, 4);

        const Id float_arr{TypeArray(float_id, Constant(uint_id, 1U))};
        gl_per_vertex_type = TypeStruct(vec4_id, float_id, float_arr, float_arr);
        Decorate(gl_per_vertex_type, spv::Decoration::Block);
        MemberDecorate(gl_per_vertex_type, 0U, spv::Decoration::BuiltIn,
                       static_cast<u32>(spv::BuiltIn::Position));
        MemberDecorate(gl_per_vertex_type, 1U, spv::Decoration::BuiltIn,
                       static_cast<u32>(spv::BuiltIn::PointSize));
        MemberDecorate(gl_per_vertex_type, 2U, spv::Decoration::BuiltIn,
                       static_cast<u32>(spv::BuiltIn::ClipDistance));
        MemberDecorate(gl_per_vertex_type, 3U, spv::Decoration::BuiltIn,
                       static_cast<u32>(spv::BuiltIn::CullDistance));

        has_clip = fs_info.clip_distance_emulation;
        num_inputs = fs_info.num_inputs - (has_clip ? 1 : 0);
        locs = ComputePerVertexLocations(profile_, info_, runtime_info_);
        for (u32 i = 0; i < num_inputs; i++) {
            if (!fs_info.inputs[i].IsDefault()) {
                input_slots.push_back(i);
                per_vertex[i] = info.fs_interpolation[i].primary == Qualifier::PerVertex;
            }
        }
        has_bary_coord = fs_info.addr_flags.persp_center_ena != 0;
    }

    void Emit() {
        DefineEntry();
        DefineInputs();
        DefineOutputs();
        AddEntryPoint(spv::ExecutionModel::Geometry, main, "main", interfaces);
        AddLabel(OpLabel());
        EmitMain();
    }

private:
    Id Int(s32 value) {
        return Constant(int_id, value);
    }

    Id AddInput(Id type) {
        const Id input{AddGlobalVariable(TypePointer(spv::StorageClass::Input, type),
                                         spv::StorageClass::Input)};
        interfaces.push_back(input);
        return input;
    }

    Id AddOutput(Id type) {
        const Id output{AddGlobalVariable(TypePointer(spv::StorageClass::Output, type),
                                          spv::StorageClass::Output)};
        interfaces.push_back(output);
        return output;
    }

    void DefineEntry() {
        AddCapability(spv::Capability::Shader);
        AddCapability(spv::Capability::Geometry);
        const Id void_function{TypeFunction(void_id)};
        main = OpFunction(void_id, spv::FunctionControlMask::MaskNone, void_function);
        AddExecutionMode(main, spv::ExecutionMode::Triangles);
        AddExecutionMode(main, spv::ExecutionMode::OutputTriangleStrip);
        AddExecutionMode(main, spv::ExecutionMode::OutputVertices, 3U);
        AddExecutionMode(main, spv::ExecutionMode::Invocations, 1U);
    }

    void DefineInputs() {
        // gl_in: per-vertex input array carrying gl_Position (3 vertices for a triangle).
        gl_in = AddInput(TypeArray(gl_per_vertex_type, Constant(uint_id, 3U)));
        // Attribute inputs come from the vertex shader, laid out at param_index + clip offset.
        const Id input_arr{TypeArray(vec4_id, Constant(uint_id, 3U))};
        input_ids.reserve(input_slots.size());
        for (u32 i : input_slots) {
            const Id id{AddInput(input_arr)};
            Decorate(id, spv::Decoration::Location,
                     fs_info.inputs[i].param_index + (has_clip ? 1 : 0));
            input_ids.push_back(id);
        }
    }

    void DefineOutputs() {
        gl_out = AddOutput(gl_per_vertex_type);
        output_ids.resize(input_slots.size());
        pv_output_ids.resize(input_slots.size());
        for (size_t k = 0; k < input_slots.size(); k++) {
            const u32 i = input_slots[k];
            const u32 loc = locs.slot_loc[i];
            if (per_vertex[i]) {
                // Expanded per-vertex attribute: 3 flat outputs carrying P0/P10/P20. Flat so the
                // hardware never interpolates the packed 32-bit bit pattern (e.g. f16 pairs).
                for (u32 j = 0; j < 3; j++) {
                    const Id id{AddOutput(vec4_id)};
                    Decorate(id, spv::Decoration::Location, loc + j);
                    Decorate(id, spv::Decoration::Flat);
                    pv_output_ids[k][j] = id;
                }
            } else {
                // Non-per-vertex attribute passes through; interpolation is decided by the FS input.
                const Id id{AddOutput(vec4_id)};
                Decorate(id, spv::Decoration::Location, loc);
                output_ids[k] = id;
            }
        }
        if (has_bary_coord) {
            bary_coord = AddOutput(vec2_id);
            Decorate(bary_coord, spv::Decoration::Location, locs.bary_coord_loc);
        }
    }

    void EmitMain() {
        const Id input_vec4{TypePointer(spv::StorageClass::Input, vec4_id)};
        const Id output_vec4{TypePointer(spv::StorageClass::Output, vec4_id)};

        // Load gl_Position of the 3 input vertices.
        std::array<Id, 3> pos;
        for (int v = 0; v < 3; v++) {
            pos[v] = OpLoad(vec4_id, OpAccessChain(input_vec4, gl_in, Int(v), Int(0)));
        }

        // Load the 3 per-vertex values of every non-default input attribute.
        std::vector<std::array<Id, 3>> attr_vals(input_slots.size());
        for (size_t k = 0; k < input_slots.size(); k++) {
            for (int v = 0; v < 3; v++) {
                attr_vals[k][v] = OpLoad(vec4_id, OpAccessChain(input_vec4, input_ids[k], Int(v)));
            }
        }

        // Barycentric coordinate constants: vertex 0 -> (0,0), 1 -> (1,0), 2 -> (0,1).
        std::array<Id, 3> bary_value;
        if (has_bary_coord) {
            bary_value[0] =
                ConstantComposite(vec2_id, Constant(float_id, 0.0f), Constant(float_id, 0.0f));
            bary_value[1] =
                ConstantComposite(vec2_id, Constant(float_id, 1.0f), Constant(float_id, 0.0f));
            bary_value[2] =
                ConstantComposite(vec2_id, Constant(float_id, 0.0f), Constant(float_id, 1.0f));
        }

        // Emit 3 vertices. Expanded attributes write the same P0/P10/P20 to every output vertex
        // (flat, independent of provoking vertex); passthrough attributes copy the current vertex.
        for (int v = 0; v < 3; v++) {
            OpStore(OpAccessChain(output_vec4, gl_out, Int(0)), pos[v]);
            for (size_t k = 0; k < input_slots.size(); k++) {
                if (per_vertex[input_slots[k]]) {
                    for (int j = 0; j < 3; j++) {
                        OpStore(pv_output_ids[k][j], attr_vals[k][j]);
                    }
                } else {
                    OpStore(output_ids[k], attr_vals[k][v]);
                }
            }
            if (has_bary_coord) {
                OpStore(bary_coord, bary_value[v]);
            }
            OpEmitVertex();
        }
        OpEndPrimitive();
        OpReturn();
        OpFunctionEnd();
    }

private:
    const Info& info;
    const FragmentRuntimeInfo& fs_info;
    PerVertexLocations locs;

    bool has_clip{};
    u32 num_inputs{};
    bool has_bary_coord{};
    std::vector<u32> input_slots;
    std::array<bool, 32> per_vertex{};
    std::vector<Id> input_ids;
    std::vector<Id> output_ids;
    std::vector<std::array<Id, 3>> pv_output_ids;

    Id main;
    Id void_id;
    Id float_id;
    Id uint_id;
    Id int_id;
    Id vec2_id;
    Id vec4_id;
    Id gl_per_vertex_type;
    Id gl_in;
    Id gl_out;
    Id bary_coord;
    std::vector<Id> interfaces;
};

} // namespace

std::vector<u32> EmitPerVertexAuxGS(const Profile& profile, const Info& info,
                                    const RuntimeInfo& runtime_info) {
    PerVertexAuxGSEmitter ctx{profile, info, runtime_info};
    ctx.Emit();
    return ctx.Assemble();
}

} // namespace Shader::Backend::SPIRV
