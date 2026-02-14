// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_error.h"
#include "core/libraries/audio3d/audio3d_error.h"
#include "core/libraries/audio3d/audio3d_openal.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Audio3dOpenAL {

static constexpr u32 AUDIO3D_SAMPLE_RATE = 48000;

static constexpr AudioOut::OrbisAudioOutParamFormat AUDIO3D_OUTPUT_FORMAT =
    AudioOut::OrbisAudioOutParamFormat::S16Stereo;
static constexpr u32 AUDIO3D_OUTPUT_NUM_CHANNELS = 2;
static constexpr u32 AUDIO3D_OUTPUT_BUFFER_FRAMES = 0x100;

static std::unique_ptr<Audio3dState> state;

s32 PS4_SYSV_ABI sceAudio3dAudioOutClose(const s32 handle) {
    LOG_INFO(Lib_Audio3d, "called, handle = {}", handle);
    return AudioOut::sceAudioOutClose(handle);
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOpen(
    const OrbisAudio3dPortId port_id, const Libraries::UserService::OrbisUserServiceUserId user_id,
    s32 type, const s32 index, const u32 len, const u32 freq,
    const AudioOut::OrbisAudioOutParamExtendedInformation param) {
    LOG_INFO(Lib_Audio3d,
             "called, port_id = {}, user_id = {}, type = {}, index = {}, len = {}, freq = {}",
             port_id, user_id, type, index, len, freq);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (len != state->ports[port_id].parameters.granularity) {
        LOG_ERROR(Lib_Audio3d, "len != state->ports[port_id].parameters.granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    return sceAudioOutOpen(user_id, static_cast<AudioOut::OrbisAudioOutPort>(type), index, len,
                           freq, param);
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

static inline s16 OrbisFloatToS16(float v) {
    if (std::abs(v) < 1.0e-20f)
        v = 0.0f;

    // Sony behavior: +1.0f -> 32767, -1.0f -> -32768
    const float scaled = v * 32768.0f;

    if (scaled >= 32767.0f)
        return 32767;
    if (scaled <= -32768.0f)
        return -32768;

    return static_cast<s16>(scaled + (scaled >= 0 ? 0.5f : -0.5f));
}

static void ConvertAudioToStereoS16(const OrbisAudio3dPcm& pcm, const u32 num_channels,
                                    u8** out_data, u32* out_size) {
    *out_data = nullptr;
    *out_size = 0;

    // Validate input parameters
    if (!pcm.sample_buffer || pcm.num_samples == 0 || num_channels == 0) {
        return;
    }

    // Check for overflow in size calculation
    const u32 num_frames = pcm.num_samples;

    // Validate frame count (prevent excessive allocation)
    constexpr u32 MAX_FRAMES = 48000 * 10; // 10 seconds at 48kHz
    if (num_frames > MAX_FRAMES || num_frames == 0) {
        return;
    }

    // Calculate output size with overflow checking
    if (num_frames > (std::numeric_limits<u32>::max() / 2)) {
        return; // Overflow in out_samples calculation
    }
    const u32 out_samples = num_frames * 2; // stereo

    if (out_samples > (std::numeric_limits<u32>::max() / sizeof(s16))) {
        return; // Overflow in out_bytes calculation
    }
    const u32 out_bytes = out_samples * sizeof(s16);

    // Allocate output buffer
    s16* dst = static_cast<s16*>(std::malloc(out_bytes));
    if (!dst) {
        return;
    }

    // Initialize to silence
    std::memset(dst, 0, out_bytes);

    *out_data = reinterpret_cast<u8*>(dst);
    *out_size = out_bytes;

    // Calculate expected input size for validation
    const size_t sample_size =
        (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) ? sizeof(s16) : sizeof(float);
    const size_t expected_input_bytes = num_frames * num_channels * sample_size;

    try {
        switch (pcm.format) {
        case OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16: {
            const s16* src = static_cast<const s16*>(pcm.sample_buffer);

            switch (num_channels) {
            case 2:
                std::memcpy(dst, src, out_bytes);
                break;

            case 1:
                // Mono to stereo
                for (u32 i = 0; i < num_frames; ++i) {
                    if (i >= num_frames)
                        break;
                    s16 s = src[i];
                    dst[i * 2 + 0] = s;
                    dst[i * 2 + 1] = s;
                }
                break;

            case 8:
                for (u32 i = 0; i < num_frames; ++i) {
                    if (i * 8 >= num_frames * num_channels)
                        break; // Prevent overflow
                    const s16* frame = &src[i * 8];
                    dst[i * 2 + 0] = frame[0];
                    dst[i * 2 + 1] = frame[1];
                }
                break;

            default:
                // Unsupported channel layout - already memset to 0
                LOG_ERROR(Lib_Audio3d, "Unsupported S16 channel count: {}", num_channels);
                break;
            }
            break;
        }

        case OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT: {
            const float* src = static_cast<const float*>(pcm.sample_buffer);

            switch (num_channels) {
            case 2:
                // Stereo float to stereo S16
                for (u32 i = 0; i < num_frames; ++i) {
                    if (i >= num_frames)
                        break;
                    dst[i * 2 + 0] = OrbisFloatToS16(src[i * 2 + 0]);
                    dst[i * 2 + 1] = OrbisFloatToS16(src[i * 2 + 1]);
                }
                break;

            case 1:
                // Mono float to stereo
                for (u32 i = 0; i < num_frames; ++i) {
                    if (i >= num_frames)
                        break;
                    s16 s = OrbisFloatToS16(src[i]);
                    dst[i * 2 + 0] = s;
                    dst[i * 2 + 1] = s;
                }
                break;

            case 8:
                // Audio3D float to stereo (L/R only)
                for (u32 i = 0; i < num_frames; ++i) {
                    if (i * 8 >= num_frames * num_channels)
                        break;
                    const float* frame = &src[i * 8];
                    dst[i * 2 + 0] = OrbisFloatToS16(frame[0]);
                    dst[i * 2 + 1] = OrbisFloatToS16(frame[1]);
                }
                break;

            default:
                // Already memset to 0
                LOG_ERROR(Lib_Audio3d, "Unsupported float channel count: {}", num_channels);
                break;
            }
            break;
        }

        default:
            // Unknown format - already memset to 0
            LOG_ERROR(Lib_Audio3d, "Unsupported audio format: {}",
                      magic_enum::enum_name(pcm.format));
            break;
        }
    } catch (...) {
        // Catch any exceptions (unlikely but safe)
        std::free(dst);
        *out_data = nullptr;
        *out_size = 0;
    }
}

static s32 PortQueueAudio(Port& port, const OrbisAudio3dPcm& pcm, const u32 num_channels) {
    u8* dst_data = nullptr;
    u32 dst_size = 0;

    // Convert audio to stereo S16
    ConvertAudioToStereoS16(pcm, num_channels, &dst_data, &dst_size);

    if (!dst_data) {
        LOG_ERROR(Lib_Audio3d, "Failed to convert audio samples");
        return ORBIS_AUDIO3D_ERROR_OUT_OF_MEMORY;
    }

    port.queue.emplace_back(AudioData{
        .sample_buffer = dst_data,
        .num_samples = pcm.num_samples,
    });
    return ORBIS_OK;
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

    if (output_route > OrbisAudio3dOutputRoute::ORBIS_AUDIO3D_OUTPUT_BOTH) {
        LOG_ERROR(Lib_Audio3d, "output_route > ORBIS_AUDIO3D_OUTPUT_BOTH");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (format > OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        LOG_ERROR(Lib_Audio3d, "format > ORBIS_AUDIO3D_FORMAT_FLOAT");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (num_channels != 2 && num_channels != 8) {
        LOG_ERROR(Lib_Audio3d, "num_channels != 2 && num_channels != 8");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (!buffer || !num_samples) {
        LOG_ERROR(Lib_Audio3d, "!buffer || !num_samples");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_FLOAT) {
        if ((reinterpret_cast<uintptr_t>(buffer) & 3) != 0) {
            LOG_ERROR(Lib_Audio3d, "buffer & 3 != 0");
            return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
        }
    } else if (format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
        if ((reinterpret_cast<uintptr_t>(buffer) & 1) != 0) {
            LOG_ERROR(Lib_Audio3d, "buffer & 1 != 0");
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

s32 PS4_SYSV_ABI sceAudio3dCreateSpeakerArray() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients2() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceAudio3dObjectReserve(const OrbisAudio3dPortId port_id,
                                         OrbisAudio3dObjectId* object_id) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}", port_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (!object_id) {
        LOG_ERROR(Lib_Audio3d, "!object_id");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    *object_id = 1; // Simple stub
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dObjectSetAttributes(const OrbisAudio3dPortId port_id,
                                               const OrbisAudio3dObjectId object_id,
                                               const u64 num_attributes,
                                               const OrbisAudio3dAttribute* attribute_array) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}, object_id = {}, num_attributes = {}", port_id,
              object_id, num_attributes);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (!num_attributes || !attribute_array) {
        LOG_ERROR(Lib_Audio3d, "!num_attributes || !attribute_array");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    for (u64 i = 0; i < num_attributes; i++) {
        const auto& attr = attribute_array[i];

        if (attr.attribute_id == OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_PCM) {
            if (attr.value_size < sizeof(OrbisAudio3dPcm)) {
                continue;
            }

            const auto* pcm = static_cast<const OrbisAudio3dPcm*>(attr.value);
            // Queue the PCM data (assumes stereo for simplicity)
            PortQueueAudio(state->ports[port_id], *pcm, 2);
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dObjectUnreserve() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortAdvance(const OrbisAudio3dPortId port_id) {
    LOG_TRACE(Lib_Audio3d, "called, port_id = {}", port_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];

    if (port.current_buffer) {
        free(port.current_buffer->sample_buffer);
        port.current_buffer.reset();
    }

    if (!port.queue.empty()) {
        port.current_buffer = port.queue.front();
        port.queue.pop_front();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortClose() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortCreate() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortDestroy() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFlush() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFreeState() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetList() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetParameters() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetQueueLevel(const OrbisAudio3dPortId port_id, u32* queue_level,
                                             u32* queue_available) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}, queue_level = {}, queue_available = {}", port_id,
              static_cast<void*>(queue_level), static_cast<void*>(queue_available));

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    if (!queue_level && !queue_available) {
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    const auto port = state->ports[port_id];
    const size_t size = port.queue.size();

    if (queue_level) {
        *queue_level = size;
    }

    if (queue_available) {
        *queue_available = port.parameters.queue_depth - size;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetState() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetStatus() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortOpen(const Libraries::UserService::OrbisUserServiceUserId user_id,
                                    const OrbisAudio3dOpenParameters* parameters,
                                    OrbisAudio3dPortId* port_id) {
    LOG_INFO(Lib_Audio3d, "called, user_id = {}, parameters = {}, id = {}", user_id,
             static_cast<const void*>(parameters), static_cast<void*>(port_id));

    if (!state) {
        LOG_ERROR(Lib_Audio3d, "!initialized");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    if (!parameters || !port_id) {
        LOG_ERROR(Lib_Audio3d, "!parameters || !id");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    const int id = static_cast<int>(state->ports.size()) + 1;

    if (id > 3) {
        LOG_ERROR(Lib_Audio3d, "id > 3");
        return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
    }

    *port_id = id;
    std::memcpy(&state->ports[id].parameters, parameters, parameters->size_this);

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

    const auto& port = state->ports[port_id];
    if (port.parameters.buffer_mode !=
        OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH) {
        LOG_ERROR(Lib_Audio3d, "port doesn't have push capability");
        return ORBIS_AUDIO3D_ERROR_NOT_SUPPORTED;
    }

    if (!port.current_buffer.has_value()) {
        // Nothing to push.
        LOG_DEBUG(Lib_Audio3d, "Port push with no buffer ready");
        return ORBIS_OK;
    }

    // TODO: Implement asynchronous blocking mode.
    const auto& [sample_buffer, num_samples] = port.current_buffer.value();
    return AudioOut::sceAudioOutOutput(state->audio_out_handle, sample_buffer);
}

s32 PS4_SYSV_ABI sceAudio3dPortQueryDebug() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortSetAttribute(const OrbisAudio3dPortId port_id,
                                            const OrbisAudio3dAttributeId attribute_id,
                                            void* attribute, const u64 attribute_size) {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dReportRegisterHandler() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dReportUnregisterHandler() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dSetGpuRenderer() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dStrError() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dTerminate() {
    LOG_INFO(Lib_Audio3d, "called");

    if (!state) {
        return ORBIS_OK;
    }

    if (state->audio_out_handle >= 0) {
        AudioOut::sceAudioOutClose(state->audio_out_handle);
    }

    // Clean up queued audio data
    for (auto& [port_id, port] : state->ports) {
        if (port.current_buffer) {
            free(port.current_buffer->sample_buffer);
        }
        for (auto& data : port.queue) {
            free(data.sample_buffer);
        }
    }

    state.reset();

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
}

} // namespace Libraries::Audio3dOpenAL
