// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <set>
#include <sirit/sirit.h>

#include "shader_recompiler/backend/spirv/emit_spirv_discard_frag.h"

namespace Shader::Backend::SPIRV {

using Sirit::Id;

constexpr u32 SPIRV_VERSION_1_5 = 0x00010500;

struct DiscardShaderEmitter : public Sirit::Module {
    explicit DiscardShaderEmitter(const std::array<Shader::OutputMap, 3>& outputs)
        : Sirit::Module{SPIRV_VERSION_1_5} {
        void_id = TypeVoid();
        bool_id = TypeBool();
        float_id = TypeFloat(32);
        vec4_id = TypeVector(float_id, 4);

        float_zero = Constant(float_id, 0.0f);

        for (u8 location = 0; location < 3; ++location) {
            for (u8 elem = 0; elem < 4; ++elem) {
                switch (outputs.at(location).at(elem)) {
                case Output::ClipDist0:
                case Output::ClipDist1:
                case Output::ClipDist2:
                case Output::ClipDist3:
                case Output::ClipDist4:
                case Output::ClipDist5:
                case Output::ClipDist6:
                case Output::ClipDist7: {
                    clip_locations.emplace_back(location, elem);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    void EmitDiscardShader() {
        std::set<u8> locations;

        for (auto& [loc, elem] : clip_locations) {
            locations.insert(loc);
        }

        ASSERT_MSG(locations.size() <= 2, "more than 2 clip planes locations");

        AddCapability(spv::Capability::Shader);
        AddExtension("SPV_EXT_demote_to_helper_invocation");
        AddCapability(spv::Capability::DemoteToHelperInvocation);
        SetMemoryModel(spv::AddressingModel::Logical, spv::MemoryModel::GLSL450);
        const Id void_function{TypeFunction(void_id)};
        main = OpFunction(void_id, spv::FunctionControlMask::MaskNone, void_function);

        auto ptr_id = TypePointer(spv::StorageClass::Input, vec4_id);

        std::array<Id, 2> locations_id = {0};
        std::vector<u8> locations_v(locations.begin(), locations.end());
        std::map<u8, Id> loc_vec_id;
        auto locations_id_len = 0;
        for (auto i = 0; i < locations_v.size(); ++i) {
            locations_id_len++;
            locations_id[i] = AddGlobalVariable(ptr_id, spv::StorageClass::Input);
            Decorate(locations_id[i], spv::Decoration::Location, locations_v[i]);
            Name(locations_id[i], fmt::format("ccdist{}_in", i));
        }

        AddEntryPoint(spv::ExecutionModel::Fragment, main, "main",
                      std::span(locations_id).first(locations_id_len));
        AddExecutionMode(main, spv::ExecutionMode::OriginUpperLeft);
        AddLabel(OpLabel());

        for (auto i = 0; i < locations_id_len; ++i) {
            auto loc_vec = OpLoad(vec4_id, locations_id[i]);
            loc_vec_id[locations_v[i]] = loc_vec;
            Name(loc_vec, fmt::format("ccdist{}", i));
        }

        for (auto& [loc, elem] : clip_locations) {
            auto plane = OpCompositeExtract(float_id, loc_vec_id[loc], elem);
            Name(plane, fmt::format("plane{}", loc * 4 + elem));

            auto clipped = OpFOrdLessThan(bool_id, plane, float_zero);
            const Id kill_label{OpLabel()};
            const Id merge_label{OpLabel()};
            OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
            OpBranchConditional(clipped, kill_label, merge_label);
            AddLabel(kill_label);
            OpDemoteToHelperInvocationEXT();
            OpBranch(merge_label);
            AddLabel(merge_label);
        }

        OpReturn();
        OpFunctionEnd();
    }

private:
    Id main;
    Id void_id;
    Id bool_id;
    Id float_id;
    Id vec4_id;
    Id float_zero;

    std::vector<std::tuple<u8, u8>> clip_locations;
};

std::vector<u32> EmitDiscardFragmentShader(std::array<Shader::OutputMap, 3> outputs) {
    DiscardShaderEmitter ctx{outputs};
    ctx.EmitDiscardShader();
    return ctx.Assemble();
}

} // namespace Shader::Backend::SPIRV
