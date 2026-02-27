// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <vector>

#include "common/types.h"
#include "core/libraries/audio/audioout.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3dOpenAL {

constexpr int ORBIS_AUDIO3D_OBJECT_INVALID = 0xFFFFFFFF;

enum class OrbisAudio3dRate : u32 {
    ORBIS_AUDIO3D_RATE_48000 = 0,
};

enum class OrbisAudio3dBufferMode : u32 {
    ORBIS_AUDIO3D_BUFFER_NO_ADVANCE = 0,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_NO_PUSH = 1,
    ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH = 2,
};

struct OrbisAudio3dOpenParameters {
    u64 size_this;
    u32 granularity;
    OrbisAudio3dRate rate;
    u32 max_objects;
    u32 queue_depth;
    OrbisAudio3dBufferMode buffer_mode;
    int : 32;
    u32 num_beds;
};

enum class OrbisAudio3dFormat : u32 {
    ORBIS_AUDIO3D_FORMAT_S16 = 0,
    ORBIS_AUDIO3D_FORMAT_FLOAT = 1,
};

enum class OrbisAudio3dOutputRoute : u32 {
    ORBIS_AUDIO3D_OUTPUT_BOTH = 0,
    ORBIS_AUDIO3D_OUTPUT_HMU_ONLY = 1,
    ORBIS_AUDIO3D_OUTPUT_TV_ONLY = 2,
};

enum class OrbisAudio3dBlocking : u32 {
    ORBIS_AUDIO3D_BLOCKING_ASYNC = 0,
    ORBIS_AUDIO3D_BLOCKING_SYNC = 1,
};

struct OrbisAudio3dPcm {
    OrbisAudio3dFormat format;
    void* sample_buffer;
    u32 num_samples;
};

enum class OrbisAudio3dAttributeId : u32 {
    ORBIS_AUDIO3D_ATTRIBUTE_PCM = 1,
    ORBIS_AUDIO3D_ATTRIBUTE_POSITION = 2,
    ORBIS_AUDIO3D_ATTRIBUTE_GAIN = 3,
    ORBIS_AUDIO3D_ATTRIBUTE_SPREAD = 4,
    ORBIS_AUDIO3D_ATTRIBUTE_PRIORITY = 5,
    ORBIS_AUDIO3D_ATTRIBUTE_PASSTHROUGH = 6,
    ORBIS_AUDIO3D_ATTRIBUTE_AMBISONICS = 7,
    ORBIS_AUDIO3D_ATTRIBUTE_APPLICATION_SPECIFIC = 8,
    ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE = 9,
    ORBIS_AUDIO3D_ATTRIBUTE_RESTRICTED = 10,
    ORBIS_AUDIO3D_ATTRIBUTE_OUTPUT_ROUTE = 11,
};

using OrbisAudio3dPortId = u32;
using OrbisAudio3dObjectId = u32;
using OrbisAudio3dAmbisonics = u32;

struct OrbisAudio3dAttribute {
    OrbisAudio3dAttributeId attribute_id;
    int : 32;
    void* value;
    u64 value_size;
};

struct AudioData {
    u8* sample_buffer;
    u32 num_samples;
    u32 num_channels{1};
    OrbisAudio3dFormat format{OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16};
};

struct ObjectState {
    std::deque<AudioData> pcm_queue;
    std::unordered_map<u32, std::vector<u8>> persistent_attributes;
};

struct Port {
    mutable std::recursive_mutex mutex;
    OrbisAudio3dOpenParameters parameters{};
    // Opened lazily on the first sceAudio3dPortPush call.
    s32 audio_out_handle{-1};
    // Handles explicitly opened by the game via sceAudio3dAudioOutOpen.
    std::vector<s32> audioout_handles;
    // Reserved objects and their state.
    std::unordered_map<OrbisAudio3dObjectId, ObjectState> objects;
    // Increasing counter for generating unique object IDs within this port.
    OrbisAudio3dObjectId next_object_id{0};
    // Bed audio queue.
    std::deque<AudioData> bed_queue;
    // Mixed stereo frames ready to be consumed by sceAudio3dPortPush.
    std::deque<AudioData> mixed_queue;
};

struct Audio3dState {
    std::unordered_map<OrbisAudio3dPortId, Port> ports;
};

s32 PS4_SYSV_ABI sceAudio3dAudioOutClose(s32 handle);
s32 PS4_SYSV_ABI sceAudio3dAudioOutOpen(OrbisAudio3dPortId port_id,
                                        Libraries::UserService::OrbisUserServiceUserId user_id,
                                        s32 type, s32 index, u32 len, u32 freq,
                                        AudioOut::OrbisAudioOutParamExtendedInformation param);
s32 PS4_SYSV_ABI sceAudio3dAudioOutOutput(s32 handle, void* ptr);
s32 PS4_SYSV_ABI sceAudio3dAudioOutOutputs(AudioOut::OrbisAudioOutOutputParam* param, u32 num);
s32 PS4_SYSV_ABI sceAudio3dBedWrite(OrbisAudio3dPortId port_id, u32 num_channels,
                                    OrbisAudio3dFormat format, void* buffer, u32 num_samples);
s32 PS4_SYSV_ABI sceAudio3dBedWrite2(OrbisAudio3dPortId port_id, u32 num_channels,
                                     OrbisAudio3dFormat format, void* buffer, u32 num_samples,
                                     OrbisAudio3dOutputRoute output_route, bool restricted);
s32 PS4_SYSV_ABI sceAudio3dCreateSpeakerArray();
s32 PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray();
s32 PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* params);
s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize();
s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients();
s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients2();
s32 PS4_SYSV_ABI sceAudio3dInitialize(s64 reserved);
s32 PS4_SYSV_ABI sceAudio3dObjectReserve(OrbisAudio3dPortId port_id,
                                         OrbisAudio3dObjectId* object_id);
s32 PS4_SYSV_ABI sceAudio3dObjectSetAttribute(OrbisAudio3dPortId port_id,
                                              OrbisAudio3dObjectId object_id,
                                              OrbisAudio3dAttributeId attribute_id,
                                              const void* attribute, u64 attribute_size);
s32 PS4_SYSV_ABI sceAudio3dObjectSetAttributes(OrbisAudio3dPortId port_id,
                                               OrbisAudio3dObjectId object_id, u64 num_attributes,
                                               const OrbisAudio3dAttribute* attribute_array);
s32 PS4_SYSV_ABI sceAudio3dObjectUnreserve(OrbisAudio3dPortId port_id,
                                           OrbisAudio3dObjectId object_id);
s32 PS4_SYSV_ABI sceAudio3dPortAdvance(OrbisAudio3dPortId port_id);
s32 PS4_SYSV_ABI sceAudio3dPortClose(OrbisAudio3dPortId port_id);
s32 PS4_SYSV_ABI sceAudio3dPortCreate();
s32 PS4_SYSV_ABI sceAudio3dPortDestroy();
s32 PS4_SYSV_ABI sceAudio3dPortFlush(OrbisAudio3dPortId port_id);
s32 PS4_SYSV_ABI sceAudio3dPortFreeState();
s32 PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported();
s32 PS4_SYSV_ABI sceAudio3dPortGetList();
s32 PS4_SYSV_ABI sceAudio3dPortGetParameters();
s32 PS4_SYSV_ABI sceAudio3dPortGetQueueLevel(OrbisAudio3dPortId port_id, u32* queue_level,
                                             u32* queue_available);
s32 PS4_SYSV_ABI sceAudio3dPortGetState();
s32 PS4_SYSV_ABI sceAudio3dPortGetStatus();
s32 PS4_SYSV_ABI sceAudio3dPortOpen(Libraries::UserService::OrbisUserServiceUserId user_id,
                                    const OrbisAudio3dOpenParameters* parameters,
                                    OrbisAudio3dPortId* port_id);
s32 PS4_SYSV_ABI sceAudio3dPortPush(OrbisAudio3dPortId port_id, OrbisAudio3dBlocking blocking);
s32 PS4_SYSV_ABI sceAudio3dPortQueryDebug();
s32 PS4_SYSV_ABI sceAudio3dPortSetAttribute(OrbisAudio3dPortId port_id,
                                            OrbisAudio3dAttributeId attribute_id, void* attribute,
                                            u64 attribute_size);
s32 PS4_SYSV_ABI sceAudio3dReportRegisterHandler();
s32 PS4_SYSV_ABI sceAudio3dReportUnregisterHandler();
s32 PS4_SYSV_ABI sceAudio3dSetGpuRenderer();
s32 PS4_SYSV_ABI sceAudio3dStrError();
s32 PS4_SYSV_ABI sceAudio3dTerminate();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3dOpenAL
