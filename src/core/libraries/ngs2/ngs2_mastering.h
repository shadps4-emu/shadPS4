// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2Mastering;

struct OrbisNgs2MasteringRackOption {
    OrbisNgs2RackOption rackOption;
    u32 maxChannels;
    u32 numPeakMeterBlocks;
};

struct OrbisNgs2MasteringVoiceSetupParam {
    OrbisNgs2VoiceParamHeader header;

    u32 numInputChannels;
    u32 flags;
};

struct OrbisNgs2MasteringVoiceMatrixParam {
    OrbisNgs2VoiceParamHeader header;

    u32 type;
    u32 numLevels;
    const float* aLevel;
};

struct OrbisNgs2MasteringVoiceLfeParam {
    OrbisNgs2VoiceParamHeader header;

    u32 enableFlag;
    u32 fc;
};

struct OrbisNgs2MasteringVoiceLimiterParam {
    OrbisNgs2VoiceParamHeader header;

    u32 enableFlag;
    float threshold;
};

struct OrbisNgs2MasteringVoiceGainParam {
    OrbisNgs2VoiceParamHeader header;

    float fbwLevel;
    float lfeLevel;
};

struct OrbisNgs2MasteringVoiceOutputParam {
    OrbisNgs2VoiceParamHeader header;

    u32 outputId;
    u32 reserved;
};

struct OrbisNgs2MasteringVoicePeakMeterParam {
    OrbisNgs2VoiceParamHeader header;
    u32 enableFlag;
    u32 reserved;
};

struct OrbisNgs2MasteringVoiceState {
    OrbisNgs2VoiceState voiceState;
    float limiterPeakLevel;
    float limiterPressLevel;
    float aInputPeakHeight[ORBIS_NGS2_MAX_VOICE_CHANNELS];
    float aOutputPeakHeight[ORBIS_NGS2_MAX_VOICE_CHANNELS];
};

struct OrbisNgs2MasteringRackInfo {
    OrbisNgs2RackInfo rackInfo;
    u32 maxChannels;
    u32 reserved;
};

} // namespace Libraries::Ngs2
