// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Xbyak {
class CodeGenerator;
}

namespace Core {

/// Sets the data pointer that contains the TLS image.
void SetTLSStorage(u64 image_address);

/// Patches any instructions that access guest TLS to use provided storage.
void PatchTLS(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c);

} // namespace Core
