// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "shader_recompiler/ir/attribute.h"

namespace Shader::IR {

std::string NameOf(Attribute attribute) {
    switch (attribute) {
    case Attribute::RenderTarget0:
        return "RenderTarget0";
    case Attribute::RenderTarget1:
        return "RenderTarget1";
    case Attribute::RenderTarget2:
        return "RenderTarget2";
    case Attribute::RenderTarget3:
        return "RenderTarget3";
    case Attribute::RenderTarget4:
        return "RenderTarget4";
    case Attribute::RenderTarget5:
        return "RenderTarget5";
    case Attribute::RenderTarget6:
        return "RenderTarget6";
    case Attribute::RenderTarget7:
        return "RenderTarget7";
    case Attribute::Depth:
        return "Depth";
    case Attribute::Null:
        return "Null";
    case Attribute::Position0:
        return "Position0";
    case Attribute::Position1:
        return "Position1";
    case Attribute::Position2:
        return "Position2";
    case Attribute::Position3:
        return "Position3";
    case Attribute::Param0:
        return "Param0";
    case Attribute::Param1:
        return "Param1";
    case Attribute::Param2:
        return "Param2";
    case Attribute::Param3:
        return "Param3";
    case Attribute::Param4:
        return "Param4";
    case Attribute::Param5:
        return "Param5";
    case Attribute::Param6:
        return "Param6";
    case Attribute::Param7:
        return "Param7";
    case Attribute::Param8:
        return "Param8";
    case Attribute::Param9:
        return "Param9";
    case Attribute::Param10:
        return "Param10";
    case Attribute::Param11:
        return "Param11";
    case Attribute::Param12:
        return "Param12";
    case Attribute::Param13:
        return "Param13";
    case Attribute::Param14:
        return "Param14";
    case Attribute::Param15:
        return "Param15";
    case Attribute::Param16:
        return "Param16";
    case Attribute::Param17:
        return "Param17";
    case Attribute::Param18:
        return "Param18";
    case Attribute::Param19:
        return "Param19";
    case Attribute::Param20:
        return "Param20";
    case Attribute::Param21:
        return "Param21";
    case Attribute::Param22:
        return "Param22";
    case Attribute::Param23:
        return "Param23";
    case Attribute::Param24:
        return "Param24";
    case Attribute::Param25:
        return "Param25";
    case Attribute::Param26:
        return "Param26";
    case Attribute::Param27:
        return "Param27";
    case Attribute::Param28:
        return "Param28";
    case Attribute::Param29:
        return "Param29";
    case Attribute::Param30:
        return "Param30";
    case Attribute::Param31:
        return "Param31";
    case Attribute::ClipDistance:
        return "ClipDistanace";
    case Attribute::CullDistance:
        return "CullDistance";
    case Attribute::RenderTargetId:
        return "RenderTargetId";
    case Attribute::ViewportId:
        return "ViewportId";
    case Attribute::VertexId:
        return "VertexId";
    case Attribute::PrimitiveId:
        return "PrimitiveId";
    case Attribute::InstanceId:
        return "InstanceId";
    case Attribute::IsFrontFace:
        return "IsFrontFace";
    case Attribute::SampleIndex:
        return "SampleIndex";
    case Attribute::GlobalInvocationId:
        return "GlobalInvocationId";
    case Attribute::WorkgroupId:
        return "WorkgroupId";
    case Attribute::WorkgroupIndex:
        return "WorkgroupIndex";
    case Attribute::LocalInvocationId:
        return "LocalInvocationId";
    case Attribute::LocalInvocationIndex:
        return "LocalInvocationIndex";
    case Attribute::FragCoord:
        return "FragCoord";
    case Attribute::InvocationId:
        return "InvocationId";
    case Attribute::PatchVertices:
        return "PatchVertices";
    case Attribute::TessellationEvaluationPointU:
        return "TessellationEvaluationPointU";
    case Attribute::TessellationEvaluationPointV:
        return "TessellationEvaluationPointV";
    case Attribute::PackedHullInvocationInfo:
        return "PackedHullInvocationInfo";
    default:
        break;
    }
    return fmt::format("<reserved attribute {}>", static_cast<int>(attribute));
}

} // namespace Shader::IR
