// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <initializer_list>
#include <map>
#include <span>
#include <vector>
#include <boost/intrusive/list.hpp>

#include "common/object_pool.h"
#include "common/types.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Gcn {
struct Block;
}

namespace Shader::IR {

class Block {
public:
    using InstructionList = boost::intrusive::list<Inst>;
    using size_type = InstructionList::size_type;
    using iterator = InstructionList::iterator;
    using const_iterator = InstructionList::const_iterator;
    using reverse_iterator = InstructionList::reverse_iterator;
    using const_reverse_iterator = InstructionList::const_reverse_iterator;

    explicit Block(Common::ObjectPool<Inst>& inst_pool_);
    ~Block();

    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    Block(Block&&) = default;
    Block& operator=(Block&&) = default;

    /// Appends a new instruction to the end of this basic block.
    void AppendNewInst(Opcode op, std::initializer_list<Value> args);

    /// Prepends a copy of an instruction to this basic block before the insertion point.
    iterator PrependNewInst(iterator insertion_point, const Inst& base_inst);

    /// Prepends a new instruction to this basic block before the insertion point.
    iterator PrependNewInst(iterator insertion_point, Opcode op,
                            std::initializer_list<Value> args = {}, u32 flags = 0);

    /// Adds a new branch to this basic block.
    void AddBranch(Block* block);

    /// Gets a mutable reference to the instruction list for this basic block.
    [[nodiscard]] InstructionList& Instructions() noexcept {
        return instructions;
    }
    /// Gets an immutable reference to the instruction list for this basic block.
    [[nodiscard]] const InstructionList& Instructions() const noexcept {
        return instructions;
    }

    /// Gets an immutable span to the immediate predecessors.
    [[nodiscard]] std::span<Block* const> ImmPredecessors() const noexcept {
        return imm_predecessors;
    }
    /// Gets an immutable span to the immediate successors.
    [[nodiscard]] std::span<Block* const> ImmSuccessors() const noexcept {
        return imm_successors;
    }

    /// Intrusively store the host definition of this instruction.
    template <typename T>
    void SetDefinition(T def) {
        definition = std::bit_cast<u32>(def);
    }

    /// Return the intrusively stored host definition of this instruction.
    template <typename T>
    [[nodiscard]] T Definition() const noexcept {
        return std::bit_cast<T>(definition);
    }

    void SsaSeal() noexcept {
        is_ssa_sealed = true;
    }
    [[nodiscard]] bool IsSsaSealed() const noexcept {
        return is_ssa_sealed;
    }

    [[nodiscard]] bool empty() const {
        return instructions.empty();
    }
    [[nodiscard]] size_type size() const {
        return instructions.size();
    }

    [[nodiscard]] Inst& front() {
        return instructions.front();
    }
    [[nodiscard]] const Inst& front() const {
        return instructions.front();
    }

    [[nodiscard]] Inst& back() {
        return instructions.back();
    }
    [[nodiscard]] const Inst& back() const {
        return instructions.back();
    }

    [[nodiscard]] iterator begin() {
        return instructions.begin();
    }
    [[nodiscard]] const_iterator begin() const {
        return instructions.cbegin();
    }
    [[nodiscard]] iterator end() {
        return instructions.end();
    }
    [[nodiscard]] const_iterator end() const {
        return instructions.cend();
    }

    [[nodiscard]] reverse_iterator rbegin() {
        return instructions.rbegin();
    }
    [[nodiscard]] const_reverse_iterator rbegin() const {
        return instructions.rbegin();
    }
    [[nodiscard]] reverse_iterator rend() {
        return instructions.rend();
    }
    [[nodiscard]] const_reverse_iterator rend() const {
        return instructions.rend();
    }

    [[nodiscard]] const_iterator cbegin() const {
        return instructions.cbegin();
    }
    [[nodiscard]] const_iterator cend() const {
        return instructions.cend();
    }

    [[nodiscard]] const_reverse_iterator crbegin() const {
        return instructions.crbegin();
    }
    [[nodiscard]] const_reverse_iterator crend() const {
        return instructions.crend();
    }

    /// Intrusively store the value of a register in the block.
    std::array<Value, NumScalarRegs> ssa_sreg_values;
    std::array<Value, NumScalarRegs> ssa_sbit_values;
    std::array<Value, NumVectorRegs> ssa_vreg_values;

    /// Block of the CFG that corresponds to this IR block.
    /// It can be null as IR has additional control flow blocks.
    const Shader::Gcn::Block* cfg_block{};

private:
    /// Memory pool for instruction list
    Common::ObjectPool<Inst>* inst_pool;

    /// List of instructions in this block
    InstructionList instructions;

    /// Block immediate predecessors
    std::vector<Block*> imm_predecessors;
    /// Block immediate successors
    std::vector<Block*> imm_successors;

    /// Intrusively store if the block is sealed in the SSA pass.
    bool is_ssa_sealed{false};

    /// Intrusively stored host definition of this block.
    u32 definition{};
};

using BlockList = std::vector<Block*>;

[[nodiscard]] std::string DumpBlock(const Block& block);

[[nodiscard]] std::string DumpBlock(const Block& block,
                                    const std::map<const Block*, size_t>& block_to_index,
                                    std::map<const Inst*, size_t>& inst_to_index,
                                    size_t& inst_index);

} // namespace Shader::IR
