// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <queue>
#include "common/types.h"
#include "core/libraries/audio/audioout.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3dOpenAL {

using OrbisAudio3dPortId = u32;
using OrbisAudio3dObjectId = u32;
using OrbisAudio3dAmbisonics = u32;
using OrbisAudio3dSpeakerArrayHandle = void*;

constexpr int ORBIS_AUDIO3D_OBJECT_INVALID = 0xFFFFFFFF;
constexpr OrbisAudio3dPortId ORBIS_AUDIO3D_PORT_INVALID = 0xFFFFFFFFu;
constexpr OrbisAudio3dPortId MaxPorts = 4;

struct OrbisAudio3dPosition {
    float x;
    float y;
    float z;
};

struct OrbisAudio3dSpeakerArrayParameters {
    OrbisAudio3dPosition* speaker_positions;
    u32 num_speakers; // max 16
    bool is_3d;
    void* buffer;
    u64 size;
};

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
    u32 _pad;
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

enum class OrbisAudio3dPortAttributeId : u32 {
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_LATE_REVERB_LEVEL = 0x10001,
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_RADIUS = 0x10002,
    ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_HEIGHT_AWARE = 0x10003,
};

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

enum class OrbisAudio3dPassthrough : u32 {
    ORBIS_AUDIO3D_PASSTHROUGH_NONE = 0,
    ORBIS_AUDIO3D_PASSTHROUGH_LEFT = 1,
    ORBIS_AUDIO3D_PASSTHROUGH_RIGHT = 2,
};

struct SpatialSource {
    u32 source{0};
    std::vector<u32> buffers;   // ALuint ring, sized queue_depth + slack
    std::vector<u32> available; // reclaimed / never-queued buffer ids
};

struct ObjectState {
    std::deque<AudioData> pcm_queue;
    std::unordered_map<u32, std::vector<u8>> persistent_attributes;
    SpatialSource al;
    bool unreserved{false};
};

struct SpatialObjectFrame {
    OrbisAudio3dObjectId object_id{};
    AudioData pcm{};
    float gain{0.0f};
    OrbisAudio3dPosition position{0.0f, 0.0f, 0.0f};
    bool has_position{false};
    float spread{0.0f};
    OrbisAudio3dPassthrough passthrough{OrbisAudio3dPassthrough::ORBIS_AUDIO3D_PASSTHROUGH_NONE};
};

struct SpatialFrameBundle {
    AudioData bed{};
    std::vector<SpatialObjectFrame> objects;
};

struct AssociatedAudioOutPort {
    s32 handle{-1};
    u32 buffer_bytes{0};
    u32 samples_per_buffer{0};
    bool is_float{false};
    std::deque<std::vector<u8>> pending;
};

struct Port {
    mutable std::recursive_mutex mutex;
    OrbisAudio3dOpenParameters parameters{};
    s32 audio_out_handle{-1};
    std::vector<AssociatedAudioOutPort> audioout_ports;
    std::unordered_map<OrbisAudio3dObjectId, ObjectState> objects;
    OrbisAudio3dObjectId next_object_id{0};
    std::deque<AudioData> bed_queue;
    std::deque<AudioData> mixed_queue;
    std::deque<SpatialFrameBundle> spatial_queue;

    // Spatial (direct OpenAL) output state.
    bool spatial_init_attempted{false};
    bool spatial_ready{false};
    bool source_radius_supported{false};
    bool direct_channels_supported{false};
    std::string device_name;
    u64 period_us{0};
    u64 last_volume_check_us{0};
    float current_gain{-1.0f};
    SpatialSource bed;
    u32 bed_channels{2};
    std::vector<s16> spatial_scratch;
    std::vector<s16> spatial_scratch_stereo;
    // EFX late reverb
    bool reverb_supported{false};
    u32 reverb_slot{0};
    u32 reverb_effect{0};
    float late_reverb_level{0.0f};
    float downmix_spread_radius{2.0f};
};

struct SpeakerArray {
    std::vector<OrbisAudio3dPosition> speakers;
    bool is_3d{false};
};

struct Audio3dState {
    std::mutex ports_mutex;
    std::unordered_map<OrbisAudio3dPortId, Port> ports;

    std::mutex speaker_arrays_mutex;
    std::unordered_map<u64, SpeakerArray> speaker_arrays;
    u64 next_speaker_array_id{1};
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
s32 PS4_SYSV_ABI sceAudio3dCreateSpeakerArray(OrbisAudio3dSpeakerArrayHandle* handle,
                                              const OrbisAudio3dSpeakerArrayParameters* parameters);
s32 PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray(OrbisAudio3dSpeakerArrayHandle handle);
s32 PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* params);
u64 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize(u32 num_speakers, bool is_3d);
s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients(OrbisAudio3dSpeakerArrayHandle handle,
                                                          OrbisAudio3dPosition pos, float spread,
                                                          float* coefficients,
                                                          u32 num_coefficients);
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
s32 PS4_SYSV_ABI sceAudio3dPortCreate(u32 granularity, u32 rate, s64 reserved,
                                      OrbisAudio3dPortId* port_id);
s32 PS4_SYSV_ABI sceAudio3dPortDestroy();
s32 PS4_SYSV_ABI sceAudio3dPortFlush(OrbisAudio3dPortId port_id);
s32 PS4_SYSV_ABI sceAudio3dPortFreeState();
s32 PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported(OrbisAudio3dPortId port_id,
                                                      OrbisAudio3dAttributeId* capabilities,
                                                      u32* num_capabilities);
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
                                            OrbisAudio3dPortAttributeId attribute_id,
                                            void* attribute, u64 attribute_size);
s32 PS4_SYSV_ABI sceAudio3dReportRegisterHandler();
s32 PS4_SYSV_ABI sceAudio3dReportUnregisterHandler();
s32 PS4_SYSV_ABI sceAudio3dSetGpuRenderer();
s32 PS4_SYSV_ABI sceAudio3dStrError();
s32 PS4_SYSV_ABI sceAudio3dTerminate();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3dOpenAL
