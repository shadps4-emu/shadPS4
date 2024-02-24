// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/types.h"
#include "core/aerolib/aerolib.h"

namespace Core::AeroLib {

// Use a direct table here + binary search as contents are static
static constexpr NidEntry NIDS[] = {
#define STUB(nid, name) {nid, #name},
#include "aerolib.inl"
#undef STUB
};

const NidEntry* FindByNid(const char* nid) {
    s64 l = 0;
    s64 r = sizeof(NIDS) / sizeof(NIDS[0]) - 1;

    while (l <= r) {
        const size_t m = l + (r - l) / 2;
        const int cmp = std::strcmp(NIDS[m].nid, nid);
        if (cmp == 0) {
            return &NIDS[m];
        } else if (cmp < 0) {
            l = m + 1;
        } else {
            r = m - 1;
        }
    }
    return nullptr;
}

} // namespace Core::AeroLib
