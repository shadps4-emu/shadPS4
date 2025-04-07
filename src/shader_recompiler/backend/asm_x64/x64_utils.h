// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/ir/type.h"

namespace Shader::Backend::X64 {

bool IsFloatingType(const IR::Value& value);
size_t GetRegBytesOfType(const IR::Value& value);
u8 GetNumComponentsOfType(const IR::Value& value);
Xbyak::Reg ResizeRegToType(const Xbyak::Reg& reg, const IR::Value& value);
void MovFloat(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovDouble(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovGP(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src);
void MovValue(EmitContext& ctx, const Operands& dst, const IR::Value& src);
void EmitInlineF16ToF32(EmitContext& ctx, const Xbyak::Operand& dest, const Xbyak::Operand& src);
void EmitInlineF32ToF16(EmitContext& ctx, const Xbyak::Operand& dest, const Xbyak::Operand& src);

inline bool IsFloatingType(IR::Inst* inst) {
    return IsFloatingType(IR::Value(inst));
}

inline size_t GetRegBytesOfType(IR::Inst* inst) {
    return GetRegBytesOfType(IR::Value(inst));
}

inline u8 GetNumComponentsOfType(IR::Inst* inst) {
    return GetNumComponentsOfType(IR::Value(inst));
}

inline Xbyak::Reg ResizeRegToType(const Xbyak::Reg& reg, IR::Inst* inst) {
    return ResizeRegToType(reg, IR::Value(inst));
}

} // namespace Shader::Backend::X64