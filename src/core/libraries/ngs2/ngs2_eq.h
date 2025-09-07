// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2Eq;

struct OrbisNgs2EqVoiceSetupParam {
    u32 numChannels;
};

struct OrbisNgs2EqVoiceFilterParam {
    u32 type;
    u32 channelMask;
    union {
        struct {
            float i0;
            float i1;
            float i2;
            float o1;
            float o2;
        } direct;
        struct {
            float fc;
            float q;
            float level;
            u32 reserved;
            u32 reserved2;
        } fcq;
    } param;
};

struct OrbisNgs2EqVoiceState {
    u32 stateFlags;
};

} // namespace Libraries::Ngs2
