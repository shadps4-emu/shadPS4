// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/ir/type.h"

namespace Shader::Backend::X64 {

bool IsFloatingType(IR::Type type);
bool IsConditionalOpcode(IR::Opcode opcode);
size_t GetRegBytesOfType(IR::Type type);
u8 GetNumComponentsOfType(IR::Type type);
Xbyak::Reg ResizeRegToType(const Xbyak::Reg& reg, IR::Type type);
void MovFloat(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovDouble(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovGP(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovValue(EmitContext& ctx, const Operands& dst, const IR::Value& src);
void EmitInlineF16ToF32(EmitContext& ctx, const Xbyak::Operand& dest, const Xbyak::Operand& src);
void EmitInlineF32ToF16(EmitContext& ctx, const Xbyak::Operand& dest, const Xbyak::Operand& src);

} // namespace Shader::Backend::X64