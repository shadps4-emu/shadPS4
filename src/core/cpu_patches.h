// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "common/types.h"

// ============================================================================
// Windows static guest red-zone protection
// ============================================================================

enum class WindowsGuestRedZoneProtectionMode : u32 {
    Disabled,
    StaticPatching,
};

namespace Core::WindowsGuestRedZoneProtection {

void SetActiveMode(WindowsGuestRedZoneProtectionMode mode) noexcept;
WindowsGuestRedZoneProtectionMode GetActiveMode() noexcept;
bool IsStaticPatchingEnabled() noexcept;

} // namespace Core::WindowsGuestRedZoneProtection

// ============================================================================
// End Windows static guest red-zone protection
// ============================================================================

namespace Core {

// Windows static guest red-zone protection
struct RedZonePatchResult {
    u64 function_count{};
    u64 instruction_count{};
    u64 red_zone_function_count{};
    u64 memory_instruction_count{};
    u64 short_memory_instruction_count{};
    u64 patched_memory_instruction_count{};
    u64 stack_dependent_memory_instruction_count{};
    u64 control_flow_memory_instruction_count{};
    u64 unrelocatable_memory_instruction_count{};
    u64 indirect_red_zone_function_count{};
    u64 cpu_patch_instruction_count{};
    u64 patched_cpu_patch_instruction_count{};
    u64 unsupported_cpu_patch_instruction_count{};
    u64 inplace_cpu_patch_instruction_count{};
    u64 uncovered_inplace_cpu_patch_instruction_count{};
    u64 uncovered_unsupported_cpu_patch_instruction_count{};

    RedZonePatchResult& operator+=(const RedZonePatchResult& other) {
        function_count += other.function_count;
        instruction_count += other.instruction_count;
        red_zone_function_count += other.red_zone_function_count;
        memory_instruction_count += other.memory_instruction_count;
        short_memory_instruction_count += other.short_memory_instruction_count;
        patched_memory_instruction_count += other.patched_memory_instruction_count;
        stack_dependent_memory_instruction_count += other.stack_dependent_memory_instruction_count;
        control_flow_memory_instruction_count += other.control_flow_memory_instruction_count;
        unrelocatable_memory_instruction_count += other.unrelocatable_memory_instruction_count;
        indirect_red_zone_function_count += other.indirect_red_zone_function_count;
        cpu_patch_instruction_count += other.cpu_patch_instruction_count;
        patched_cpu_patch_instruction_count += other.patched_cpu_patch_instruction_count;
        unsupported_cpu_patch_instruction_count += other.unsupported_cpu_patch_instruction_count;
        inplace_cpu_patch_instruction_count += other.inplace_cpu_patch_instruction_count;
        uncovered_inplace_cpu_patch_instruction_count +=
            other.uncovered_inplace_cpu_patch_instruction_count;
        uncovered_unsupported_cpu_patch_instruction_count +=
            other.uncovered_unsupported_cpu_patch_instruction_count;
        return *this;
    }
};

/// Registers a module for patching, providing an area to generate trampoline code.
void RegisterPatchModule(void* module_ptr, u64 module_size, void* trampoline_area_ptr,
                         u64 trampoline_area_size);

/// Applies CPU patches that need to be done before beginning executions.
void PrePatchInstructions(u64 segment_addr, u64 segment_size);

// Windows static guest red-zone protection
/// Keeps Windows exception dispatch outside live guest red zones at faultable memory accesses.
RedZonePatchResult PatchRedZoneMemoryInstructions(u64 segment_addr, u64 segment_size,
                                                  std::span<const uintptr_t> function_starts);

/// Applies CPU patches ahead of time to the functions listed in the EH frame search table. A patch
/// site too short for a jump is relocated together with its neighboring instructions. Between
/// those functions, only the SSE4a instructions long enough for a jump are patched.
RedZonePatchResult PatchCpuInstructionsStatically(u64 segment_addr, u64 segment_size,
                                                  std::span<const uintptr_t> function_starts);

} // namespace Core
