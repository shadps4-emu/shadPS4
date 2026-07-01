// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <set>

#include "common/assert.h"
#include "common/logging/log.h"
#include "sirit/sirit.h"
#include "video_core/renderer_vulkan/vk_shader_emulation.h"

namespace Vulkan {

std::vector<u32> GenerateClipDistanceShaderSpirv(std::span<std::tuple<u8, u8>> clip_locations) {
    Sirit::Module ctx;

    std::set<u8> locations;

    for (auto& [loc, elem] : clip_locations) {
        locations.insert(loc);
    }

    ASSERT_MSG(locations.size() <= 2, "more than 2 clip planes locations");

    ctx.AddCapability(spv::Capability::Shader);
    ctx.AddCapability(spv::Capability::DemoteToHelperInvocation);
    ctx.SetMemoryModel(spv::AddressingModel::Logical, spv::MemoryModel::GLSL450);
    auto void_id = ctx.TypeVoid();
    auto bool_id = ctx.TypeBool();
    auto float_id = ctx.TypeFloat(32);
    auto v4f_id = ctx.TypeVector(float_id, 4);
    auto ptr_id = ctx.TypePointer(spv::StorageClass::Input, v4f_id);
    auto zero_f = ctx.Constant(float_id, 0.0f);
    const Sirit::Id void_function{ctx.TypeFunction(void_id)};

    std::array<Sirit::Id, 2> locations_id = {0};
    std::vector<u8> locations_v(locations.begin(), locations.end());
    std::map<u8, Sirit::Id> loc_vec_id;
    auto locations_id_len = 0;
    for (auto i = 0; i < locations_v.size(); ++i) {
        locations_id_len++;
        locations_id[i] = ctx.AddGlobalVariable(ptr_id, spv::StorageClass::Input);
        ctx.Decorate(locations_id[i], spv::Decoration::Location, locations_v[i]);
        ctx.Name(locations_id[i], fmt::format("ccdist{}_in", i));
    }

    auto main = ctx.OpFunction(void_id, spv::FunctionControlMask::MaskNone, void_function);
    ctx.AddEntryPoint(spv::ExecutionModel::Fragment, main, "main",
                      std::span(locations_id).first(locations_id_len));
    ctx.AddExecutionMode(main, spv::ExecutionMode::OriginUpperLeft);
    ctx.AddLabel(ctx.OpLabel());

    for (auto i = 0; i < locations_id_len; ++i) {
        auto loc_vec = ctx.OpLoad(v4f_id, locations_id[i]);
        loc_vec_id[locations_v[i]] = loc_vec;
        ctx.Name(loc_vec, fmt::format("ccdist{}", i));
    }

    for (auto& [loc, elem] : clip_locations) {
        auto plane = ctx.OpCompositeExtract(float_id, loc_vec_id[loc], elem);
        ctx.Name(plane, fmt::format("plane{}", loc * 4 + elem));

        auto clipped = ctx.OpFOrdLessThan(bool_id, plane, zero_f);
        const Sirit::Id kill_label{ctx.OpLabel()};
        const Sirit::Id merge_label{ctx.OpLabel()};
        ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
        ctx.OpBranchConditional(clipped, kill_label, merge_label);
        ctx.AddLabel(kill_label);
        ctx.OpDemoteToHelperInvocationEXT();
        ctx.OpBranch(merge_label);
        ctx.AddLabel(merge_label);
    }

    ctx.OpReturn();
    ctx.OpFunctionEnd();

    return ctx.Assemble();
}

} // namespace Vulkan
