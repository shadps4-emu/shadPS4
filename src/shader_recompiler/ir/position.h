// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <magic_enum/magic_enum.hpp>
#include "common/logging/log.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::IR {

/// Maps special position export to builtin attribute stores
template <typename StageRuntimeInfo>
inline void ExportPosition(IREmitter& ir, const StageRuntimeInfo& stage, Attribute attribute,
                           u32 comp, const IR::F32& value) {
    if (attribute == Attribute::Position0) {
        ir.SetAttribute(attribute, value, comp);
        return;
    }

    const u32 index = u32(attribute) - u32(Attribute::Position1);
    const auto output = stage.outputs[index][comp];
    if constexpr (std::is_same_v<StageRuntimeInfo, VertexRuntimeInfo>) {
        // Certain outputs are supposed to be set by the last pre-rasterization stage. We don't
        // currently have a mechanism for passing these on when emulating rect/quad lists using
        // tessellation, which comes after, so just ignore the export for now. Note that this
        // only matters for vertex shaders, as geometry shaders come last in pre-rasterization.
        const auto last_stage_required = output == Output::PointSize ||
                                         output == Output::RenderTargetIndex ||
                                         output == Output::ViewportIndex;
        if (stage.tess_emulated_primitive && last_stage_required) {
            LOG_WARNING(Render,
                        "{} is exported in vertex shader but tessellation-based primitive "
                        "emulation is active. Not implemented yet.",
                        magic_enum::enum_name(output));
            return;
        }
    }

    switch (output) {
    case Output::ClipDist0:
    case Output::ClipDist1:
    case Output::ClipDist2:
    case Output::ClipDist3:
    case Output::ClipDist4:
    case Output::ClipDist5:
    case Output::ClipDist6:
    case Output::ClipDist7: {
        const u32 index = u32(output) - u32(Output::ClipDist0);
        ir.SetAttribute(IR::Attribute::ClipDistance, value, index);
        break;
    }
    case Output::CullDist0:
    case Output::CullDist1:
    case Output::CullDist2:
    case Output::CullDist3:
    case Output::CullDist4:
    case Output::CullDist5:
    case Output::CullDist6:
    case Output::CullDist7: {
        const u32 index = u32(output) - u32(Output::CullDist0);
        ir.SetAttribute(IR::Attribute::CullDistance, value, index);
        break;
    }
    case Output::PointSize:
        ir.SetAttribute(IR::Attribute::PointSize, value);
        break;
    case Output::RenderTargetIndex:
        ir.SetAttribute(IR::Attribute::RenderTargetIndex, value);
        break;
    case Output::ViewportIndex:
        ir.SetAttribute(IR::Attribute::ViewportIndex, value);
        break;
    case Output::None:
        LOG_WARNING(Render_Recompiler, "The {} component of {} isn't mapped, skipping",
                    "xyzw"[comp], NameOf(attribute));
        break;
    default:
        UNREACHABLE_MSG("Unhandled output {} on attribute {}", u32(output), NameOf(attribute));
    }
}

} // namespace Shader::IR
