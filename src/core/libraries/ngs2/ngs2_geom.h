// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2Geom;

struct OrbisNgs2GeomVector {
    float x;
    float y;
    float z;
};

struct OrbisNgs2GeomCone {
    float innerLevel;
    float innerAngle;
    float outerLevel;
    float outerAngle;
};

struct OrbisNgs2GeomRolloff {
    u32 model;
    float maxDistance;
    float rolloffFactor;
    float referenceDistance;
};

struct OrbisNgs2GeomListenerParam {
    OrbisNgs2GeomVector position;
    OrbisNgs2GeomVector orientFront;
    OrbisNgs2GeomVector orientUp;
    OrbisNgs2GeomVector velocity;
    float soundSpeed;
    u32 reserved[2];
};

struct OrbisNgs2GeomListenerWork {
    float matrix[4][4];
    OrbisNgs2GeomVector velocity;
    float soundSpeed;
    u32 coordinate;
    u32 reserved[3];
};

struct OrbisNgs2GeomSourceParam {
    OrbisNgs2GeomVector position;
    OrbisNgs2GeomVector velocity;
    OrbisNgs2GeomVector direction;
    OrbisNgs2GeomCone cone;
    OrbisNgs2GeomRolloff rolloff;
    float dopplerFactor;
    float fbwLevel;
    float lfeLevel;
    float maxLevel;
    float minLevel;
    float radius;
    u32 numSpeakers;
    u32 matrixFormat;
    u32 reserved[2];
};

struct OrbisNgs2GeomA3dAttribute {
    OrbisNgs2GeomVector position;
    float volume;
    u32 reserved[4];
};

struct OrbisNgs2GeomAttribute {
    float pitchRatio;
    float aLevel[ORBIS_NGS2_MAX_VOICE_CHANNELS * ORBIS_NGS2_MAX_VOICE_CHANNELS];

    OrbisNgs2GeomA3dAttribute a3dAttrib;
    u32 reserved[4];
};

} // namespace Libraries::Ngs2
