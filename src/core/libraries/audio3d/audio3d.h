// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include <queue>

#include <al.h>
#include "common/types.h"
#include "core/libraries/audio/audioout.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3d {

constexpr int ORBIS_AUDIO3D_PORT_INVALID = 0xFFFFFFFF;

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
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PCM = 0x00000001,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PRIORITY = 0x00000002,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_POSITION = 0x00000003,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_SPREAD = 0x00000004,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_GAIN = 0x00000005,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PASSTHROUGH = 0x00000006,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_RESET_STATE = 0x00000007,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_APPLICATION_SPECIFIC = 0x00000008,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_AMBISONICS = 0x00000009,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_RESTRICTED = 0x0000000A,
    ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_OUTPUT_ROUTE = 0x0000000B,
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_LATE_REVERB_LEVEL = 0x00010001,
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_RADIUS = 0x00010002,
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_HEIGHT_AWARE = 0x00010003
};

using OrbisAudio3dPortId = u32;
using OrbisAudio3dObjectId = u32;

struct OrbisAudio3dAttribute {
    OrbisAudio3dAttributeId attribute_id;
    int : 32;
    void* value;
    u64 value_size;
};

// OpenAL-specific structures for 3D audio
struct OpenAL3dSource {
    ALuint source_id = 0;
    ALuint buffer_id = 0;
    bool active = false;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    float gain = 1.0f;
    float pitch = 1.0f;
    float reference_distance = 1.0f;
    float max_distance = 100.0f;
    float rolloff_factor = 1.0f;
    float cone_inner_angle = 360.0f;
    float cone_outer_angle = 360.0f;
    float cone_outer_gain = 0.0f;
    bool looping = false;
};

// Simplified audio data - always stereo S16
struct AudioData {
    std::vector<std::byte> sample_buffer; // Always stereo S16 format
    u32 num_samples;                      // Number of stereo samples
};

struct Audio3dObject {
    bool in_use = false;
    bool active = false;
    OpenAL3dSource al_source;
    std::deque<AudioData> pcm_queue; // Stereo S16 audio data
    bool passthrough = false;      
    float spread = 360.0f;
    u32 priority = 0;            // 0 = lowest priority, higher values = higher priority
    bool priority_valid = false; // Whether priority has been set
};

struct Port {
    OrbisAudio3dOpenParameters parameters{};
    std::deque<AudioData> queue; // Stereo S16 audio data
    std::optional<AudioData> current_buffer{};
    std::vector<Audio3dObject> objects;
    std::mutex lock;

    // OpenAL listener for this port
    float listener_position[3] = {0.0f, 0.0f, 0.0f};
    float listener_velocity[3] = {0.0f, 0.0f, 0.0f};
    float listener_orientation[6] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
};

struct Audio3dState {
    std::unordered_map<OrbisAudio3dPortId, Port> ports;
    s32 audio_out_handle;
};

// Function declarations
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
s32 PS4_SYSV_ABI sceAudio3dObjectSetAttributes(OrbisAudio3dPortId port_id,
                                               OrbisAudio3dObjectId object_id, u64 num_attributes,
                                               const OrbisAudio3dAttribute* attribute_array);
s32 PS4_SYSV_ABI sceAudio3dObjectUnreserve(OrbisAudio3dPortId port_id,
                                           OrbisAudio3dObjectId object_id);
s32 PS4_SYSV_ABI sceAudio3dPortAdvance(OrbisAudio3dPortId port_id);
s32 PS4_SYSV_ABI sceAudio3dPortClose();
s32 PS4_SYSV_ABI sceAudio3dPortCreate();
s32 PS4_SYSV_ABI sceAudio3dPortDestroy();
s32 PS4_SYSV_ABI sceAudio3dPortFlush();
s32 PS4_SYSV_ABI sceAudio3dPortFreeState();
s32 PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported(OrbisAudio3dPortId portId,
                                                      OrbisAudio3dAttributeId* caps, u32* numCaps);
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
} // namespace Libraries::Audio3d