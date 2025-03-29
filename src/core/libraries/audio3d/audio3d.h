// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <queue>

#include "common/types.h"
#include "core/libraries/audio/audioout.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3d {

using OrbisUserServiceUserId = s32;

enum OrbisAudio3dRate { ORBIS_AUDIO3D_RATE_48000 = 0 };

enum OrbisAudio3dBufferMode {
    ORBIS_AUDIO3D_BUFFER_NO_ADVANCE = 0,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_NO_PUSH = 1,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH = 2
};

struct OrbisAudio3dOpenParameters {
    size_t size_this;
    u32 granularity;
    OrbisAudio3dRate rate;
    u32 max_objects;
    u32 queue_depth;
    OrbisAudio3dBufferMode buffer_mode;
    int : 32;
    u32 num_beds;
};

enum OrbisAudio3dFormat { ORBIS_AUDIO3D_FORMAT_S16 = 0, ORBIS_AUDIO3D_FORMAT_FLOAT = 1 };

enum OrbisAudio3dOutputRoute {
    ORBIS_AUDIO3D_OUTPUT_BOTH = 0,
    ORBIS_AUDIO3D_OUTPUT_HMU_ONLY = 1,
    ORBIS_AUDIO3D_OUTPUT_TV_ONLY = 2
};

enum OrbisAudio3dBlocking { ORBIS_AUDIO3D_BLOCKING_ASYNC = 0, ORBIS_AUDIO3D_BLOCKING_SYNC = 1 };

struct OrbisAudio3dPcm {
    OrbisAudio3dFormat format;
    void* sample_buffer;
    u32 num_samples;
};

using OrbisAudio3dPortId = u32;
using OrbisAudio3dAttributeId = u32;
using OrbisAudio3dObjectId = u32;

struct OrbisAudio3dAttribute {
    OrbisAudio3dAttributeId attribute_id;
    int : 32;
    void* value;
    size_t value_size;
};

struct Port {
    OrbisAudio3dOpenParameters parameters{};
    std::deque<void*> queue; // Only stores PCM buffers for now
};

struct Audio3dState {
    std::unordered_map<OrbisAudio3dPortId, Port> ports;
};

int PS4_SYSV_ABI sceAudio3dAudioOutClose();
int PS4_SYSV_ABI sceAudio3dAudioOutOpen(OrbisAudio3dPortId port_id, OrbisUserServiceUserId user_id,
                                        s32 type, s32 index, u32 len, u32 freq,
                                        AudioOut::OrbisAudioOutParamExtendedInformation param);
int PS4_SYSV_ABI sceAudio3dAudioOutOutput(s32 handle, void* ptr);
int PS4_SYSV_ABI sceAudio3dAudioOutOutputs(AudioOut::OrbisAudioOutOutputParam* param, u32 num);
int PS4_SYSV_ABI sceAudio3dBedWrite(OrbisAudio3dPortId port_id, u32 num_channels,
                                    OrbisAudio3dFormat format, void* buffer, u32 num_samples);
int PS4_SYSV_ABI sceAudio3dBedWrite2(OrbisAudio3dPortId port_id, u32 num_channels,
                                     OrbisAudio3dFormat format, void* buffer, u32 num_samples,
                                     OrbisAudio3dOutputRoute output_route, bool restricted);
int PS4_SYSV_ABI sceAudio3dCreateSpeakerArray();
int PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray();
int PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters();
int PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize();
int PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients();
int PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients2();
int PS4_SYSV_ABI sceAudio3dInitialize(s64 reserved);
int PS4_SYSV_ABI sceAudio3dObjectReserve(OrbisAudio3dPortId port_id,
                                         OrbisAudio3dObjectId* object_id);
int PS4_SYSV_ABI sceAudio3dObjectSetAttributes(OrbisAudio3dPortId port_id,
                                               OrbisAudio3dObjectId object_id,
                                               size_t num_attributes,
                                               const OrbisAudio3dAttribute* attribute_array);
int PS4_SYSV_ABI sceAudio3dObjectUnreserve();
int PS4_SYSV_ABI sceAudio3dPortAdvance(OrbisAudio3dPortId port_id);
int PS4_SYSV_ABI sceAudio3dPortClose();
int PS4_SYSV_ABI sceAudio3dPortCreate();
int PS4_SYSV_ABI sceAudio3dPortDestroy();
int PS4_SYSV_ABI sceAudio3dPortFlush();
int PS4_SYSV_ABI sceAudio3dPortFreeState();
int PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported();
int PS4_SYSV_ABI sceAudio3dPortGetList();
int PS4_SYSV_ABI sceAudio3dPortGetParameters();
int PS4_SYSV_ABI sceAudio3dPortGetQueueLevel(OrbisAudio3dPortId port_id, u32* queue_level,
                                             u32* queue_available);
int PS4_SYSV_ABI sceAudio3dPortGetState();
int PS4_SYSV_ABI sceAudio3dPortGetStatus();
int PS4_SYSV_ABI sceAudio3dPortOpen(OrbisUserServiceUserId user_id,
                                    const OrbisAudio3dOpenParameters* parameters,
                                    OrbisAudio3dPortId* port_id);
int PS4_SYSV_ABI sceAudio3dPortPush(OrbisAudio3dPortId port_id, OrbisAudio3dBlocking blocking);
int PS4_SYSV_ABI sceAudio3dPortQueryDebug();
int PS4_SYSV_ABI sceAudio3dPortSetAttribute(OrbisAudio3dPortId port_id,
                                            OrbisAudio3dAttributeId attribute_id, void* attribute,
                                            size_t attribute_size);
int PS4_SYSV_ABI sceAudio3dReportRegisterHandler();
int PS4_SYSV_ABI sceAudio3dReportUnregisterHandler();
int PS4_SYSV_ABI sceAudio3dSetGpuRenderer();
int PS4_SYSV_ABI sceAudio3dStrError();
int PS4_SYSV_ABI sceAudio3dTerminate();

void RegisterlibSceAudio3d(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3d
