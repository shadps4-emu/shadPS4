// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2Pan;

struct OrbisNgs2PanParam {
    float angle;
    float distance;
    float fbwLevel;
    float lfeLevel;
};

struct OrbisNgs2PanWork {
    float aSpeakerAngle[ORBIS_NGS2_MAX_VOICE_CHANNELS];
    float unitAngle;
    u32 numSpeakers;
};

} // namespace Libraries::Ngs2
