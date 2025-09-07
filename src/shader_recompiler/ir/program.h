// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "shader_recompiler/frontend/instruction.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/abstract_syntax_list.h"
#include "shader_recompiler/ir/basic_block.h"

namespace Shader::IR {

struct Program {
    explicit Program(Info& info_) : info{info_} {}

    AbstractSyntaxList syntax_list;
    BlockList blocks;
    BlockList post_order_blocks;
    std::vector<Gcn::GcnInst> ins_list;
    Info& info;
};

void DumpProgram(const Program& program, const Info& info, const std::string& type = "");

} // namespace Shader::IR
