// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <optional>
#include <span>
#include "common/types.h"

namespace Core::WindowsFaultTracker {

enum class MemoryAccess : u8 {
    Read,
    Write,
};

enum class WatchAction : u8 {
    Add,
    Remove,
};

/**
 * On Windows, starts a small child monitor which attaches to the emulator before memory tracking
 * begins. First-chance debug events arrive before Windows builds a user-mode exception frame on
 * the guest stack, so tracked accesses and CPU patch exceptions do not destroy System V red-zone
 * data.
 *
 * The normal emulator process retains its PID and window ownership. The monitor returns from main
 * immediately after its debug loop ends.
 */
std::optional<int> Bootstrap(int argc, char* argv[]);

/// Returns whether this process is using the child-process Windows fault monitor.
bool IsEnabled() noexcept;

/**
 * Installs the debuggee-side fault callbacks. Read and mixed-access faults are redirected to an
 * alternate-stack trampoline before Windows exception dispatch. Write-only faults can instead be
 * made writable by the monitor and deferred until the GPU thread consumes guest memory. Neither
 * path writes to the guest stack.
 */
void InstallFaultHandler(std::function<bool(VAddr, u64, MemoryAccess)> callback,
                         std::function<void(std::span<const VAddr>)> deferred_write_callback,
                         u64 write_granularity);

/// Removes the callback before its target is destroyed.
void RemoveFaultHandler();

/// Applies write-only faults deferred by the monitor before GPU commands consume guest memory.
void DrainPendingWrites();

/// Returns whether the monitor has deferred writes awaiting GPU-thread invalidation.
bool HasPendingWrites() noexcept;

/**
 * Routes first-chance illegal-instruction exceptions through the alternate-stack trampoline before
 * normal Windows exception dispatch. The trampoline invokes the registered SignalDispatch
 * handlers using the captured guest context.
 */
void InstallIllegalInstructionHandler();

/// Adds or removes watches for every 4 KiB page touching the range.
void WatchMemory(VAddr address, u64 size, MemoryAccess access, WatchAction action) noexcept;

} // namespace Core::WindowsFaultTracker
