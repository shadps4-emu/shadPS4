// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include "common/types.h"

/// Retrieves the PKG entry name from its type identifier.
std::string_view GetEntryNameByType(u32 type);
