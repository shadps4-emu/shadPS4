// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "block_manager.h"
#include "common/logging/log.h"

namespace Core::Jit {

BlockManager::BlockManager() = default;

BlockManager::~BlockManager() {
    Clear();
}

CodeBlock* BlockManager::GetBlock(VAddr ps4_address) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = blocks.find(ps4_address);
    if (it != blocks.end()) {
        return it->second.get();
    }
    return nullptr;
}

CodeBlock* BlockManager::CreateBlock(VAddr ps4_address, void* arm64_code, size_t code_size,
                                     size_t instruction_count) {
    std::lock_guard<std::mutex> lock(mutex);

    auto block = std::make_unique<CodeBlock>(ps4_address, arm64_code, code_size, instruction_count);
    CodeBlock* result = block.get();
    blocks[ps4_address] = std::move(block);

    LOG_DEBUG(Core, "Created code block at PS4 address {:#x}, ARM64 code: {}, size: {}",
              ps4_address, arm64_code, code_size);

    return result;
}

void BlockManager::InvalidateBlock(VAddr ps4_address) {
    std::lock_guard<std::mutex> lock(mutex);

    // Delink all links pointing to this block
    auto lower = block_links.lower_bound({ps4_address, nullptr});
    auto upper = block_links.upper_bound(
        {ps4_address, reinterpret_cast<ExitFunctionLinkData*>(UINTPTR_MAX)});
    for (auto it = lower; it != upper;) {
        it->second(it->first.host_link);
        it = block_links.erase(it);
    }

    blocks.erase(ps4_address);
    LOG_DEBUG(Core, "Invalidated code block at PS4 address {:#x}", ps4_address);
}

void BlockManager::InvalidateRange(VAddr start, VAddr end) {
    std::lock_guard<std::mutex> lock(mutex);

    // Delink all links pointing to blocks in this range
    auto link_it = block_links.begin();
    while (link_it != block_links.end()) {
        if (link_it->first.guest_destination >= start && link_it->first.guest_destination < end) {
            link_it->second(link_it->first.host_link);
            link_it = block_links.erase(link_it);
        } else {
            ++link_it;
        }
    }

    auto it = blocks.begin();
    while (it != blocks.end()) {
        VAddr block_addr = it->first;
        if (block_addr >= start && block_addr < end) {
            it = blocks.erase(it);
        } else {
            auto& deps = it->second->dependencies;
            bool has_dependency_in_range = false;
            for (VAddr dep : deps) {
                if (dep >= start && dep < end) {
                    has_dependency_in_range = true;
                    break;
                }
            }
            if (has_dependency_in_range) {
                it = blocks.erase(it);
            } else {
                ++it;
            }
        }
    }

    LOG_DEBUG(Core, "Invalidated code blocks in range {:#x} - {:#x}", start, end);
}

void BlockManager::AddDependency(VAddr block_address, VAddr dependency) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = blocks.find(block_address);
    if (it != blocks.end()) {
        it->second->dependencies.insert(dependency);
    }
}

void BlockManager::AddBlockLink(VAddr guest_dest, ExitFunctionLinkData* link_data,
                                BlockDelinkerFunc delinker) {
    std::lock_guard<std::mutex> lock(mutex);
    block_links[{guest_dest, link_data}] = delinker;
}

void BlockManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    // Delink all links before clearing
    for (auto& [tag, delinker] : block_links) {
        delinker(tag.host_link);
    }
    block_links.clear();
    blocks.clear();
}

size_t BlockManager::GetTotalCodeSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    size_t total = 0;
    for (const auto& [addr, block] : blocks) {
        total += block->code_size;
    }
    return total;
}

} // namespace Core::Jit
