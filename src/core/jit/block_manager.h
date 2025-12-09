// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include "common/types.h"

namespace Core::Jit {

struct ExitFunctionLinkData {
    void* host_code;
    VAddr guest_rip;
    void* caller_address;
    u32 original_instruction;
};

using BlockDelinkerFunc = std::function<void(ExitFunctionLinkData*)>;

struct BlockLinkTag {
    VAddr guest_destination;
    ExitFunctionLinkData* host_link;

    bool operator<(const BlockLinkTag& other) const {
        if (guest_destination < other.guest_destination) {
            return true;
        } else if (guest_destination == other.guest_destination) {
            return host_link < other.host_link;
        } else {
            return false;
        }
    }
};

struct CodeBlock {
    VAddr ps4_address;
    void* arm64_code;
    size_t code_size;
    size_t instruction_count;
    std::set<VAddr> dependencies;
    bool is_linked;

    // Control flow targets for linking
    VAddr fallthrough_target;    // Next sequential address (if block doesn't end with branch)
    VAddr branch_target;         // Direct branch target (JMP)
    void* branch_patch_location; // Location in ARM64 code to patch for direct branch

    CodeBlock(VAddr addr, void* code, size_t size, size_t count)
        : ps4_address(addr), arm64_code(code), code_size(size), instruction_count(count),
          is_linked(false), fallthrough_target(0), branch_target(0),
          branch_patch_location(nullptr) {}
};

class BlockManager {
public:
    BlockManager();
    ~BlockManager();

    CodeBlock* GetBlock(VAddr ps4_address);
    CodeBlock* CreateBlock(VAddr ps4_address, void* arm64_code, size_t code_size,
                           size_t instruction_count);
    void InvalidateBlock(VAddr ps4_address);
    void InvalidateRange(VAddr start, VAddr end);
    void AddDependency(VAddr block_address, VAddr dependency);
    void AddBlockLink(VAddr guest_dest, ExitFunctionLinkData* link_data,
                      BlockDelinkerFunc delinker);
    void Clear();

    size_t GetBlockCount() const {
        return blocks.size();
    }
    size_t GetTotalCodeSize() const;

    std::unordered_map<VAddr, std::unique_ptr<CodeBlock>> blocks;
    std::map<BlockLinkTag, BlockDelinkerFunc> block_links;
    mutable std::mutex mutex;
};

} // namespace Core::Jit
