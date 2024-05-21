// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string>
#include "shader_recompiler/frontend/instruction.h"
#include "shader_recompiler/ir/abstract_syntax_list.h"
#include "shader_recompiler/ir/basic_block.h"

namespace Shader {
enum class Stage : u32;
}

namespace Shader::IR {

struct Program {
    AbstractSyntaxList syntax_list;
    BlockList blocks;
    BlockList post_order_blocks;
    std::vector<Gcn::GcnInst> ins_list;
    Stage stage;
};

[[nodiscard]] std::string DumpProgram(const Program& program);

} // namespace Shader::IR
