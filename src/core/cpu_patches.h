// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "common/types.h"

namespace Core {

class MemoryManager;

struct CodeRange {
    u64 address;
    u64 size;
};

/// Registers a module for patching, providing an area to generate trampoline code.
void RegisterPatchModule(MemoryManager* memory, void* module_ptr, u64 module_size,
                         void* trampoline_area_ptr, u64 trampoline_area_size);

/// Applies CPU patches that need to be done before beginning executions.
void PrePatchInstructions(std::span<const CodeRange> ranges);

} // namespace Core
