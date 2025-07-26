// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include <string>
#include <boost/container/small_vector.hpp>
#include <boost/intrusive/set.hpp>

#include "common/assert.h"
#include "common/object_pool.h"
#include "common/types.h"
#include "shader_recompiler/frontend/instruction.h"
#include "shader_recompiler/ir/condition.h"

namespace Shader::Gcn {

using Hook =
    boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>;

enum class EndClass {
    Branch, ///< Block ends with a (un)conditional branch.
    Exit,   ///< Block ends with an exit instruction.
};

/// A block represents a linear range of instructions.
struct Block : Hook {
    [[nodiscard]] bool Contains(u32 pc) const noexcept;

    bool operator<(const Block& rhs) const noexcept {
        return begin < rhs.begin;
    }

    u32 begin;
    u32 end;
    u32 begin_index;
    u32 end_index;
    IR::Condition cond{};
    GcnInst end_inst{};
    EndClass end_class{};
    Block* branch_true{};
    Block* branch_false{};
    bool is_dummy{};
};

class CFG {
    using Label = u32;

public:
    explicit CFG(Common::ObjectPool<Block>& block_pool, std::span<const GcnInst> inst_list);

    [[nodiscard]] std::string Dot() const;

private:
    void EmitLabels();
    void EmitBlocks();
    void LinkBlocks();
    void SplitDivergenceScopes();

    void AddLabel(Label address) {
        const auto it = std::ranges::find(labels, address);
        if (it == labels.end()) {
            labels.push_back(address);
        }
    };

    size_t GetIndex(Label label) {
        if (label == 0) {
            return 0ULL;
        }
        const auto it_index = std::ranges::lower_bound(index_to_pc, label);
        ASSERT(it_index != index_to_pc.end() || label > index_to_pc.back());
        return std::distance(index_to_pc.begin(), it_index);
    };

public:
    Common::ObjectPool<Block>& block_pool;
    std::span<const GcnInst> inst_list;
    std::vector<u32> index_to_pc;
    boost::container::small_vector<Label, 16> labels;
    boost::intrusive::set<Block> blocks;
};

} // namespace Shader::Gcn
