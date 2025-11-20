// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <string>

#include <fmt/format.h>

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

void DumpProgram(const Program& program, const Info& info, const std::string& type) {
    using namespace Common::FS;

    if (!Config::dumpShaders()) {
        return;
    }

    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto ir_filename =
        fmt::format("{}_{:#018x}.{}irprogram.txt", info.stage, info.pgm_hash, type);
    const auto ir_file = IOFile{dump_dir / ir_filename, FileAccessMode::Create, FileType::TextFile};

    size_t index{0};
    std::map<const IR::Inst*, size_t> inst_to_index;
    std::map<const IR::Block*, size_t> block_to_index;

    for (const IR::Block* const block : program.blocks) {
        block_to_index.emplace(block, index);
        ++index;
    }

    for (const auto& block : program.blocks) {
        std::string s = IR::DumpBlock(*block, block_to_index, inst_to_index, index) + '\n';
        ir_file.WriteString(s);
    }

    const auto asl_filename = fmt::format("{}_{:#018x}.{}asl.txt", info.stage, info.pgm_hash, type);
    const auto asl_file =
        IOFile{dump_dir / asl_filename, FileAccessMode::Create, FileType::TextFile};

    for (const auto& node : program.syntax_list) {
        std::string s = IR::DumpASLNode(node, block_to_index, inst_to_index) + '\n';
        asl_file.WriteString(s);
    }
}

} // namespace Shader::IR
