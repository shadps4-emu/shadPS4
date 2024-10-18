// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/ir/abstract_syntax_list.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/value.h"

namespace Shader {
struct Info;
struct Profile;
struct RuntimeInfo;
} // namespace Shader

namespace Shader::Gcn {

[[nodiscard]] IR::AbstractSyntaxList BuildASL(Common::ObjectPool<IR::Inst>& inst_pool,
                                              Common::ObjectPool<IR::Block>& block_pool, CFG& cfg,
                                              Info& info, const RuntimeInfo& runtime_info,
                                              const Profile& profile);

} // namespace Shader::Gcn
