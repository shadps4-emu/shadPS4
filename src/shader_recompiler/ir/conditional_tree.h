// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/abstract_syntax_list.h"

namespace Shader::IR {

void AddConditionalTreeFromASL(AbstractSyntaxList& syntax_list);

} // namespace Shader::IR
