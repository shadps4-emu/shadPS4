// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <stddef.h>

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3d {

class Audio3d;

typedef int OrbisUserServiceUserId;
typedef unsigned int OrbisAudio3dPortId;
typedef unsigned int OrbisAudio3dObjectId;
typedef unsigned int OrbisAudio3dAttributeId;

enum OrbisAudio3dFormat {
    ORBIS_AUDIO3D_FORMAT_S16 = 0,  // s16
    ORBIS_AUDIO3D_FORMAT_FLOAT = 1 // f32
};

enum OrbisAudio3dRate { ORBIS_AUDIO3D_RATE_48000 = 0 };

enum OrbisAudio3dBufferMode {
    ORBIS_AUDIO3D_BUFFER_NO_ADVANCE = 0,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_NO_PUSH = 1,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH = 2
};

enum OrbisAudio3dBlocking { ORBIS_AUDIO3D_BLOCKING_ASYNC = 0, ORBIS_AUDIO3D_BLOCKING_SYNC = 1 };

enum OrbisAudio3dPassthrough {
    ORBIS_AUDIO3D_PASSTHROUGH_NONE = 0,
    ORBIS_AUDIO3D_PASSTHROUGH_LEFT = 1,
    ORBIS_AUDIO3D_PASSTHROUGH_RIGHT = 2
};

enum OrbisAudio3dOutputRoute {
    ORBIS_AUDIO3D_OUTPUT_BOTH = 0,
    ORBIS_AUDIO3D_OUTPUT_HMU_ONLY = 1,
    ORBIS_AUDIO3D_OUTPUT_TV_ONLY = 2
};

enum OrbisAudio3dAmbisonics {
    ORBIS_AUDIO3D_AMBISONICS_NONE = ~0,
    ORBIS_AUDIO3D_AMBISONICS_W = 0,
    ORBIS_AUDIO3D_AMBISONICS_X = 1,
    ORBIS_AUDIO3D_AMBISONICS_Y = 2,
    ORBIS_AUDIO3D_AMBISONICS_Z = 3,
    ORBIS_AUDIO3D_AMBISONICS_R = 4,
    ORBIS_AUDIO3D_AMBISONICS_S = 5,
    ORBIS_AUDIO3D_AMBISONICS_T = 6,
    ORBIS_AUDIO3D_AMBISONICS_U = 7,
    ORBIS_AUDIO3D_AMBISONICS_V = 8,
    ORBIS_AUDIO3D_AMBISONICS_K = 9,
    ORBIS_AUDIO3D_AMBISONICS_L = 10,
    ORBIS_AUDIO3D_AMBISONICS_M = 11,
    ORBIS_AUDIO3D_AMBISONICS_N = 12,
    ORBIS_AUDIO3D_AMBISONICS_O = 13,
    ORBIS_AUDIO3D_AMBISONICS_P = 14,
    ORBIS_AUDIO3D_AMBISONICS_Q = 15
};

static const OrbisAudio3dAttributeId s_sceAudio3dAttributePcm = 0x00000001;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributePriority = 0x00000002;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributePosition = 0x00000003;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeSpread = 0x00000004;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeGain = 0x00000005;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributePassthrough = 0x00000006;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeResetState = 0x00000007;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeApplicationSpecific = 0x00000008;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeAmbisonics = 0x00000009;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeRestricted = 0x0000000A;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeOutputRoute = 0x0000000B;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeLateReverbLevel = 0x00010001;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeDownmixSpreadRadius = 0x00010002;
static const OrbisAudio3dAttributeId s_sceAudio3dAttributeDownmixSpreadHeightAware = 0x00010003;

struct OrbisAudio3dSpeakerArray;
using OrbisAudio3dSpeakerArrayHandle = OrbisAudio3dSpeakerArray*; // head

struct OrbisAudio3dOpenParameters {
    size_t szSizeThis;
    unsigned int uiGranularity;
    OrbisAudio3dRate eRate;
    unsigned int uiMaxObjects;
    unsigned int uiQueueDepth;
    OrbisAudio3dBufferMode eBufferMode;
    char padding[32];
    unsigned int uiNumBeds;
};

struct OrbisAudio3dAttribute {
    OrbisAudio3dAttributeId uiAttributeId;
    char padding[32];
    const void* pValue;
    size_t szValue;
};

struct OrbisAudio3dPosition {
    float fX;
    float fY;
    float fZ;
};

struct OrbisAudio3dPcm {
    OrbisAudio3dFormat eFormat;
    const void* pSampleBuffer;
    unsigned int uiNumSamples;
};

struct OrbisAudio3dSpeakerArrayParameters {
    OrbisAudio3dPosition* pSpeakerPosition;
    unsigned int uiNumSpeakers;
    bool bIs3d;
    void* pBuffer;
    size_t szSize;
};

struct OrbisAudio3dApplicationSpecific {
    size_t szSizeThis;
    u8 cApplicationSpecific[32];
};

void PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* sParameters);

void RegisterlibSceAudio3d(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3d