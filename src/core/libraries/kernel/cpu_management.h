// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI sceKernelIsNeoMode();

int PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(int* ver);

} // namespace Libraries::Kernel
