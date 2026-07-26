// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <optional>
#include "common/types.h"

namespace Core::WindowsFaultTracker {

enum class MemoryAccess : u8 {
    Read,
    Write,
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

/// Returns whether this process is using the external Windows fault monitor.
bool IsEnabled() noexcept;

/**
 * Installs the debuggee-side fault callback. The monitor redirects a faulting thread to an
 * alternate-stack trampoline before Windows exception dispatch, so the callback runs on the
 * original thread without touching its guest stack.
 */
void InstallFaultHandler(std::function<bool(VAddr, u64, MemoryAccess)> callback);

/// Removes the callback before its target is destroyed.
void RemoveFaultHandler();

/**
 * Routes first-chance illegal-instruction exceptions through the alternate-stack trampoline before
 * normal Windows exception dispatch. The trampoline invokes the registered SignalDispatch
 * handlers using the captured guest context.
 */
void InstallIllegalInstructionHandler();

/// Adds or removes watches for every 4 KiB page touching the range.
void WatchMemory(VAddr address, u64 size, MemoryAccess access, bool enable) noexcept;

} // namespace Core::WindowsFaultTracker
