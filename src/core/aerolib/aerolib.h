// SPDX-FileCopyrightText: Copyright 2026 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

namespace Core::AeroLib {

struct NidEntry {
    const char* nid;
    const char* name;
};

const NidEntry* FindByNid(const char* nid);

} // namespace Core::AeroLib
