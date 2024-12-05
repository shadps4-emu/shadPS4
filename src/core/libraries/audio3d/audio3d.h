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

using OrbisUserServiceUserId = s32;
using OrbisAudio3dPortId = u32;
using OrbisAudio3dObjectId = u32;
using OrbisAudio3dAttributeId = u32;

enum class OrbisAudio3dFormat {
    S16 = 0,
    Float = 1,
};

enum class OrbisAudio3dRate {
    Rate48000 = 0,
};

enum class OrbisAudio3dBufferMode { NoAdvance = 0, AdvanceNoPush = 1, AdvanceAndPush = 2 };

enum class OrbisAudio3dBlocking {
    Async = 0,
    Sync = 1,
};

enum class OrbisAudio3dPassthrough {
    None = 0,
    Left = 1,
    Right = 2,
};

enum class OrbisAudio3dOutputRoute {
    Both = 0,
    HmuOnly = 1,
    TvOnly = 2,
};

enum class OrbisAudio3dAmbisonics : u32 {
    None = ~0U,
    W = 0,
    X = 1,
    Y = 2,
    Z = 3,
    R = 4,
    S = 5,
    T = 6,
    U = 7,
    V = 8,
    K = 9,
    L = 10,
    M = 11,
    N = 12,
    O = 13,
    P = 14,
    Q = 15
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
    size_t size_this;
    u32 granularity;
    OrbisAudio3dRate rate;
    u32 max_objects;
    u32 queue_depth;
    OrbisAudio3dBufferMode buffer_mode;
    char padding[32];
    u32 num_beds;
};

struct OrbisAudio3dAttribute {
    OrbisAudio3dAttributeId attribute_id;
    char padding[32];
    const void* p_value;
    size_t value;
};

struct OrbisAudio3dPosition {
    float fX;
    float fY;
    float fZ;
};

struct OrbisAudio3dPcm {
    OrbisAudio3dFormat format;
    const void* sample_buffer;
    u32 num_samples;
};

struct OrbisAudio3dSpeakerArrayParameters {
    OrbisAudio3dPosition* speaker_position;
    u32 num_speakers;
    bool is_3d;
    void* buffer;
    size_t size;
};

struct OrbisAudio3dApplicationSpecific {
    size_t size_this;
    u8 application_specific[32];
};

void PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* sParameters);

void RegisterlibSceAudio3d(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3d
