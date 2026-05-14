// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#ifdef _WIN64

struct _EXCEPTION_POINTERS;

namespace Core {

using VehHandlerFn = long (*)(_EXCEPTION_POINTERS* exp) noexcept;

// Prepares a dedicated stack for running vectored exception handler (VEH) work on the calling
// thread. Safe to call multiple times.
void InitializeVehStackForCurrentThread() noexcept;

// Releases any resources allocated by InitializeVehStackForCurrentThread() on the calling thread.
// Safe to call multiple times.
void CleanupVehStackForCurrentThread() noexcept;

// Runs the provided function on the calling thread's VEH stack (if available), otherwise executes
// it on the current stack.
long RunOnVehStack(VehHandlerFn fn, _EXCEPTION_POINTERS* exp) noexcept;

} // namespace Core

#endif
