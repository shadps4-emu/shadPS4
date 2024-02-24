// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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
