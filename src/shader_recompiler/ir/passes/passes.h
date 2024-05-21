// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/basic_block.h"

namespace Shader::Optimization {

void SsaRewritePass(IR::BlockList& program);
void IdentityRemovalPass(IR::BlockList& program);
void DeadCodeEliminationPass(IR::BlockList& program);
void ConstantPropagationPass(IR::BlockList& program);
void ResourceTrackingPass(IR::BlockList& program);

} // namespace Shader::Optimization
