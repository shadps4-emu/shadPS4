// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <AL/al.h>
#include <AL/alc.h>
#include <magic_enum/magic_enum.hpp>

#include <iostream>
#include <alext.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_error.h"
#include "core/libraries/audio/openal_manager.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/audio3d/audio3d_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Audio3d {

static constexpr u32 AUDIO3D_SAMPLE_RATE = 48000;
static constexpr AudioOut::OrbisAudioOutParamFormat AUDIO3D_OUTPUT_FORMAT =
    AudioOut::OrbisAudioOutParamFormat::S16Stereo;
static constexpr u32 AUDIO3D_OUTPUT_NUM_CHANNELS = 2;
static constexpr u32 AUDIO3D_OUTPUT_BUFFER_FRAMES = 0x100;

static std::unique_ptr<Audio3dState> state;

static bool Audio3dHRTFActive() {
    auto& al = Libraries::AudioOut::OpenALManager::Instance();
    return al.IsInitialized() && al.IsHRTFEnabled();
}

// Convert PS4 3D coordinate system to OpenAL
static void ConvertPositionToOpenAL(const float* ps4_pos, float* al_pos) {
    al_pos[0] = ps4_pos[0];  // X same
    al_pos[1] = ps4_pos[1];  // Y same
    al_pos[2] = -ps4_pos[2]; // Z inverted
}

// Helper function to downmix 8-channel audio to stereo S16
static std::vector<std::byte> Downmix8ChannelToStereoS16(const OrbisAudio3dPcm& pcm) {
    std::vector<std::byte> converted_data;

    const size_t output_size = pcm.num_samples * 2 * sizeof(int16_t);
    converted_data.resize(output_size);
    int16_t* output = reinterpret_cast<int16_t*>(converted_data.data());

    if (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
        const int16_t* input = static_cast<const int16_t*>(pcm.sample_buffer);

        for (u32 i = 0; i < pcm.num_samples; i++) {
            size_t base_idx = i * 8;

            // Convert to float for processing
            float fl = input[base_idx + 0] / 32768.0f;
            float fr = input[base_idx + 1] / 32768.0f;
            float fc = input[base_idx + 2] / 32768.0f;
            float lfe = input[base_idx + 3] / 32768.0f;
            float bl = input[base_idx + 4] / 32768.0f;
            float br = input[base_idx + 5] / 32768.0f;
            float sl = input[base_idx + 6] / 32768.0f;
            float sr = input[base_idx + 7] / 32768.0f;

            // Downmix with volume reduction
            float left = fl * 0.7f + fc * 0.5f + sl * 0.5f + bl * 0.3f + lfe * 0.1f;
            float right = fr * 0.7f + fc * 0.5f + sr * 0.5f + br * 0.3f + lfe * 0.1f;

            // Clamp and convert back to int16
            left = std::clamp(left, -1.0f, 1.0f);
            right = std::clamp(right, -1.0f, 1.0f);

            output[i * 2] = static_cast<int16_t>(left * 32767.0f);
            output[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
        }
        LOG_DEBUG(Lib_Audio3d, "Downmixed 8-channel S16 to stereo: {} samples", pcm.num_samples);
    } else if (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        const float* input = static_cast<const float*>(pcm.sample_buffer);

        for (u32 i = 0; i < pcm.num_samples; i++) {
            size_t base_idx = i * 8;

            float fl = input[base_idx + 0];
            float fr = input[base_idx + 1];
            float fc = input[base_idx + 2];
            float lfe = input[base_idx + 3];
            float bl = input[base_idx + 4];
            float br = input[base_idx + 5];
            float sl = input[base_idx + 6];
            float sr = input[base_idx + 7];

            // 7.1 to stereo downmix
            float left = fl * 0.7f + fc * 0.5f + sl * 0.5f + bl * 0.3f + lfe * 0.1f;
            float right = fr * 0.7f + fc * 0.5f + sr * 0.5f + br * 0.3f + lfe * 0.1f;

            // Clamp and convert to S16
            left = std::clamp(left, -1.0f, 1.0f);
            right = std::clamp(right, -1.0f, 1.0f);

            output[i * 2] = static_cast<int16_t>(left * 32767.0f);
            output[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
        }
        LOG_DEBUG(Lib_Audio3d, "Downmixed 8-channel FLOAT to stereo S16: {} samples",
                  pcm.num_samples);
    } else {
        LOG_ERROR(Lib_Audio3d, "Unsupported format for downmixing: {}",
                  magic_enum::enum_name(pcm.format));
        return {};
    }

    return converted_data;
}
// Convert any input to stereo S16 format
static std::vector<std::byte> ConvertToStereoS16(const OrbisAudio3dPcm& pcm, u32 num_channels) {
    std::vector<std::byte> converted_data;

    if (!pcm.sample_buffer || pcm.num_samples == 0) {
        LOG_ERROR(Lib_Audio3d, "Empty or invalid PCM data");
        return converted_data;
    }

    const size_t output_size = pcm.num_samples * 2 * sizeof(int16_t);
    converted_data.resize(output_size);
    int16_t* output = reinterpret_cast<int16_t*>(converted_data.data());

    if (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
        // Handle S16 format
        if (num_channels == 2) {
            // Direct copy for stereo S16
            const size_t input_size = pcm.num_samples * 2 * sizeof(int16_t);
            std::memcpy(output, pcm.sample_buffer, input_size);
            LOG_DEBUG(Lib_Audio3d, "Direct copy stereo S16: {} samples", pcm.num_samples);
        } else if (num_channels == 1) {
            // Convert mono S16 to stereo S16 (duplicate channels)
            const int16_t* input = static_cast<const int16_t*>(pcm.sample_buffer);
            for (u32 i = 0; i < pcm.num_samples; i++) {
                output[i * 2] = input[i];     // Left channel
                output[i * 2 + 1] = input[i]; // Right channel
            }
            LOG_DEBUG(Lib_Audio3d, "Converted mono S16 to stereo: {} samples", pcm.num_samples);
        } else if (num_channels == 8) {
            // Downmix 8-channel S16 to stereo S16
            converted_data = Downmix8ChannelToStereoS16(pcm);
            LOG_DEBUG(Lib_Audio3d, "Downmixed 8-channel S16 to stereo: {} samples",
                      pcm.num_samples);
        } else {
            LOG_ERROR(Lib_Audio3d, "Unsupported channel count for S16: {} (only 1, 2 or 8)",
                      num_channels);
            return {};
        }
    } else if (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        // Handle FLOAT format (convert to S16)
        if (num_channels == 2) {
            // Convert stereo FLOAT to stereo S16
            const float* input = static_cast<const float*>(pcm.sample_buffer);
            for (u32 i = 0; i < pcm.num_samples * 2; i++) {
                // Clamp float to [-1.0, 1.0] and convert to int16
                float sample = std::clamp(input[i], -1.0f, 1.0f);
                output[i] = static_cast<int16_t>(sample * 32767.0f);
            }
            LOG_DEBUG(Lib_Audio3d, "Converted stereo FLOAT to stereo S16: {} samples",
                      pcm.num_samples);
        } else if (num_channels == 1) {
            // Convert mono FLOAT to stereo S16
            const float* input = static_cast<const float*>(pcm.sample_buffer);
            for (u32 i = 0; i < pcm.num_samples; i++) {
                float sample = std::clamp(input[i], -1.0f, 1.0f);
                int16_t s16_sample = static_cast<int16_t>(sample * 32767.0f);
                output[i * 2] = s16_sample;     // Left channel
                output[i * 2 + 1] = s16_sample; // Right channel
            }
            LOG_DEBUG(Lib_Audio3d, "Converted mono FLOAT to stereo S16: {} samples",
                      pcm.num_samples);
        } else if (num_channels == 8) {
            // Convert and downmix 8-channel FLOAT to stereo S16
            const float* input = static_cast<const float*>(pcm.sample_buffer);
            for (u32 i = 0; i < pcm.num_samples; i++) {
                size_t base_idx = i * 8;

                float fl = input[base_idx + 0];
                float fr = input[base_idx + 1];
                float fc = input[base_idx + 2];
                float lfe = input[base_idx + 3];
                float bl = input[base_idx + 4];
                float br = input[base_idx + 5];
                float sl = input[base_idx + 6];
                float sr = input[base_idx + 7];

                // 7.1 to stereo downmix
                float left = fl * 0.7f + fc * 0.5f + sl * 0.5f + bl * 0.3f + lfe * 0.1f;
                float right = fr * 0.7f + fc * 0.5f + sr * 0.5f + br * 0.3f + lfe * 0.1f;

                // Clamp and convert to S16
                left = std::clamp(left, -1.0f, 1.0f);
                right = std::clamp(right, -1.0f, 1.0f);

                output[i * 2] = static_cast<int16_t>(left * 32767.0f);
                output[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
            }
            LOG_DEBUG(Lib_Audio3d, "Converted 8-channel FLOAT to stereo S16: {} samples",
                      pcm.num_samples);
        } else {
            LOG_ERROR(Lib_Audio3d, "Unsupported channel count for FLOAT: {} (only 1, 2 or 8)",
                      num_channels);
            return {};
        }
    } else {
        LOG_ERROR(Lib_Audio3d, "Unsupported audio format: {}", magic_enum::enum_name(pcm.format));
        return {};
    }

    return converted_data;
}

// Queue audio data (always converted to stereo S16)
static s32 PortQueueAudio(Port& port, const OrbisAudio3dPcm& pcm, const u32 num_channels) {
    LOG_DEBUG(Lib_Audio3d, "PortQueueAudio: channels={}, samples={}, format={}", num_channels,
              pcm.num_samples, magic_enum::enum_name(pcm.format));

    if (!pcm.sample_buffer || pcm.num_samples == 0) {
        LOG_ERROR(Lib_Audio3d, "Invalid PCM data for queue");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    // Convert to stereo S16
    auto converted_data = ConvertToStereoS16(pcm, num_channels);

    if (converted_data.empty()) {
        LOG_ERROR(Lib_Audio3d, "Failed to convert audio to stereo S16");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    // Verify size matches expected stereo S16
    const size_t expected_size = pcm.num_samples * 2 * sizeof(int16_t);
    if (converted_data.size() != expected_size) {
        LOG_ERROR(Lib_Audio3d,
                  "Converted size mismatch: got {} bytes, expected {} bytes ({} samples)",
                  converted_data.size(), expected_size, pcm.num_samples);
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    AudioData audio_data{
        .sample_buffer = std::move(converted_data),
        .num_samples = pcm.num_samples,
    };

    std::lock_guard lock(port.lock);
    port.queue.emplace_back(std::move(audio_data));

    LOG_DEBUG(Lib_Audio3d, "Queued stereo S16 audio: {} bytes, {} samples, from {} channels",
              audio_data.sample_buffer.size(), audio_data.num_samples, num_channels);

    return ORBIS_OK;
}

// Clean up processed OpenAL buffers
static void CleanupProcessedBuffers(Audio3dObject& obj) {
    if (obj.al_source.source_id == 0)
        return;

    Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();

    ALint processed = 0;
    alGetSourcei(obj.al_source.source_id, AL_BUFFERS_PROCESSED, &processed);

    if (processed > 0) {
        std::vector<ALuint> buffers(processed);
        alSourceUnqueueBuffers(obj.al_source.source_id, processed, buffers.data());
        alDeleteBuffers(processed, buffers.data());
    }
}

// Stream object audio (always stereo S16)
static void StreamObjectAudio(Audio3dObject& obj) {
    if (!obj.in_use || !obj.active)
        return;

    // Clean up processed buffers first
    CleanupProcessedBuffers(obj);

    // Ensure OpenAL source exists
    if (obj.al_source.source_id == 0) {
        alGenSources(1, &obj.al_source.source_id);
        if (obj.al_source.source_id == 0) {
            LOG_ERROR(Lib_Audio3d, "Failed to generate OpenAL source");
            return;
        }

        // Set default OpenAL source properties
        Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
        alSourcef(obj.al_source.source_id, AL_PITCH, obj.al_source.pitch);
        alSourcef(obj.al_source.source_id, AL_GAIN, obj.al_source.gain);
        alSource3f(obj.al_source.source_id, AL_POSITION, obj.al_source.position[0],
                   obj.al_source.position[1], obj.al_source.position[2]);
        alSource3f(obj.al_source.source_id, AL_VELOCITY, obj.al_source.velocity[0],
                   obj.al_source.velocity[1], obj.al_source.velocity[2]);
        alSourcef(obj.al_source.source_id, AL_REFERENCE_DISTANCE, obj.al_source.reference_distance);
        alSourcef(obj.al_source.source_id, AL_MAX_DISTANCE, obj.al_source.max_distance);
        alSourcef(obj.al_source.source_id, AL_ROLLOFF_FACTOR, obj.al_source.rolloff_factor);
        alSourcei(obj.al_source.source_id, AL_LOOPING, obj.al_source.looping ? AL_TRUE : AL_FALSE);
        alSourcei(obj.al_source.source_id, AL_SOURCE_RELATIVE, AL_FALSE);
    }

    // Check if there is audio to stream
    if (obj.pcm_queue.empty())
        return;

    auto& data = obj.pcm_queue.front();

    if (data.sample_buffer.empty()) {
        LOG_WARNING(Lib_Audio3d, "Empty audio buffer in queue");
        obj.pcm_queue.pop_front();
        return;
    }

    // Generate OpenAL buffer
    ALuint buffer = 0;
    alGenBuffers(1, &buffer);

    if (buffer == 0) {
        LOG_ERROR(Lib_Audio3d, "Failed to generate OpenAL buffer");
        obj.pcm_queue.pop_front();
        return;
    }

    // Always use stereo S16 format (AL_FORMAT_STEREO16 = 0x1103)
    ALenum al_format = AL_FORMAT_STEREO16;

    // Upload data to buffer
    alBufferData(buffer, al_format, data.sample_buffer.data(),
                 static_cast<ALsizei>(data.sample_buffer.size()), AUDIO3D_SAMPLE_RATE);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG_ERROR(Lib_Audio3d, "Failed to upload buffer data: {}", error);
        alDeleteBuffers(1, &buffer);
        obj.pcm_queue.pop_front();
        return;
    }

    // Queue buffer to source
    alSourceQueueBuffers(obj.al_source.source_id, 1, &buffer);

    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG_ERROR(Lib_Audio3d, "Failed to queue buffer: {}", error);
        alDeleteBuffers(1, &buffer);
        obj.pcm_queue.pop_front();
        return;
    }

    // Check if source is playing
    ALint state;
    alGetSourcei(obj.al_source.source_id, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(obj.al_source.source_id);
        error = alGetError();
        if (error != AL_NO_ERROR) {
            LOG_ERROR(Lib_Audio3d, "Failed to play source: {}", error);
        }
    }

    // Remove from queue
    obj.pcm_queue.pop_front();
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOutput(const s32 handle, void* ptr) {
    LOG_DEBUG(Lib_Audio3d, "called, handle = {}, ptr = {}", handle, ptr);

    if (!ptr) {
        LOG_ERROR(Lib_Audio3d, "!ptr");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (handle < 0 || (handle & 0xFFFF) > 25) {
        LOG_ERROR(Lib_Audio3d, "handle < 0 || (handle & 0xFFFF) > 25");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    return AudioOut::sceAudioOutOutput(handle, ptr);
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOutputs(AudioOut::OrbisAudioOutOutputParam* param,
                                           const u32 num) {
    LOG_DEBUG(Lib_Audio3d, "called, param = {}, num = {}", static_cast<void*>(param), num);

    if (!param || !num) {
        LOG_ERROR(Lib_Audio3d, "!param || !num");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    return AudioOut::sceAudioOutOutputs(param, num);
}

s32 PS4_SYSV_ABI sceAudio3dBedWrite(const OrbisAudio3dPortId port_id, const u32 num_channels,
                                    const OrbisAudio3dFormat format, void* buffer,
                                    const u32 num_samples) {
    return sceAudio3dBedWrite2(port_id, num_channels, format, buffer, num_samples,
                               OrbisAudio3dOutputRoute::ORBIS_AUDIO3D_OUTPUT_BOTH, false);
}

s32 PS4_SYSV_ABI sceAudio3dBedWrite2(const OrbisAudio3dPortId port_id, const u32 num_channels,
                                     const OrbisAudio3dFormat format, void* buffer,
                                     const u32 num_samples,
                                     const OrbisAudio3dOutputRoute output_route,
                                     const bool restricted) {
    LOG_DEBUG(
        Lib_Audio3d,
        "called, port_id = {}, num_channels = {}, format = {}, num_samples = {}, output_route "
        "= {}, restricted = {}",
        port_id, num_channels, magic_enum::enum_name(format), num_samples,
        magic_enum::enum_name(output_route), restricted);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    // We support both S16 and FLOAT formats
    if (format != OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16 &&
        format != OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        LOG_ERROR(Lib_Audio3d, "Only S16 or FLOAT format supported, got: {}",
                  magic_enum::enum_name(format));
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    // Support mono (1), stereo (2), and 8-channel audio
    if (num_channels != 1 && num_channels != 2 && num_channels != 8) {
        LOG_ERROR(Lib_Audio3d, "Only 1, 2, or 8 channels supported, got: {}", num_channels);
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (!buffer || !num_samples) {
        LOG_ERROR(Lib_Audio3d, "!buffer || !num_samples");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    // Verify buffer alignment
    if (format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
        // S16 requires 2-byte alignment
        if ((reinterpret_cast<uintptr_t>(buffer) & 1) != 0) {
            LOG_ERROR(Lib_Audio3d, "s16 buffer & 1 != 0 (not 2-byte aligned)");
            return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
        }
    } else if (format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        // FLOAT requires 4-byte alignment
        if ((reinterpret_cast<uintptr_t>(buffer) & 3) != 0) {
            LOG_ERROR(Lib_Audio3d, "float buffer & 3 != 0 (not 4-byte aligned)");
            return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
        }
    }

    return PortQueueAudio(state->ports[port_id],
                          OrbisAudio3dPcm{
                              .format = format,
                              .sample_buffer = buffer,
                              .num_samples = num_samples,
                          },
                          num_channels);
}
s32 PS4_SYSV_ABI sceAudio3dObjectSetAttributes(const OrbisAudio3dPortId port_id,
                                               OrbisAudio3dObjectId object_id,
                                               const u64 num_attributes,
                                               const OrbisAudio3dAttribute* attribute_array) {
    LOG_DEBUG(Lib_Audio3d,
              "called, port_id = {}, object_id = {}, num_attributes = {}, attribute_array = {}",
              port_id, object_id, num_attributes, fmt::ptr(attribute_array));

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];

    for (u64 i = 0; i < num_attributes; i++) {
        const auto& attribute = attribute_array[i];

        switch (attribute.attribute_id) {
        case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PCM: {
            if (object_id >= port.objects.size())
                return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;

            auto& obj = port.objects[object_id];
            const auto* pcm = static_cast<OrbisAudio3dPcm*>(attribute.value);

            // Convert to stereo S16 (objects are mono, but we'll handle as stereo)
            auto converted_data = ConvertToStereoS16(*pcm, 1);

            if (converted_data.empty()) {
                return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
            }

            AudioData audio_data{
                .sample_buffer = std::move(converted_data),
                .num_samples = pcm->num_samples,
            };

            obj.pcm_queue.push_back(std::move(audio_data));
            break;
        }
        case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_POSITION: {
            if (object_id >= port.objects.size()) {
                return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
            }

            auto& obj = port.objects[object_id];
            if (attribute.value_size >= sizeof(float) * 3) {
                const float* position = static_cast<const float*>(attribute.value);
                ConvertPositionToOpenAL(position, obj.al_source.position);

                if (obj.al_source.source_id != 0) {
                    Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
                    alSource3f(obj.al_source.source_id, AL_POSITION, obj.al_source.position[0],
                               obj.al_source.position[1], obj.al_source.position[2]);
                }
            }
            break;
        }
        case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_GAIN: {
            if (object_id >= port.objects.size()) {
                return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
            }

            auto& obj = port.objects[object_id];
            if (attribute.value_size >= sizeof(float)) {
                obj.al_source.gain = *static_cast<const float*>(attribute.value);

                if (obj.al_source.source_id != 0) {
                    Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
                    alSourcef(obj.al_source.source_id, AL_GAIN, obj.al_source.gain);
                }
            }
            break;
        }
        default:
            LOG_INFO(Lib_Audio3d, "Processing attribute ID: {:#x}",
                     static_cast<u32>(attribute.attribute_id));
            break;
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortAdvance(const OrbisAudio3dPortId port_id) {
    LOG_DEBUG(Lib_Audio3d, "sceAudio3dPortAdvance(port_id={})", port_id);

    auto it = state->ports.find(port_id);
    if (it == state->ports.end())
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;

    Port& port = it->second;

    if (port.parameters.buffer_mode == OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_NO_ADVANCE) {
        return ORBIS_AUDIO3D_ERROR_NOT_SUPPORTED;
    }

    if (!port.queue.empty()) {
        port.current_buffer = std::move(port.queue.front());
        port.queue.pop_front();
    } else {
        port.current_buffer.reset();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetQueueLevel(const OrbisAudio3dPortId port_id, u32* queue_level,
                                             u32* queue_available) {
    auto it = state->ports.find(port_id);
    if (it == state->ports.end())
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;

    if (!queue_level && !queue_available)
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;

    const Port& port = it->second;
    const u32 depth = port.parameters.queue_depth;
    const u32 level = static_cast<u32>(port.queue.size());

    if (queue_level)
        *queue_level = level;

    if (queue_available)
        *queue_available = (level >= depth) ? 0 : (depth - level);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortPush(const OrbisAudio3dPortId port_id,
                                    const OrbisAudio3dBlocking blocking) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}, blocking = {}", port_id,
              magic_enum::enum_name(blocking));

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::lock_guard lock(port.lock);

    if (port.parameters.buffer_mode !=
        OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH) {
        LOG_ERROR(Lib_Audio3d, "port doesn't have push capability");
        return ORBIS_AUDIO3D_ERROR_NOT_SUPPORTED;
    }

    if (!port.current_buffer.has_value()) {
        // Generate silent audio if nothing to push
        LOG_DEBUG(Lib_Audio3d, "Port push with no buffer ready - generating silence");

        // Create silent buffer (stereo S16, granularity samples)
        const size_t silent_size = port.parameters.granularity * 2 * sizeof(int16_t);
        std::vector<std::byte> silent_buffer(silent_size, std::byte{0});

        return AudioOut::sceAudioOutOutput(state->audio_out_handle, silent_buffer.data());
    }

    auto& audio_data = port.current_buffer.value();

    // Stream object audio
    for (auto& obj : port.objects) {
        StreamObjectAudio(obj);
    }

    // Output bed audio
    if (!audio_data.sample_buffer.empty()) {
        LOG_DEBUG(Lib_Audio3d, "Pushing bed audio: {} bytes", audio_data.sample_buffer.size());

        return AudioOut::sceAudioOutOutput(
            state->audio_out_handle,
            const_cast<void*>(static_cast<const void*>(audio_data.sample_buffer.data())));
    } else {
        // Generate silent audio
        const size_t silent_size = port.parameters.granularity * 2 * sizeof(int16_t);
        std::vector<std::byte> silent_buffer(silent_size, std::byte{0});

        return AudioOut::sceAudioOutOutput(state->audio_out_handle, silent_buffer.data());
    }
}

s32 PS4_SYSV_ABI sceAudio3dPortSetAttribute(const OrbisAudio3dPortId port_id,
                                            const OrbisAudio3dAttributeId attribute_id,
                                            void* attribute, const u64 attribute_size) {
    LOG_INFO(Lib_Audio3d,
             "called, port_id = {}, attribute_id = {}, attribute = {}, attribute_size = {}",
             port_id, static_cast<u32>(attribute_id), attribute, attribute_size);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (!attribute) {
        LOG_ERROR(Lib_Audio3d, "!attribute");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    auto& port = state->ports[port_id];

    switch (attribute_id) {
    case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_LATE_REVERB_LEVEL:
        LOG_INFO(Lib_Audio3d, "Setting late reverb level (not implemented)");
        break;

    case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_POSITION:
        // Update listener position
        if (attribute_size >= sizeof(float) * 3) {
            const float* position = static_cast<const float*>(attribute);
            ConvertPositionToOpenAL(position, port.listener_position);

            if (Libraries::AudioOut::OpenALManager::Instance().IsInitialized()) {
                Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
                alListener3f(AL_POSITION, port.listener_position[0], port.listener_position[1],
                             port.listener_position[2]);
            }
        }
        break;

    default:
        LOG_INFO(Lib_Audio3d, "Setting port attribute {:#x} (not fully implemented)",
                 static_cast<u32>(attribute_id));
        break;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dCreateSpeakerArray() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients2() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortClose() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortCreate() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortDestroy() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFlush() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFreeState() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetList() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetParameters() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetState() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetStatus() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortQueryDebug() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dReportRegisterHandler() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dReportUnregisterHandler() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dSetGpuRenderer() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dStrError() {
    LOG_INFO(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* params) {
    LOG_DEBUG(Lib_Audio3d, "called");
    if (params) {
        auto default_params = OrbisAudio3dOpenParameters{
            .size_this = 0x20,
            .granularity = 0x100,
            .rate = OrbisAudio3dRate::ORBIS_AUDIO3D_RATE_48000,
            .max_objects = 512,
            .queue_depth = 2,
            .buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH,
        };
        memcpy(params, &default_params, 0x20);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dInitialize(const s64 reserved) {
    LOG_INFO(Lib_Audio3d, "called, reserved = {}", reserved);

    if (reserved != 0) {
        LOG_ERROR(Lib_Audio3d, "reserved != 0");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (state) {
        LOG_ERROR(Lib_Audio3d, "already initialized");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    state = std::make_unique<Audio3dState>();

    if (const auto init_ret = AudioOut::sceAudioOutInit();
        init_ret < 0 && init_ret != ORBIS_AUDIO_OUT_ERROR_ALREADY_INIT) {
        return init_ret;
    }

    // Initialize OpenAL for 3D audio
    if (!Libraries::AudioOut::OpenALManager::Instance().Initialize(AUDIO3D_SAMPLE_RATE)) {
        LOG_ERROR(Lib_Audio3d, "Failed to initialize OpenAL for 3D audio");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    // Enable HRTF if available
    if (Libraries::AudioOut::OpenALManager::Instance().HasHRTF()) {
        LOG_INFO(Lib_Audio3d, "HRTF is available");
    }

    // Set default OpenAL listener parameters
    Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    float listener_orientation[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
    alListenerfv(AL_ORIENTATION, listener_orientation);

    AudioOut::OrbisAudioOutParamExtendedInformation ext_info{};
    ext_info.data_format.Assign(AUDIO3D_OUTPUT_FORMAT);
    state->audio_out_handle =
        AudioOut::sceAudioOutOpen(0xFF, AudioOut::OrbisAudioOutPort::Audio3d, 0,
                                  AUDIO3D_OUTPUT_BUFFER_FRAMES, AUDIO3D_SAMPLE_RATE, ext_info);
    if (state->audio_out_handle < 0) {
        return state->audio_out_handle;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dTerminate() {
    LOG_INFO(Lib_Audio3d, "called");
    if (!state) {
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    // Clean up OpenAL resources for each port
    for (auto& [port_id, port] : state->ports) {
        std::lock_guard lock(port.lock);

        // Clean up OpenAL sources for objects
        for (auto& obj : port.objects) {
            if (obj.al_source.source_id != 0) {
                Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
                if (obj.al_source.buffer_id != 0) {
                    alDeleteBuffers(1, &obj.al_source.buffer_id);
                }
                alDeleteSources(1, &obj.al_source.source_id);
                obj.al_source.source_id = 0;
                obj.al_source.buffer_id = 0;
            }
        }
        port.objects.clear();
        port.queue.clear();
        port.current_buffer.reset();
    }
    state->ports.clear();

    AudioOut::sceAudioOutOutput(state->audio_out_handle, nullptr);
    AudioOut::sceAudioOutClose(state->audio_out_handle);
    state.reset();

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortOpen(const Libraries::UserService::OrbisUserServiceUserId user_id,
                                    const OrbisAudio3dOpenParameters* parameters,
                                    OrbisAudio3dPortId* port_id) {
    LOG_INFO(Lib_Audio3d, "called, user_id = {}, parameters = {},parameters_size = {:#x} id = {}",
             user_id, static_cast<const void*>(parameters), parameters->size_this,
             static_cast<void*>(port_id));

    if (!state) {
        LOG_ERROR(Lib_Audio3d, "!initialized");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    if (!parameters || !port_id) {
        if (port_id)
            *port_id = ORBIS_AUDIO3D_PORT_INVALID;
        LOG_ERROR(Lib_Audio3d, "ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER returned");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (user_id != Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        *port_id = ORBIS_AUDIO3D_PORT_INVALID;
        LOG_ERROR(Lib_Audio3d, "User in not System user (255)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    u32 max_objects = 0;
    u32 queue_depth = 0;
    u32 num_beds = 2;
    OrbisAudio3dBufferMode buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_NO_ADVANCE;

    switch (parameters->size_this) {
    case 0x10:
        max_objects = 512;
        queue_depth = 2;
        num_beds = 2;
        buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_NO_ADVANCE;
        break;

    case 0x18:
        max_objects = parameters->max_objects;
        queue_depth = parameters->queue_depth;
        num_beds = 2;
        buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_NO_PUSH;
        break;

    case 0x20:
        max_objects = parameters->max_objects;
        queue_depth = parameters->queue_depth;
        num_beds = 2;
        buffer_mode = parameters->buffer_mode;
        break;

    case 0x28:
        max_objects = parameters->max_objects;
        queue_depth = parameters->queue_depth;
        num_beds = parameters->num_beds;
        buffer_mode = parameters->buffer_mode;
        break;

    default:
        *port_id = ORBIS_AUDIO3D_PORT_INVALID;
        LOG_ERROR(Lib_Audio3d, "Not valid structure size");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    /* Validation */
    if (parameters->rate != OrbisAudio3dRate::ORBIS_AUDIO3D_RATE_48000 ||
        parameters->granularity < 0x100 || (parameters->granularity & 0xff) != 0 ||
        max_objects == 0 || queue_depth == 0 || num_beds != 2 ||
        buffer_mode > OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH) {
        *port_id = ORBIS_AUDIO3D_PORT_INVALID;
        LOG_ERROR(Lib_Audio3d, "ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER returned");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    /* Granularity-based queue limits */
    if ((parameters->granularity == 0x100 && queue_depth > 64) ||
        (parameters->granularity == 0x200 && queue_depth > 31) ||
        (parameters->granularity == 0x300 && queue_depth > 20) ||
        (parameters->granularity > 0x300 && queue_depth > 15)) {
        *port_id = ORBIS_AUDIO3D_PORT_INVALID;
        LOG_ERROR(Lib_Audio3d, "ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER returned");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (max_objects > 512) {
        LOG_WARNING(Lib_Audio3d, "uiMaxObjects clamped to 512");
        max_objects = 512;
    }

    /* Fixed pool of 4 ports */
    for (OrbisAudio3dPortId id = 0; id < 4; ++id) {
        if (!state->ports.contains(id)) {

            auto [it, inserted] = state->ports.try_emplace(id);
            auto& port = it->second;

            port.parameters = *parameters;
            port.parameters.max_objects = max_objects;
            port.parameters.queue_depth = queue_depth;
            port.parameters.num_beds = num_beds;
            port.parameters.buffer_mode = buffer_mode;

            port.queue.clear();
            port.current_buffer.reset();
            port.objects.clear();
            port.objects.resize(port.parameters.max_objects);

            *port_id = id;

            LOG_INFO(Lib_Audio3d,
                     "Opened Audio3D port {} (max_objects={}, beds={}, queue_depth={})", id,
                     max_objects, num_beds, queue_depth);

            return ORBIS_OK;
        }
    }

    *port_id = ORBIS_AUDIO3D_PORT_INVALID;
    LOG_ERROR(Lib_Audio3d, "ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES returned");
    return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOpen(
    const OrbisAudio3dPortId port_id, const Libraries::UserService::OrbisUserServiceUserId user_id,
    s32 type, const s32 index, const u32 len, const u32 freq,
    const AudioOut::OrbisAudioOutParamExtendedInformation param) {
    LOG_INFO(Lib_Audio3d,
             "called, port_id = {}, user_id = {}, type = {}, index = {}, len = {}, freq = {}",
             port_id, user_id, type, index, len, freq);

    if (!state || !state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "Invalid Audio3D port");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];

    if (type < 0)
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;

    if (len != port.parameters.granularity) {
        LOG_ERROR(Lib_Audio3d, "len {} != granularity {}", len, port.parameters.granularity);
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    const auto ao_type = static_cast<AudioOut::OrbisAudioOutPort>(type /* | 0x80000000*/);

    s32 audio_out_handle = AudioOut::sceAudioOutOpen(user_id, ao_type, index, len, freq, param);

    if (audio_out_handle < 0) {
        LOG_ERROR(Lib_Audio3d, "sceAudioOutOpen failed: 0x{:x}",
                  static_cast<u32>(audio_out_handle));
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    return audio_out_handle;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutClose(const s32 handle) {
    LOG_INFO(Lib_Audio3d, "called, handle = {}", handle);
    return AudioOut::sceAudioOutClose(handle);
}

s32 PS4_SYSV_ABI sceAudio3dObjectReserve(const OrbisAudio3dPortId port_id,
                                         OrbisAudio3dObjectId* object_id) {
    LOG_INFO(Lib_Audio3d, "called, port_id = {}, object_id = {}", port_id,
             static_cast<void*>(object_id));

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (!object_id) {
        LOG_ERROR(Lib_Audio3d, "!object_id");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    auto& port = state->ports[port_id];
    std::lock_guard lock(port.lock);

    for (u32 i = 0; i < port.objects.size(); ++i) {
        auto& obj = port.objects[i];
        if (!obj.in_use) {
            obj = {}; // reset

            // Create OpenAL source for this object
            if (Libraries::AudioOut::OpenALManager::Instance().IsInitialized()) {
                Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();

                alGenSources(1, &obj.al_source.source_id);
                ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    LOG_ERROR(Lib_Audio3d, "Failed to generate OpenAL source: {}", error);
                    return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
                }

                // Set default OpenAL source properties for stereo S16
                alSourcef(obj.al_source.source_id, AL_PITCH, obj.al_source.pitch);
                alSourcef(obj.al_source.source_id, AL_GAIN, obj.al_source.gain);
                alSource3f(obj.al_source.source_id, AL_POSITION, obj.al_source.position[0],
                           obj.al_source.position[1], obj.al_source.position[2]);
                alSource3f(obj.al_source.source_id, AL_VELOCITY, obj.al_source.velocity[0],
                           obj.al_source.velocity[1], obj.al_source.velocity[2]);
                alSourcef(obj.al_source.source_id, AL_REFERENCE_DISTANCE,
                          obj.al_source.reference_distance);
                alSourcef(obj.al_source.source_id, AL_MAX_DISTANCE, obj.al_source.max_distance);
                alSourcef(obj.al_source.source_id, AL_ROLLOFF_FACTOR, obj.al_source.rolloff_factor);
                alSourcei(obj.al_source.source_id, AL_LOOPING,
                          obj.al_source.looping ? AL_TRUE : AL_FALSE);

                // Enable 3D positioning
                alSourcei(obj.al_source.source_id, AL_SOURCE_RELATIVE, AL_FALSE);
            }

            obj.in_use = true;
            obj.active = true;
            *object_id = i;
            return ORBIS_OK;
        }
    }

    // No free object slots
    *object_id = 0xFFFFFFFF;
    LOG_ERROR(Lib_Audio3d, "OUT OF RESOURCES");
    return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
}

s32 PS4_SYSV_ABI sceAudio3dObjectUnreserve(OrbisAudio3dPortId port_id,
                                           OrbisAudio3dObjectId object_id) {
    LOG_INFO(Lib_Audio3d, "called prot_id = {} , object_id = {}", port_id, object_id);
    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];

    if (object_id >= port.objects.size())
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;

    std::lock_guard lock(port.lock);

    auto& obj = port.objects[object_id];

    if (!obj.in_use)
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;

    // Clean up OpenAL resources
    if (obj.al_source.source_id != 0) {
        Libraries::AudioOut::OpenALManager::Instance().MakeContextCurrent();
        if (obj.al_source.buffer_id != 0) {
            alDeleteBuffers(1, &obj.al_source.buffer_id);
        }
        alDeleteSources(1, &obj.al_source.source_id);
    }

    // Mark free
    obj.in_use = false;
    obj.active = false;
    obj.al_source = {}; // Reset OpenAL source

    return ORBIS_OK;
}

s32 sceAudio3dPortGetAttributesSupported(OrbisAudio3dPortId portId, OrbisAudio3dAttributeId* caps,
                                         u32* numCaps) {
    LOG_INFO(Lib_Audio3d, "called, portId = {}, caps = {},numCaps = {} ", portId,
             static_cast<void*>(caps), static_cast<void*>(numCaps));

    if (!numCaps)
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;

    if (!state->ports.contains(portId)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    const bool hrtf = Audio3dHRTFActive();

    std::vector<OrbisAudio3dAttributeId> supported;

    // Always supported for S16 stereo
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PCM);
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_GAIN);
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PRIORITY);
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_RESET_STATE);
    supported.push_back(
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_APPLICATION_SPECIFIC);
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_RESTRICTED);

    if (hrtf) {
        // Object-based renderer with HRTF
        supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_POSITION);
        supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_SPREAD);
        supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_OUTPUT_ROUTE);
        supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_PASSTHROUGH);
        supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_OBJECT_ATTRIBUTE_AMBISONICS);
    } else {
        // Speaker / bed renderer
        supported.push_back(
            OrbisAudio3dAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_RADIUS);
        supported.push_back(
            OrbisAudio3dAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_HEIGHT_AWARE);
    }

    // Supported in both
    supported.push_back(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_LATE_REVERB_LEVEL);

    if (!caps) {
        *numCaps = static_cast<unsigned int>(supported.size());
        return ORBIS_OK;
    }

    const unsigned int count = std::min(*numCaps, static_cast<unsigned int>(supported.size()));

    memcpy(caps, supported.data(), count * sizeof(OrbisAudio3dAttributeId));
    *numCaps = count;

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("pZlOm1aF3aA", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dAudioOutClose);
    LIB_FUNCTION("ucEsi62soTo", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dAudioOutOpen);
    LIB_FUNCTION("7NYEzJ9SJbM", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dAudioOutOutput);
    LIB_FUNCTION("HbxYY27lK6E", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dAudioOutOutputs);
    LIB_FUNCTION("9tEwE0GV0qo", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dBedWrite);
    LIB_FUNCTION("xH4Q9UILL3o", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dBedWrite2);
    LIB_FUNCTION("lvWMW6vEqFU", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dCreateSpeakerArray);
    LIB_FUNCTION("8hm6YdoQgwg", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dDeleteSpeakerArray);
    LIB_FUNCTION("Im+jOoa5WAI", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dGetDefaultOpenParameters);
    LIB_FUNCTION("kEqqyDkmgdI", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dGetSpeakerArrayMemorySize);
    LIB_FUNCTION("-R1DukFq7Dk", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dGetSpeakerArrayMixCoefficients);
    LIB_FUNCTION("-Re+pCWvwjQ", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dGetSpeakerArrayMixCoefficients2);
    LIB_FUNCTION("UmCvjSmuZIw", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dInitialize);
    LIB_FUNCTION("jO2tec4dJ2M", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dObjectReserve);
    LIB_FUNCTION("4uyHN9q4ZeU", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dObjectSetAttributes);
    LIB_FUNCTION("1HXxo-+1qCw", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dObjectUnreserve);
    LIB_FUNCTION("lw0qrdSjZt8", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortAdvance);
    LIB_FUNCTION("OyVqOeVNtSk", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortClose);
    LIB_FUNCTION("UHFOgVNz0kk", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortCreate);
    LIB_FUNCTION("Mw9mRQtWepY", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortDestroy);
    LIB_FUNCTION("ZOGrxWLgQzE", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortFlush);
    LIB_FUNCTION("uJ0VhGcxCTQ", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortFreeState);
    LIB_FUNCTION("9ZA23Ia46Po", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dPortGetAttributesSupported);
    LIB_FUNCTION("SEggctIeTcI", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortGetList);
    LIB_FUNCTION("flPcUaXVXcw", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortGetParameters);
    LIB_FUNCTION("YaaDbDwKpFM", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortGetQueueLevel);
    LIB_FUNCTION("CKHlRW2E9dA", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortGetState);
    LIB_FUNCTION("iRX6GJs9tvE", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortGetStatus);
    LIB_FUNCTION("XeDDK0xJWQA", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortOpen);
    LIB_FUNCTION("VEVhZ9qd4ZY", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortPush);
    LIB_FUNCTION("-pzYDZozm+M", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortQueryDebug);
    LIB_FUNCTION("Yq9bfUQ0uJg", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dPortSetAttribute);
    LIB_FUNCTION("QfNXBrKZeI0", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dReportRegisterHandler);
    LIB_FUNCTION("psv2gbihC1A", "libSceAudio3d", 1, "libSceAudio3d",
                 sceAudio3dReportUnregisterHandler);
    LIB_FUNCTION("yEYXcbAGK14", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dSetGpuRenderer);
    LIB_FUNCTION("Aacl5qkRU6U", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dStrError);
    LIB_FUNCTION("WW1TS2iz5yc", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dTerminate);
};

} // namespace Libraries::Audio3d