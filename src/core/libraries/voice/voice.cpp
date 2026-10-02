// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <mutex>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioin_backend.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/voice/voice.h"
#include "core/libraries/voice/voice_error.h"

// Real hardware encodes/decodes through libSceAjm; this instead exchanges raw s16 PCM between
// ports, so it only interoperates between two instances of this codebase, not real hardware.
// sceVoice never touches the network itself either way -- games move the bytes themselves.
// Device ports (mic/speaker) open lazily on first real use, not at sceVoiceCreatePort.

namespace Libraries::Voice {

namespace {

constexpr u32 kSampleRate = 16000; // Rate the retail module validates for its ports.
constexpr u32 kFrameSamples = 320; // 20 ms at 16 kHz.
constexpr u32 kFrameBytes = kFrameSamples * sizeof(s16);
constexpr size_t kMaxQueuedSamples = kFrameSamples * 8; // ~160 ms of headroom.

std::unique_ptr<AudioOut::PortBackend> OpenPlaybackDevice() {
    AudioOut::PortOut port_out{};
    port_out.type = AudioOut::OrbisAudioOutPort::Voice;
    // Mirrors AudioOut's own S16Mono table entry (see GetFormatInfo() in audioout.cpp).
    port_out.format_info = AudioOut::AudioFormatInfo{.is_float = false,
                                                     .sample_size = sizeof(s16),
                                                     .num_channels = 1,
                                                     .channel_layout = {0},
                                                     .is_std = false};
    port_out.sample_rate = kSampleRate;
    port_out.buffer_frames = kFrameSamples;
    port_out.volume.fill(AudioOut::ORBIS_AUDIO_OUT_VOLUME_0DB);
    if (EmulatorSettings.GetAudioBackend() == AudioBackend::OpenAL) {
        AudioOut::OpenALAudioOut backend;
        return backend.Open(port_out);
    }
    AudioOut::SDLAudioOut backend;
    return backend.Open(port_out);
}

struct Port {
    bool allocated = false;
    OrbisVoicePortParam param{};
    bool paused = false;
    bool muted = false;
    float volume = 1.0f;
    u32 bitrate = 48000;
    s32 connected_to = -1; // For an "in" port, the "out" port it is routed to, if any.

    // capture_config must outlive audio_in: AudioIn::PortInBackend stores a reference to it,
    // not a copy.
    std::unique_ptr<AudioIn::PortIn> capture_config{};
    std::unique_ptr<AudioIn::PortInBackend> audio_in{};
    std::unique_ptr<AudioOut::PortBackend> audio_out{};
    bool audio_in_open_tried = false;
    bool audio_out_open_tried = false;

    // Set by sceVoiceWriteToIPort; distinguishes an app-fed port from a mic-backed one.
    bool app_fed = false;
    std::deque<s16> queue;
};

struct VoiceManager {
    std::mutex mutex;
    bool initialized = false;
    bool started = false;
    std::array<Port, ORBIS_VOICE_MAX_PORT> ports{};
};

VoiceManager g_voice_manager;

// Requires g_voice_manager.mutex to already be held.
void ClosePortDevices(Port& port) {
    port.audio_in.reset();
    port.capture_config.reset();
    port.audio_out.reset();
    port.audio_in_open_tried = false;
    port.audio_out_open_tried = false;
    port.app_fed = false;
    port.queue.clear();
}

// Requires g_voice_manager.mutex to already be held. Returns null if no capture device is
// available (e.g. no microphone on the host); that's not a hard error.
AudioIn::PortInBackend* GetOrOpenCapture(Port& port) {
    if (!port.audio_in_open_tried) {
        port.audio_in_open_tried = true;
        port.capture_config = std::make_unique<AudioIn::PortIn>();
        auto& cfg = *port.capture_config;
        cfg.type = AudioIn::OrbisAudioInType::VoiceChat;
        cfg.format = AudioIn::OrbisAudioInParamFormat::S16Mono;
        cfg.samples_num = kFrameSamples;
        cfg.freq = kSampleRate;
        cfg.channels_num = 1;
        cfg.sample_size = sizeof(s16);
        AudioIn::SDLAudioIn backend;
        port.audio_in = backend.Open(cfg);
        if (!port.audio_in) {
            port.capture_config.reset();
            LOG_WARNING(Lib_Voice, "No capture device available for a voice port");
        }
    }
    return port.audio_in.get();
}

AudioOut::PortBackend* GetOrOpenPlayback(Port& port) {
    if (!port.audio_out_open_tried) {
        port.audio_out_open_tried = true;
        port.audio_out = OpenPlaybackDevice();
        if (!port.audio_out) {
            LOG_WARNING(Lib_Voice, "No playback device available for a voice port");
        }
    }
    return port.audio_out.get();
}

} // namespace

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[in_port_id].allocated ||
        !g_voice_manager.ports[out_port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    g_voice_manager.ports[in_port_id].connected_to = static_cast<s32>(out_port_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceCreatePort(s32* port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id == nullptr || param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto it = std::find_if(g_voice_manager.ports.begin(), g_voice_manager.ports.end(),
                                 [](const Port& port) { return !port.allocated; });
    if (it == g_voice_manager.ports.end()) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *it = Port{};
    it->allocated = true;
    it->param = *param;
    *port_id = static_cast<s32>(std::distance(g_voice_manager.ports.begin(), it));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDeletePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.connected_to == static_cast<s32>(port_id)) {
            port.connected_to = -1;
        }
    }
    g_voice_manager.ports[port_id] = Port{};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& in_port = g_voice_manager.ports[in_port_id];
    if (in_port.connected_to == static_cast<s32>(out_port_id)) {
        in_port.connected_to = -1;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceEnd() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.initialized = false;
    g_voice_manager.started = false;
    g_voice_manager.ports = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (bitrate == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *bitrate = port.bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetMuteFlag(u32 port_id, bool* mute) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (mute == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *mute = port.muted;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortAttr(u32 port_id, s32 attr_id, void* value, u32 size) {
    LOG_TRACE(Lib_Voice, "port_id = {}, attr_id = {}, size = {}", port_id, attr_id, size);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (value == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    // Required size per id, confirmed via disassembly (see voice.h).
    u32 expected_size;
    switch (attr_id) {
    case ORBIS_VOICE_ATTR_1000:
    case ORBIS_VOICE_ATTR_1001:
    case ORBIS_VOICE_ATTR_2000:
    case ORBIS_VOICE_ATTR_2002:
        expected_size = 1;
        break;
    case ORBIS_VOICE_ATTR_1003:
        expected_size = 2;
        break;
    case ORBIS_VOICE_ATTR_1002:
    case ORBIS_VOICE_ATTR_2001:
        expected_size = 4;
        break;
    default:
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    if (size != expected_size) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    // Attribute meaning is unknown; report a stable default instead of garbage.
    switch (expected_size) {
    case 1:
        *static_cast<u8*>(value) = 0;
        break;
    case 2:
        *static_cast<u16*>(value) = 0;
        break;
    case 4:
        *static_cast<u32*>(value) = 0;
        break;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (info == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    info->port_type = port.param.type;
    info->state = port.paused ? 1 : 0;
    info->edge = nullptr;
    info->byte_count = static_cast<u32>(port.queue.size() * sizeof(s16));
    info->frame_size = kFrameBytes;
    info->edge_count = 0;
    info->reserved = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetResourceInfo(void* info) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (info == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetVolume(u32 port_id, float* volume) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (volume == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *volume = port.volume;
    return ORBIS_OK;
}

static s32 InitInternal(const OrbisVoiceInitParam* param) {
    if (g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_ALREADY_INIT;
    }
    if (param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.ports = {};
    g_voice_manager.started = false;
    g_voice_manager.initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceInit(const OrbisVoiceInitParam* param, s32 container) {
    LOG_INFO(Lib_Voice, "called, container = {}", container);
    return InitInternal(param);
}

s32 PS4_SYSV_ABI sceVoiceInitHQ(const OrbisVoiceInitParam* param, s32 container) {
    LOG_INFO(Lib_Voice, "called, container = {}", container);
    return InitInternal(param);
}

s32 PS4_SYSV_ABI sceVoicePausePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.paused = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoicePausePortAll() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.paused = true;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceReadFromOPort(u32 port_id, void* data, u32* size) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (data == nullptr || size == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& out_port = g_voice_manager.ports[port_id];
    if (!out_port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    const u32 requested = *size;
    *size = 0;
    if (out_port.paused || out_port.muted || requested == 0) {
        return ORBIS_OK;
    }
    // Find the port connected into this one.
    const auto it = std::find_if(
        g_voice_manager.ports.begin(), g_voice_manager.ports.end(),
        [port_id](const Port& p) { return p.connected_to == static_cast<s32>(port_id); });
    if (it == g_voice_manager.ports.end() || !it->allocated || it->paused || it->muted) {
        return ORBIS_OK;
    }
    Port& in_port = *it;
    if (in_port.app_fed) {
        // Drain data queued by a prior WriteToIPort.
        const u32 available_bytes = static_cast<u32>(in_port.queue.size() * sizeof(s16));
        const u32 to_copy = std::min(requested, available_bytes);
        std::copy_n(in_port.queue.begin(), to_copy / sizeof(s16), static_cast<s16*>(data));
        in_port.queue.erase(in_port.queue.begin(), in_port.queue.begin() + to_copy / sizeof(s16));
        *size = to_copy;
        return ORBIS_OK;
    }
    // Otherwise treat it as mic-backed and capture a live frame.
    auto* capture = GetOrOpenCapture(in_port);
    if (capture == nullptr) {
        return ORBIS_OK;
    }
    std::array<s16, kFrameSamples> frame{};
    if (capture->TryRead(frame.data()) <= 0) {
        // No data ready yet; never block the caller waiting for the microphone.
        return ORBIS_OK;
    }
    const u32 to_copy = std::min(requested, kFrameBytes);
    std::memcpy(data, frame.data(), to_copy);
    *size = to_copy;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResetPort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    const auto param = port.param;
    port = Port{};
    port.allocated = true;
    port.param = param;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.paused = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePortAll() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.paused = false;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetBitRate(u32 port_id, u32 bitrate) {
    LOG_INFO(Lib_Voice, "port_id = {}, bitrate = {}", port_id, bitrate);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.bitrate = bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlag(u32 port_id, bool mute) {
    LOG_INFO(Lib_Voice, "port_id = {}, mute = {}", port_id, mute);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.muted = mute;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll(bool mute) {
    LOG_INFO(Lib_Voice, "mute = {}", mute);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.muted = mute;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetThreadsParams(void* params) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (params == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetVolume(u32 port_id, float volume) {
    LOG_INFO(Lib_Voice, "port_id = {}, volume = {}", port_id, volume);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.volume = volume;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStart(const OrbisVoiceStartParam* param) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (param == nullptr || param->container == 0) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.started = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStop() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.started = false;
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            ClosePortDevices(port);
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceUpdatePort(u32 port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.param = *param;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADAdjustment(u32 port_id, s32 value) {
    LOG_INFO(Lib_Voice, "port_id = {}, value = {}", port_id, value);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADSetVersion(u32 version) {
    LOG_INFO(Lib_Voice, "version = {}", version);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceWriteToIPort(u32 port_id, const void* data, u32* size) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (data == nullptr || size == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& in_port = g_voice_manager.ports[port_id];
    if (!in_port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    in_port.app_fed = true;
    if (in_port.paused || in_port.muted || *size == 0) {
        return ORBIS_OK;
    }
    const u32 sample_count = *size / sizeof(s16);
    const auto* samples = static_cast<const s16*>(data);
    // Play back immediately if connected to a device port and the size matches our fixed
    // frame; otherwise just queue it.
    if (in_port.connected_to >= 0) {
        Port& out_port = g_voice_manager.ports[in_port.connected_to];
        if (out_port.allocated && !out_port.paused && !out_port.muted && *size == kFrameBytes) {
            auto* playback = GetOrOpenPlayback(out_port);
            if (playback != nullptr) {
                std::array<s16, kFrameSamples> scaled{};
                const float gain = std::clamp(in_port.volume * out_port.volume, 0.0f, 4.0f);
                for (u32 i = 0; i < kFrameSamples; ++i) {
                    scaled[i] =
                        static_cast<s16>(std::clamp(samples[i] * gain, -32768.0f, 32767.0f));
                }
                playback->Output(scaled.data());
                return ORBIS_OK;
            }
        }
    }
    in_port.queue.insert(in_port.queue.end(), samples, samples + sample_count);
    if (in_port.queue.size() > kMaxQueuedSamples) {
        in_port.queue.erase(in_port.queue.begin(), in_port.queue.end() - kMaxQueuedSamples);
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oV9GAdJ23Gw", "libSceVoice", 1, "libSceVoice", sceVoiceConnectIPortToOPort);
    LIB_FUNCTION("nXpje5yNpaE", "libSceVoice", 1, "libSceVoice", sceVoiceCreatePort);
    LIB_FUNCTION("b7kJI+nx2hg", "libSceVoice", 1, "libSceVoice", sceVoiceDeletePort);
    LIB_FUNCTION("ajVj3QG2um4", "libSceVoice", 1, "libSceVoice", sceVoiceDisconnectIPortFromOPort);
    LIB_FUNCTION("Oo0S5PH7FIQ", "libSceVoice", 1, "libSceVoice", sceVoiceEnd);
    LIB_FUNCTION("cJLufzou6bc", "libSceVoice", 1, "libSceVoice", sceVoiceGetBitRate);
    LIB_FUNCTION("Pc4z1QjForU", "libSceVoice", 1, "libSceVoice", sceVoiceGetMuteFlag);
    LIB_FUNCTION("elcxZTEfHZM", "libSceVoice", 1, "libSceVoice", sceVoiceGetPortAttr);
    LIB_FUNCTION("CrLqDwWLoXM", "libSceVoice", 1, "libSceVoice", sceVoiceGetPortInfo);
    LIB_FUNCTION("Z6QV6j7igvE", "libSceVoice", 1, "libSceVoice", sceVoiceGetResourceInfo);
    LIB_FUNCTION("jjkCjneOYSs", "libSceVoice", 1, "libSceVoice", sceVoiceGetVolume);
    LIB_FUNCTION("9TrhuGzberQ", "libSceVoice", 1, "libSceVoice", sceVoiceInit);
    LIB_FUNCTION("IPHvnM5+g04", "libSceVoice", 1, "libSceVoice", sceVoiceInitHQ);
    LIB_FUNCTION("x0slGBQW+wY", "libSceVoice", 1, "libSceVoice", sceVoicePausePort);
    LIB_FUNCTION("Dinob0yMRl8", "libSceVoice", 1, "libSceVoice", sceVoicePausePortAll);
    LIB_FUNCTION("cQ6DGsQEjV4", "libSceVoice", 1, "libSceVoice", sceVoiceReadFromOPort);
    LIB_FUNCTION("udAxvCePkUs", "libSceVoice", 1, "libSceVoice", sceVoiceResetPort);
    LIB_FUNCTION("gAgN+HkiEzY", "libSceVoice", 1, "libSceVoice", sceVoiceResumePort);
    LIB_FUNCTION("jbkJFmOZ9U0", "libSceVoice", 1, "libSceVoice", sceVoiceResumePortAll);
    LIB_FUNCTION("TexwmOHQsDg", "libSceVoice", 1, "libSceVoice", sceVoiceSetBitRate);
    LIB_FUNCTION("gwUynkEgNFY", "libSceVoice", 1, "libSceVoice", sceVoiceSetMuteFlag);
    LIB_FUNCTION("oUha0S-Ij9Q", "libSceVoice", 1, "libSceVoice", sceVoiceSetMuteFlagAll);
    LIB_FUNCTION("clyKUyi3RYU", "libSceVoice", 1, "libSceVoice", sceVoiceSetThreadsParams);
    LIB_FUNCTION("QBFoAIjJoXQ", "libSceVoice", 1, "libSceVoice", sceVoiceSetVolume);
    LIB_FUNCTION("54phPH2LZls", "libSceVoice", 1, "libSceVoice", sceVoiceStart);
    LIB_FUNCTION("Ao2YNSA7-Qo", "libSceVoice", 1, "libSceVoice", sceVoiceStop);
    LIB_FUNCTION("jSZNP7xJrcw", "libSceVoice", 1, "libSceVoice", sceVoiceUpdatePort);
    LIB_FUNCTION("hg9T73LlRiU", "libSceVoice", 1, "libSceVoice", sceVoiceVADAdjustment);
    LIB_FUNCTION("wFeAxEeEi-8", "libSceVoice", 1, "libSceVoice", sceVoiceVADSetVersion);
    LIB_FUNCTION("YeJl6yDlhW0", "libSceVoice", 1, "libSceVoice", sceVoiceWriteToIPort);
};

} // namespace Libraries::Voice
