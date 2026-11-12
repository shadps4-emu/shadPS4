// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <vector>
#include <queue>

namespace Shader::IR {

template <typename Successors>
std::vector<size_t> TopologicalSort(size_t num_nodes, Successors&& successors) {
    std::vector<size_t> in_degree(num_nodes);
    for (size_t node = 0; node < num_nodes; ++node) {
        for (const size_t successor : successors(node)) {
            ++in_degree[successor];
        }
    }
    std::priority_queue<size_t, std::vector<size_t>, std::greater<>> ready;
    for (size_t node = 0; node < num_nodes; ++node) {
        if (in_degree[node] == 0) {
            ready.push(node);
        }
    }
    std::vector<size_t> order;
    order.reserve(num_nodes);
    std::vector<bool> sorted(num_nodes);
    size_t first_unsorted = 0;
    while (order.size() < num_nodes) {
        size_t node;
        if (ready.empty()) {
            while (sorted[first_unsorted]) {
                ++first_unsorted;
            }
            node = first_unsorted;
        } else {
            node = ready.top();
            ready.pop();
        }
        sorted[node] = true;
        order.push_back(node);
        for (const size_t successor : successors(node)) {
            if (--in_degree[successor] == 0 && !sorted[successor]) {
                ready.push(successor);
            }
        }
    }
    return order;
}

} // namespace Shader::IR
