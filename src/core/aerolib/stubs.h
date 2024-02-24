// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::AeroLib {

u64 UnresolvedStub();

u64 GetStub(const char* nid);

} // namespace Core::AeroLib
