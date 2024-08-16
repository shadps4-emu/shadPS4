// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <type_traits>
#include <boost/container/small_vector.hpp>
#include <queue>
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

template <typename Pred>
auto BreadthFirstSearch(const Inst* inst, Pred&& pred) -> std::invoke_result_t<Pred, const Inst*> {
    // Most often case the instruction is the desired already.
    if (const std::optional result = pred(inst)) {
        return result;
    }

    // Breadth-first search visiting the right most arguments first
    boost::container::small_vector<const Inst*, 2> visited;
    std::queue<const Inst*> queue;
    queue.push(inst);

    while (!queue.empty()) {
        // Pop one instruction from the queue
        const Inst* const inst{queue.front()};
        queue.pop();
        if (const std::optional result = pred(inst)) {
            // This is the instruction we were looking for
            return result;
        }
        // Visit the right most arguments first
        for (size_t arg = inst->NumArgs(); arg--;) {
            const Value arg_value{inst->Arg(arg)};
            if (arg_value.IsImmediate()) {
                continue;
            }
            // Queue instruction if it hasn't been visited
            const Inst* const arg_inst{arg_value.InstRecursive()};
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    // SSA tree has been traversed and the result hasn't been found
    return std::nullopt;
}

template <typename Pred>
auto BreadthFirstSearch(const Value& value, Pred&& pred)
    -> std::invoke_result_t<Pred, const Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return BreadthFirstSearch(value.InstRecursive(), pred);
}

} // namespace Shader::IR
