// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void SsaRewritePass(IR::BlockList& program);
void IdentityRemovalPass(IR::BlockList& program);
void DeadCodeEliminationPass(IR::Program& program);
void ConstantPropagationPass(IR::BlockList& program);
void FlattenExtendedUserdataPass(IR::Program& program);
void ResourceTrackingPass(IR::Program& program);
void CollectShaderInfoPass(IR::Program& program);
void LowerSharedMemToRegisters(IR::Program& program);
void RingAccessElimination(const IR::Program& program, const RuntimeInfo& runtime_info,
                           Stage stage);

} // namespace Shader::Optimization
