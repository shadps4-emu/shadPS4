// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

void EmitLoadSharedU32(EmitContext& ctx) {
    throw NotImplementedException("LoadSharedU32");
}

void EmitLoadSharedU64(EmitContext& ctx) {
    throw NotImplementedException("LoadSharedU64");
}

void EmitWriteSharedU32(EmitContext& ctx) {
    throw NotImplementedException("WriteSharedU32");
}

void EmitWriteSharedU64(EmitContext& ctx) {
    throw NotImplementedException("WriteSharedU64");
}
}