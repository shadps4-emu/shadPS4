// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Xbyak {
class CodeGenerator;
}

namespace Core {

/// Initializes a stack for the current thread for use by patch implementations.
void InitializeThreadPatchStack();

/// Cleans up the patch stack for the current thread.
void CleanupThreadPatchStack();

/// Patches CPU instructions that cannot run as-is on the host.
void PatchInstructions(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c);

} // namespace Core
