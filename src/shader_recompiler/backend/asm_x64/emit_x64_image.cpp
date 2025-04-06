// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

void EmitImageSampleRaw(EmitContext& ctx) {
    // We can reach this here. We done resource tracking pass yet.
    throw NotImplementedException("ImageSampleRaw");
}

void EmitImageSampleImplicitLod(EmitContext& ctx) {
    throw NotImplementedException("ImageSampleImplicitLod");
}

void EmitImageSampleExplicitLod(EmitContext& ctx) {
    throw NotImplementedException("ImageSampleExplicitLod");
}

void EmitImageSampleDrefImplicitLod(EmitContext& ctx) {
    throw NotImplementedException("ImageSampleDrefImplicitLod");
}

void EmitImageSampleDrefExplicitLod(EmitContext& ctx) {
    throw NotImplementedException("ImageSampleDrefExplicitLod");
}

void EmitImageGather(EmitContext& ctx) {
    throw NotImplementedException("ImageGather");
}

void EmitImageGatherDref(EmitContext& ctx) {
    throw NotImplementedException("ImageGatherDref");
}

void EmitImageQueryDimensions(EmitContext& ctx) {
    throw NotImplementedException("ImageQueryDimensions");
}

void EmitImageQueryLod(EmitContext& ctx) {
    throw NotImplementedException("ImageQueryLod");
}

void EmitImageGradient(EmitContext& ctx) {
    throw NotImplementedException("ImageGradient");
}

void EmitImageRead(EmitContext& ctx) {
    throw NotImplementedException("ImageRead");
}

void EmitImageWrite(EmitContext& ctx) {
    throw NotImplementedException("ImageWrite");
}

void EmitCubeFaceIndex(EmitContext& ctx) {
    throw NotImplementedException("CubeFaceIndex");
}

} // namespace Shader::Backend::X64