// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/hack_features.h"

namespace Common {

bool HackFeatures::isTheOrder1886 = false;
bool HackFeatures::initialized = false;

void HackFeatures::Init(std::string_view game_serial) {
    ASSERT_MSG(!initialized, "HackFeatures::Init called more than once");
    initialized = true;

    // CUSA00035 / CUSA00076 / CUSA00100 are different regional/language
    // releases of The Order: 1886.
    if (game_serial == "CUSA00035" || game_serial == "CUSA00076" ||
        game_serial == "CUSA00100") {
        isTheOrder1886 = true;
    }
}

} // namespace Common
