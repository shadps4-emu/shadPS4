// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core {

/// Installs a host exception handler to handle guest TLS access.
void InstallTlsHandler();

/// Patches any instructions that access TLS to trigger the exception handler.
void PatchTLS(u64 segment_addr, u64 segment_size);

} // namespace Core
