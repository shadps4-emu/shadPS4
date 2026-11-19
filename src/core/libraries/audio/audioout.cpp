// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/thread.h"
#include "core/emulator_settings.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"
#include "core/libraries/audio/audioout_error.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/pad/pad_errors.h"
#include "core/libraries/system/userservice.h"
#include "core/user_settings.h"

namespace Libraries::AudioOut {

// Port table with shared_ptr - use std::shared_mutex for RW locking
std::array<std::shared_ptr<PortOut>, ORBIS_AUDIO_OUT_NUM_PORTS> port_table{};
std::shared_mutex port_table_mutex;
std::mutex port_allocation_mutex;

static std::unique_ptr<AudioOutBackend> audio;
static std::atomic<int> lazy_init{0};

// Mastering library state.
struct MasteringState {
    std::mutex mutex;
    std::atomic<u32> generation{0};
    bool initialized = false;
    bool swap_eq_compressor = false;
    u32 params_id = ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_DEFAULT;
    OrbisAudioOutMasteringParamsV2 params{};
};
static MasteringState mastering;

static void ResetMasteringParams() {
    // Default setting after init: all modules bypassed (ORBIS_AUDIO_OUT_MASTERING_PRESET_OFF).
    mastering.params = {};
    mastering.params.paramsHeader.paramsId = ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_DEFAULT;
    mastering.params.moduleBypassFlags = ORBIS_AUDIO_OUT_MASTERING_BYPASSED_PARAM_EQ |
                                         ORBIS_AUDIO_OUT_MASTERING_BYPASSED_COMPRESSOR |
                                         ORBIS_AUDIO_OUT_MASTERING_BYPASSED_MASTER_VOLUME |
                                         ORBIS_AUDIO_OUT_MASTERING_BYPASSED_LIMITER;
    mastering.params_id = ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_DEFAULT;
    mastering.swap_eq_compressor = false;
}

// Port allocation ranges
constexpr struct PortRange {
    s32 start;
    s32 end;
    s32 count() const {
        return end - start + 1;
    }
} port_ranges[] = {
    {0, 7},   // MAIN
    {8, 8},   // BGM
    {9, 12},  // VOICE
    {13, 16}, // PERSONAL
    {17, 20}, // PADSPK
    {21, 21}, // Type 5-8
    {22, 22}, // Audio3d (126)
    {23, 23}, // AUX (127)
    {24, 24}, // Type 125
};

static std::atomic<u32> loudness_measurement{ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_STOP};

static AudioFormatInfo GetFormatInfo(const OrbisAudioOutParamFormat format) {
    static constexpr std::array<AudioFormatInfo, 8> format_infos = {{
        // S16Mono
        {false, 2, 1, {0}, false},
        // S16Stereo
        {false, 2, 2, {0, 1}, false},
        // S16_8CH
        {false, 2, 8, {0, 1, 2, 3, 4, 5, 6, 7}, false},
        // FloatMono
        {true, 4, 1, {0}, false},
        // FloatStereo
        {true, 4, 2, {0, 1}, false},
        // Float_8CH
        {true, 4, 8, {0, 1, 2, 3, 4, 5, 6, 7}, false},
        // S16_8CH_Std
        {false, 2, 8, {0, 1, 2, 3, 6, 7, 4, 5}, true},
        // Float_8CH_Std
        {true, 4, 8, {0, 1, 2, 3, 6, 7, 4, 5}, true},
    }};
    const auto index = static_cast<u32>(format);
    ASSERT_MSG(index < format_infos.size(), "Unknown audio format {}", index);
    return format_infos[index];
}

/*
 * Helper functions
 **/
static int GetPortRange(OrbisAudioOutPort type) {
    s32 _type = static_cast<s32>(type);

    switch (_type) {
    case 0:
        return 0; // MAIN
    case 1:
        return 1; // BGM
    case 2:
        return 2; // VOICE
    case 3:
        return 3; // PERSONAL
    case 4:
        return 4; // PADSPK
    case 5:
        return 5; // Type 5
    case 6:
        return 5; // Type 6
    case 7:
        return 5; // Type 7
    case 8:
        return 5; // Type 8
    case 126:
        return 6; // Audio3d
    case 125:
        return 8; // Type 125
    case 127:
        return 7; // AUX
    default:
        return -1;
    }
}

static int GetPortId(s32 handle) {
    const int port_id = handle & 0xFF;

    if (port_id >= ORBIS_AUDIO_OUT_NUM_PORTS) {
        LOG_DEBUG(Lib_AudioOut, "Invalid handle {:#x}: port index {} out of range (max {})", handle,
                  port_id, ORBIS_AUDIO_OUT_NUM_PORTS - 1);
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    if ((handle & 0x3F000000) != 0x20000000) {
        LOG_DEBUG(Lib_AudioOut, "Invalid handle {:#x}: bad magic {:#x}, expected {:#x}", handle,
                  handle & 0x3F000000, 0x20000000);
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    return port_id;
}

static s32 GetPortType(s32 handle) {
    return (handle >> 16) & 0xFF;
}

static bool IsValidHandlePortType(s32 handle) {
    const s32 port_type = GetPortType(handle);
    return (port_type >= 0 && port_type <= 4) || port_type == 14 || port_type == 126 ||
           port_type == 127;
}

static int AllocatePort(OrbisAudioOutPort type) {
    int range_idx = GetPortRange(type);
    if (range_idx < 0) {
        return -1;
    }

    const auto& range = port_ranges[range_idx];
    for (int i = range.start; i <= range.end; i++) {
        std::shared_lock read_lock{port_table_mutex};
        if (!port_table[i]) {
            return i;
        }
    }
    return -1;
}

void AdjustVol() {
    if (lazy_init.load(std::memory_order_relaxed) == 0 && audio == nullptr) {
        return;
    }

    std::shared_lock read_lock{port_table_mutex};
    for (int i = 0; i < ORBIS_AUDIO_OUT_NUM_PORTS; i++) {
        if (auto port = port_table[i]) {
            std::unique_lock lock{port->mutex, std::try_to_lock};
            if (lock.owns_lock()) {
                port->impl->SetVolume(port->volume);
            }
        }
    }
}

static void ApplyChannelBalance(void* buffer, u32 frames, const AudioFormatInfo& format,
                                const std::array<s32, 8>& volume) {
    const u32 num_channels = std::min<u32>(format.num_channels, ORBIS_AUDIO_OUT_CHANNEL_MAX);
    if (buffer == nullptr || num_channels < 2) {
        return;
    }

    const auto first = volume.begin();
    const auto last = first + num_channels;
    const s32 max_volume = *std::max_element(first, last);
    if (max_volume <= 0 || std::all_of(first, last, [&](s32 v) { return v == max_volume; })) {
        return;
    }

    std::array<float, ORBIS_AUDIO_OUT_CHANNEL_MAX> gain{};
    for (u32 pos = 0; pos < num_channels; pos++) {
        const u32 channel = (format.is_std && pos >= ORBIS_AUDIO_OUT_CHANNEL_LS) ? (pos ^ 2) : pos;
        gain[pos] = static_cast<float>(std::max(volume[channel], 0)) / max_volume;
    }

    if (format.is_float) {
        auto* samples = static_cast<float*>(buffer);
        for (u32 frame = 0; frame < frames; frame++) {
            for (u32 pos = 0; pos < num_channels; pos++) {
                *samples++ *= gain[pos];
            }
        }
    } else {
        auto* samples = static_cast<s16*>(buffer);
        for (u32 frame = 0; frame < frames; frame++) {
            for (u32 pos = 0; pos < num_channels; pos++) {
                *samples = static_cast<s16>(*samples * gain[pos]);
                samples++;
            }
        }
    }
}

static bool IsMasteredPort(OrbisAudioOutPort type) {
    return type == OrbisAudioOutPort::Main || type == OrbisAudioOutPort::Bgm ||
           type == OrbisAudioOutPort::Audio3d;
}

static void ApplyMastering(PortOut& port) {
    if (!IsMasteredPort(port.type)) {
        return;
    }

    const u32 generation = mastering.generation.load(std::memory_order_acquire);
    if (generation != port.mastering_generation) {
        std::scoped_lock lock{mastering.mutex};
        if (mastering.initialized) {
            port.mastering.SetParams(&mastering.params, mastering.params_id,
                                     mastering.swap_eq_compressor);
        } else {
            port.mastering.Disable();
        }
        port.mastering_generation = mastering.generation.load(std::memory_order_acquire);
    }

    if (!port.mastering.IsActive()) {
        return;
    }

    const u32 num_channels =
        std::min<u32>(port.format_info.num_channels, ORBIS_AUDIO_OUT_CHANNEL_MAX);
    const s32 max_volume =
        *std::max_element(port.volume.begin(), port.volume.begin() + num_channels);
    const float pre_gain = static_cast<float>(max_volume) / ORBIS_AUDIO_OUT_VOLUME_0DB;

    port.mastering.Process(port.output_buffer, port.buffer_frames, num_channels,
                           port.format_info.is_float, port.format_info.is_std, pre_gain);
}

static void ApplyGain(void* buffer, u32 frames, const AudioFormatInfo& format, float gain) {
    if (buffer == nullptr || gain == 1.0f) {
        return;
    }
    const u32 num_samples = frames * format.num_channels;
    if (format.is_float) {
        auto* samples = static_cast<float*>(buffer);
        for (u32 i = 0; i < num_samples; i++) {
            samples[i] *= gain;
        }
    } else {
        auto* samples = static_cast<s16*>(buffer);
        for (u32 i = 0; i < num_samples; i++) {
            samples[i] = static_cast<s16>(samples[i] * gain);
        }
    }
}

static void AudioOutputThread(std::shared_ptr<PortOut> port, const std::stop_token& stop) {
    {
        const auto thread_name = fmt::format("shadPS4:AudioOutputThread:{}", fmt::ptr(port.get()));
        Common::SetCurrentThreadName(thread_name.c_str());
    }

    Common::AccurateTimer timer(
        std::chrono::nanoseconds(1000000000ULL * port->buffer_frames / port->sample_rate));

    while (true) {
        timer.Start();

        {
            std::unique_lock lock{port->mutex};
            if (!port->impl || stop.stop_requested()) {
                break;
            }

            if (port->output_ready) {
                ApplyChannelBalance(port->output_buffer, port->buffer_frames, port->format_info,
                                    port->volume);
                ApplyMastering(*port);
                if (port->padspk_mixed_to_main) {
                    // PADSPK audio mixed with MAIN is attenuated by the mix level.
                    const float mix_gain =
                        std::clamp(port->mixLevelPadSpk, 0, ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_0DB) /
                        static_cast<float>(ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_0DB);
                    ApplyGain(port->output_buffer, port->buffer_frames, port->format_info,
                              mix_gain);
                }
                port->impl->Output(port->output_buffer);
                port->output_ready = false;
                port->last_output_time =
                    Kernel::sceKernelGetProcessTime(); // moved from sceAudioOutOutput TOOD recheck
            }
        }

        port->output_cv.notify_all();

        if (stop.stop_requested()) {
            break;
        }

        timer.End();
    }

    {
        std::unique_lock lock{port->mutex};
        port->closing = true;
        port->output_ready = false;
    }
    port->output_cv.notify_all();
}

/*
 * sceAudioOut implementation
 **/
s32 PS4_SYSV_ABI sceAudioOutInit() {
    LOG_TRACE(Lib_AudioOut, "called");

    int expected = 0;
    if (!lazy_init.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
        return ORBIS_AUDIO_OUT_ERROR_ALREADY_INIT;
    }

    if (EmulatorSettings.GetAudioBackend() == AudioBackend::OpenAL) {
        audio = std::make_unique<OpenALAudioOut>();
    } else {
        audio = std::make_unique<SDLAudioOut>();
    }

    LOG_INFO(Lib_AudioOut, "Audio system initialized");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutOpen(UserService::OrbisUserServiceUserId user_id,
                                 OrbisAudioOutPort port_type, s32 index, u32 length,
                                 u32 sample_rate,
                                 OrbisAudioOutParamExtendedInformation param_type) {
    LOG_INFO(Lib_AudioOut,
             "called, user_id={}, port_type={}({}), index={}, length={}, "
             "sample_rate={}, data_format={}({}), attributes={}({})",
             user_id, magic_enum::enum_name(port_type), static_cast<u32>(port_type), index, length,
             sample_rate, magic_enum::enum_name(param_type.data_format.Value()),
             static_cast<u32>(param_type.data_format.Value()),
             magic_enum::enum_name(param_type.attributes.Value()),
             static_cast<u32>(param_type.attributes.Value()));

    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "Audio out not initialized");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    if (length == 0 || length > 2048 || (length & 0xFF) != 0) {
        LOG_ERROR(Lib_AudioOut, "Invalid size");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_SIZE;
    }

    s32 _type = static_cast<s32>(port_type);
    u32 param_raw = param_type.Unpack();

    // Extract attributes
    bool is_restricted = (param_raw & ORBIS_AUDIO_OUT_PARAM_ATTR_RESTRICTED) != 0;
    bool is_mix_to_main = (param_raw & ORBIS_AUDIO_OUT_PARAM_ATTR_MIX_TO_MAIN) != 0;

    if (_type != 3 && is_mix_to_main) {
        LOG_ERROR(Lib_AudioOut, "Invalid format");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_FORMAT;
    }

    if (_type != 0 && (param_raw & 0x70000000) != 0) {
        LOG_ERROR(Lib_AudioOut, "Invalid format");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_FORMAT;
    }

    if ((port_type < OrbisAudioOutPort::Main || port_type > OrbisAudioOutPort::PadSpk) &&
        (port_type != OrbisAudioOutPort::Audio3d && port_type != OrbisAudioOutPort::Aux)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }
    if (sample_rate != 48000) {
        LOG_ERROR(Lib_AudioOut, "Invalid sample rate");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_SAMPLE_FREQ;
    }

    if (index != 0) {
        LOG_ERROR(Lib_AudioOut, "index is not valid !=0 {}", index);
    }

    const auto format = param_type.data_format.Value();
    if (format < OrbisAudioOutParamFormat::S16Mono ||
        format > OrbisAudioOutParamFormat::Float_8CH_Std) {
        LOG_ERROR(Lib_AudioOut, "Invalid format");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_FORMAT;
    }
    constexpr u32 known_attrs =
        ORBIS_AUDIO_OUT_PARAM_ATTR_RESTRICTED | ORBIS_AUDIO_OUT_PARAM_ATTR_MIX_TO_MAIN;
    if ((param_raw & ORBIS_AUDIO_OUT_PARAM_ATTR_MASK & ~known_attrs) != 0) {
        // TODO Handle attributes in output audio device
        LOG_ERROR(Lib_AudioOut, "Invalid format attribute");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_FORMAT;
    }
    const bool is_user_port = port_type == OrbisAudioOutPort::Voice ||
                              port_type == OrbisAudioOutPort::Personal ||
                              port_type == OrbisAudioOutPort::PadSpk;
    const bool is_shared_port = port_type == OrbisAudioOutPort::Main ||
                                port_type == OrbisAudioOutPort::Bgm ||
                                port_type == OrbisAudioOutPort::Aux;
    if (is_user_port) {
        if (user_id < 0 || user_id == UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
            LOG_ERROR(Lib_AudioOut, "Invalid user id {} for per-user port type {}", user_id,
                      magic_enum::enum_name(port_type));
            return ORBIS_DEVICE_SERVICE_ERROR_INVALID_USER;
        }
        if (UserManagement.GetUserByID(user_id) == nullptr) {
            LOG_ERROR(Lib_AudioOut, "User {} is not logged in", user_id);
            return ORBIS_DEVICE_SERVICE_ERROR_USER_NOT_LOGIN;
        }
    } else if (is_shared_port && user_id != UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        LOG_WARNING(Lib_AudioOut, "Shared port type {} opened with user id {} instead of SYSTEM",
                    magic_enum::enum_name(port_type), user_id);
    }

    std::unique_lock lock{port_allocation_mutex};

    // Allocate port
    int port_id = AllocatePort(port_type);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioOut, "Error allocated port");
        return ORBIS_AUDIO_OUT_ERROR_PORT_FULL;
    }

    // Create port object
    std::shared_ptr<PortOut> port;
    try {
        port = std::make_shared<PortOut>();

        port->userId = user_id;
        port->type = static_cast<OrbisAudioOutPort>(_type);
        port->format_info = GetFormatInfo(param_type.data_format.Value());
        port->sample_rate = sample_rate;
        port->buffer_frames = length;
        port->volume.fill(ORBIS_AUDIO_OUT_VOLUME_0DB);
        port->mixLevelPadSpk = ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_DEFAULT;

        // Set attributes
        port->is_restricted = is_restricted;
        port->is_mix_to_main = is_mix_to_main;

        // Log attributes if present
        if (is_restricted) {
            LOG_INFO(Lib_AudioOut, "Audio port opened with RESTRICTED attribute");
        }
        if (is_mix_to_main) {
            LOG_INFO(Lib_AudioOut, "Audio port opened with MIX_TO_MAIN attribute");
        }

        // Create backend
        port->impl = audio->Open(*port);
        if (!port->impl) {
            throw std::runtime_error("Failed to create audio backend");
        }

        // A PADSPK port without a dedicated controller speaker device is mixed with MAIN.
        port->padspk_mixed_to_main =
            port->type == OrbisAudioOutPort::PadSpk && port->impl->SharesMainDevice();
        if (port->padspk_mixed_to_main) {
            LOG_INFO(Lib_AudioOut, "PADSPK port has no dedicated device: mixing with MAIN");
        }

        // Allocate buffer
        port->output_buffer = std::malloc(port->BufferSize());
        if (!port->output_buffer) {
            throw std::bad_alloc();
        }

        // Start output thread - pass shared_ptr by value to keep port alive
        port->output_thread.Run([port](std::stop_token stop) { AudioOutputThread(port, stop); });

        // Set initial volume
        port->impl->SetVolume(port->volume);

    } catch (const std::bad_alloc&) {
        return ORBIS_AUDIO_OUT_ERROR_MEMORY;
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_AudioOut, "Failed to open audio port: {}", e.what());
        return ORBIS_AUDIO_OUT_ERROR_TRANS_EVENT;
    }

    {
        std::unique_lock write_lock{port_table_mutex};
        port_table[port_id] = port;
    }

    // Create handle
    s32 handle = (_type << 16) | port_id | 0x20000000;
    LOG_INFO(Lib_AudioOut, "opened port_type={}({}) -> port_id={}, handle={:#x}",
             magic_enum::enum_name(port_type), _type, port_id, handle);
    return handle;
}

static s32 ClosePort(s32 handle, bool force) {
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "Audio out not initialized");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioOut, "Invalid port id for handle {:#x}", handle);
        return port_id;
    }

    s32 port_type = GetPortType(handle);
    if (port_type >= 5 && port_type <= 13) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    // Check valid types
    if (!((port_type >= 0 && port_type <= 4) || port_type == 14 || port_type == 126 ||
          port_type == 127)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    std::unique_lock lock{port_allocation_mutex};

    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port wasn't open {}", port_id);
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    {
        std::unique_lock port_lock{port->mutex};
        if (!force && port->output_waiters > 0) {
            LOG_ERROR(Lib_AudioOut, "Port {} is busy outputting, cannot close", port_id);
            return ORBIS_AUDIO_OUT_ERROR_BUSY;
        }
        port->closing = true;
        port->output_ready = false;
    }
    port->output_cv.notify_all();

    {
        std::unique_lock write_lock{port_table_mutex};
        port_table[port_id].reset();
    }

    // Stop the output thread.
    port->output_thread.Stop();

    {
        std::unique_lock port_lock{port->mutex};
        std::free(port->output_buffer);
        port->output_buffer = nullptr;
    }

    LOG_DEBUG(Lib_AudioOut, "Closed audio port {}", port_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutClose(s32 handle) {
    LOG_INFO(Lib_AudioOut, "handle = {:#x}", handle);
    return ClosePort(handle, false);
}

s32 ForceClosePort(s32 handle) {
    LOG_INFO(Lib_AudioOut, "handle = {:#x}", handle);
    return ClosePort(handle, true);
}

s32 PS4_SYSV_ABI sceAudioOutGetLastOutputTime(s32 handle, u64* output_time) {
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_DEBUG(Lib_AudioOut, "Invalid port id for handle {:#x}", handle);
        return port_id;
    }

    if (!IsValidHandlePortType(handle)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    if (!output_time) {
        LOG_ERROR(Lib_AudioOut, "Invalid pointer");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }

    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port not opened {}", port_id);
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    std::unique_lock lock{port->mutex};
    *output_time = port->last_output_time;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetPortState(s32 handle, OrbisAudioOutPortState* state) {
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_DEBUG(Lib_AudioOut, "Invalid port id for handle {:#x}", handle);
        return port_id;
    }

    if (!IsValidHandlePortType(handle)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    if (!state) {
        LOG_ERROR(Lib_AudioOut, "Invalid pointer");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }

    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port is not open {}", port_id);
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    std::unique_lock lock{port->mutex};

    const u8 format_channels = port->format_info.num_channels;

    const auto shared_device_channels = [&]() -> u8 {
        if (format_channels <= 2) {
            return format_channels;
        }
        const u32 out = port->impl ? port->impl->GetOutputChannels() : 0;
        return (out == 6 || out == 8) ? static_cast<u8>(out) : u8{2};
    };

    bool reports_external = false;

    state->volume = -1; // only meaningful for PADSPK

    switch (port->type) {
    case OrbisAudioOutPort::Main:
    case OrbisAudioOutPort::Bgm:
        state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_PRIMARY;
        state->channel = shared_device_channels();
        reports_external = true;
        break;
    case OrbisAudioOutPort::Audio3d:
        state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_PRIMARY;
        state->channel = shared_device_channels();
        break;
    case OrbisAudioOutPort::Voice:
    case OrbisAudioOutPort::Personal:
        state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_HEADPHONE;
        state->channel = format_channels;
        reports_external = true;
        break;
    case OrbisAudioOutPort::PadSpk:
        if (port->padspk_mixed_to_main) {
            state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_PRIMARY;
        } else {
            state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_TERTIARY;
            state->volume = 127; // max
        }
        state->channel = 1;
        reports_external = true;
        break;
    case OrbisAudioOutPort::Aux:
        state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_UNKNOWN;
        state->channel = 0; // ORBIS_AUDIO_OUT_STATE_CHANNEL_UNKNOWN
        break;
    default:
        state->output = ORBIS_AUDIO_OUT_STATE_OUTPUT_UNKNOWN;
        state->channel = 0;
        break;
    }

    if (reports_external && !port->is_restricted) {
        state->output |= ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_EXTERNAL;
    }

    state->reserved8_1[0] = 0;
    state->reserved64[0] = 0;
    state->reserved64[1] = 0;
    state->rerouteCounter = 0;
    state->flag = 0;
    LOG_DEBUG(Lib_AudioOut,
              "called, handle={:#x}, state={}, output={}, channel={}, volume={}, "
              "rerouteCounter={}, flag={}",
              handle, fmt::ptr(state), state->output, state->channel, state->volume,
              state->rerouteCounter, state->flag);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutOutput(s32 handle, void* ptr) {
    LOG_TRACE(Lib_AudioOut, "called, handle={:#x}, ptr={}", handle, fmt::ptr(ptr));

    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioOut, "invalid port id for handle {:#x}", handle);
        return port_id;
    }

    s32 port_type = GetPortType(handle);
    if (port_type >= 5 && port_type <= 13) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    // Check valid types
    if (!((port_type >= 0 && port_type <= 4) || port_type == 14 || port_type == 126 ||
          port_type == 127)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port is not opened {}", port_id);
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    s32 samples_sent = 0;
    {
        std::unique_lock lock{port->mutex};
        port->output_waiters++;
        port->output_cv.wait(lock, [&] { return !port->output_ready || port->closing; });
        port->output_waiters--;

        if (port->closing) {
            LOG_DEBUG(Lib_AudioOut, "Port {} closed while waiting for drain", port_id);
            return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
        }

        if (ptr != nullptr) {
            std::memcpy(port->output_buffer, ptr, port->BufferSize());
            port->output_ready = true;
            samples_sent = port->buffer_frames;
        }
    }

    return samples_sent;
}

s32 PS4_SYSV_ABI sceAudioOutOutputs(OrbisAudioOutOutputParam* param, u32 num) {
    if (param) {
        LOG_TRACE(Lib_AudioOut, "called, param={}, num={}", fmt::ptr(param), num);
        for (u32 i = 0; i < num; i++) {
            LOG_TRACE(Lib_AudioOut, "  [{}] handle={:#x}, ptr={}", i, param[i].handle,
                      fmt::ptr(param[i].ptr));
        }
    } else {
        LOG_TRACE(Lib_AudioOut, "called, param=nullptr, num={}", num);
    }
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    if (num == 0 || num > 25) {
        LOG_ERROR(Lib_AudioOut, "ports is full");
        return ORBIS_AUDIO_OUT_ERROR_PORT_FULL;
    }

    if (!param) {
        LOG_ERROR(Lib_AudioOut, "invalid pointer");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }

    std::vector<std::shared_ptr<PortOut>> ports;
    std::vector<std::unique_lock<std::mutex>> locks;
    ports.reserve(num);
    locks.reserve(num);

    u32 buffer_frames = 0;

    {
        std::shared_lock read_lock{port_table_mutex};

        for (u32 i = 0; i < num; i++) {
            int port_id = GetPortId(param[i].handle);
            if (port_id < 0) {
                LOG_ERROR(Lib_AudioOut, "invalid port id");
                return port_id;
            }

            s32 port_type = GetPortType(param[i].handle);
            if (port_type >= 5 && port_type <= 13) {
                LOG_ERROR(Lib_AudioOut, "Invalid port type");
                return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
            }

            // Check valid types
            if (!((port_type >= 0 && port_type <= 4) || port_type == 14 || port_type == 126 ||
                  port_type == 127)) {
                LOG_ERROR(Lib_AudioOut, "Invalid port type");
                return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
            }

            // Check for duplicate handles
            for (u32 j = 0; j < i; j++) {
                if (param[i].handle == param[j].handle) {
                    LOG_ERROR(Lib_AudioOut, "Duplicate audio handles: {:#x}", param[i].handle);
                    return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
                }
            }

            // Get port
            auto port = port_table[port_id];
            if (!port) {
                LOG_ERROR(Lib_AudioOut, "Port not opened {}", port_id);
                return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
            }

            ports.push_back(port);
            locks.emplace_back(port->mutex);

            // Check consistent buffer size
            if (i == 0) {
                buffer_frames = port->buffer_frames;
            } else if (port->buffer_frames != buffer_frames) {
                LOG_ERROR(Lib_AudioOut, "Invalid port size");
                return ORBIS_AUDIO_OUT_ERROR_INVALID_SIZE;
            }
        }
    }

    // Wait for all ports to be ready
    for (u32 i = 0; i < num; i++) {
        ports[i]->output_waiters++;
        ports[i]->output_cv.wait(locks[i],
                                 [&] { return !ports[i]->output_ready || ports[i]->closing; });
        ports[i]->output_waiters--;

        if (ports[i]->closing) {
            LOG_DEBUG(Lib_AudioOut, "Port closed while waiting for drain");
            return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
        }
    }

    // Copy data to all ports
    for (u32 i = 0; i < num; i++) {
        if (param[i].ptr != nullptr) {
            std::memcpy(ports[i]->output_buffer, param[i].ptr, ports[i]->BufferSize());
            ports[i]->output_ready = true;
        }
    }

    return buffer_frames;
}

s32 PS4_SYSV_ABI sceAudioOutSetVolume(s32 handle, s32 flag, s32* vol) {
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioOut, "invalid port_id");
        return port_id;
    }

    s32 port_type = GetPortType(handle);
    if (port_type >= 5 && port_type <= 13) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    // Check valid types
    if (!((port_type >= 0 && port_type <= 4) || port_type == 14 || port_type == 126 ||
          port_type == 127)) {
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    if (!vol) {
        LOG_ERROR(Lib_AudioOut, "Invalid pointer");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }

    // Get port with shared lock (read-only access to table)
    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port not opened {}", port_id);
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    std::unique_lock lock{port->mutex};

    const u32 num_channels =
        std::min<u32>(port->format_info.num_channels, ORBIS_AUDIO_OUT_CHANNEL_MAX);
    const bool is_std = port->format_info.is_std;
    const auto channel_for_bit = [is_std](u32 bit) -> u32 {
        if (is_std && bit >= ORBIS_AUDIO_OUT_CHANNEL_LS) {
            return bit ^ 2; // 4<->6, 5<->7
        }
        return bit;
    };

    for (u32 bit = 0; bit < num_channels; bit++) {
        if ((flag & (1u << bit)) == 0) {
            continue;
        }
        const s32 value = vol[channel_for_bit(bit)];
        if (value < 0 || value > ORBIS_AUDIO_OUT_VOLUME_0DB) {
            LOG_ERROR(Lib_AudioOut, "Invalid volume {} for channel {}", value,
                      channel_for_bit(bit));
            return ORBIS_AUDIO_OUT_ERROR_INVALID_VOLUME;
        }
    }

    for (u32 bit = 0; bit < num_channels; bit++) {
        if ((flag & (1u << bit)) == 0) {
            continue;
        }
        const u32 channel = channel_for_bit(bit);
        port->volume[channel] = vol[channel];
    }

    port->impl->SetVolume(port->volume);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetMixLevelPadSpk(s32 handle, s32 mixLevel) {
    LOG_INFO(Lib_AudioOut, "called, handle = {:#x}, mixLevel = {}", handle, mixLevel);
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioOut, "Invalid port_id");
        return port_id;
    }

    if (GetPortType(handle) != 4) { // PadSpk
        LOG_ERROR(Lib_AudioOut, "Invalid port type");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT_TYPE;
    }

    if (mixLevel < 0 || mixLevel > ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_0DB) {
        LOG_ERROR(Lib_AudioOut, "Invalid mix level");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_MIXLEVEL;
    }

    std::shared_ptr<PortOut> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        port = port_table[port_id];
    }

    if (!port) {
        LOG_ERROR(Lib_AudioOut, "Port not opened");
        return ORBIS_AUDIO_OUT_ERROR_NOT_OPENED;
    }

    std::unique_lock lock{port->mutex};
    // Applied by the output thread whenever the port is mixed with MAIN.
    port->mixLevelPadSpk = mixLevel;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetSystemState(OrbisAudioOutSystemState* state) {
    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }
    if (state == nullptr) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }
    memset(state, 0, sizeof(*state));
    if (loudness_measurement.load(std::memory_order_relaxed) !=
        ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_STOP) {
        state->loudness = ORBIS_AUDIO_OUT_STATE_LOUDNESS_MIN;
    }
    LOG_DEBUG(Lib_AudioOut, "called");
    return ORBIS_OK;
}

/*
 * Stubbed functions
 **/
s32 PS4_SYSV_ABI sceAudioOutDeviceIdOpen() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioDeviceControlGet() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioDeviceControlSet() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutA3dControl() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutA3dExit() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutA3dInit() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutAttachToApplicationByPid() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutChangeAppModuleState() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutDetachFromApplicationByPid() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExConfigureOutputMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExGetSystemInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExPtClose() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExPtGetLastOutputTime() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExPtOpen() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutExSystemInfoIsSupportedAudioOutExMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetFocusEnablePid() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetHandleStatusInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetInfoOpenNum() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutOpenEx() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetSimulatedBusUsableStatusByBusType() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo2() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutGetSparkVss() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutInitIpmiGetSession() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutMasteringGetState(OrbisAudioOutMasteringStatesHeader* state) {
    LOG_DEBUG(Lib_AudioOut, "called, state = {}", fmt::ptr(state));

    if (state == nullptr) {
        LOG_ERROR(Lib_AudioOut, "Invalid state pointer");
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    }

    {
        std::scoped_lock lock{mastering.mutex};
        if (!mastering.initialized) {
            LOG_ERROR(Lib_AudioOut, "Mastering library is not initialized");
            return ORBIS_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED;
        }
    }

    if (state->statesId != ORBIS_AUDIO_OUT_MASTERING_STATES_ID_DEFAULT) {
        LOG_ERROR(Lib_AudioOut, "Invalid states id {}", state->statesId);
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_INVALID_STATES_ID;
    }

    auto* states = reinterpret_cast<OrbisAudioOutMasteringStates*>(state);
    std::memset(states->reserved, 0, sizeof(states->reserved));

    // Start from an idle chain: no measured signal and unity gain everywhere.
    auto& compressor = states->compressorStates;
    compressor = {};
    compressor.descriptor.id = ORBIS_AUDIO_OUT_MASTERING_STATES_STRUCT_ID_COMPRESSOR_DEFAULT;
    compressor.descriptor.size = sizeof(OrbisAudioOutMasteringCompressorStates);
    for (auto& band : compressor.compressionCoeff) {
        std::fill(std::begin(band), std::end(band), 1.0f);
    }

    auto& limiter = states->limiterStates;
    limiter = {};
    limiter.descriptor.id = ORBIS_AUDIO_OUT_MASTERING_STATES_STRUCT_ID_LIMITER_DEFAULT;
    limiter.descriptor.size = sizeof(OrbisAudioOutMasteringLimiterStates);
    std::fill(std::begin(limiter.gainPeak), std::end(limiter.gainPeak), 1.0f);

    std::array<std::shared_ptr<PortOut>, ORBIS_AUDIO_OUT_NUM_PORTS> ports;
    {
        std::shared_lock read_lock{port_table_mutex};
        ports = port_table;
    }
    for (const auto& port : ports) {
        if (!port || !IsMasteredPort(port->type)) {
            continue;
        }
        const auto meters = port->mastering.GetMeters();
        if (!meters.valid) {
            continue;
        }
        for (u32 band = 0; band < MasteringProcessor::MaxCompressorBands; band++) {
            for (u32 ch = 0; ch < MasteringProcessor::MaxChannels; ch++) {
                compressor.inputRms[band][ch] =
                    std::hypot(compressor.inputRms[band][ch], meters.input_rms[band][ch]);
                compressor.compressionCoeff[band][ch] = std::min(
                    compressor.compressionCoeff[band][ch], meters.compression_coeff[band][ch]);
            }
        }
        for (u32 ch = 0; ch < MasteringProcessor::MaxChannels; ch++) {
            limiter.inputPeak[ch] = std::max(limiter.inputPeak[ch], meters.input_peak[ch]);
            limiter.outputPeak[ch] = std::max(limiter.outputPeak[ch], meters.output_peak[ch]);
            limiter.gainPeak[ch] = std::min(limiter.gainPeak[ch], meters.gain_peak[ch]);
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutMasteringInit(u32 flags) {
    LOG_INFO(Lib_AudioOut, "called, flags = {:#x}", flags);
    if (flags != 0) {
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    }

    std::scoped_lock lock{mastering.mutex};
    mastering.initialized = true;
    ResetMasteringParams();
    mastering.generation.fetch_add(1, std::memory_order_release);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutMasteringSetParam(const OrbisAudioOutMasteringParamsHeader* param,
                                              u32 flags) {
    LOG_INFO(Lib_AudioOut, "called, param = {}, flags = {:#x}", fmt::ptr(param), flags);

    if (param == nullptr || (flags & ~ORBIS_AUDIO_OUT_MASTERING_MODULE_FLAG_SWAP_EQ_COMPRESSOR)) {
        LOG_ERROR(Lib_AudioOut, "Invalid param pointer or flags");
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    }

    std::scoped_lock lock{mastering.mutex};
    if (!mastering.initialized) {
        LOG_ERROR(Lib_AudioOut, "Mastering library is not initialized");
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED;
    }

    size_t size = 0;
    switch (param->paramsId) {
    case ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_DEFAULT:
        size = sizeof(OrbisAudioOutMasteringParams);
        break;
    case ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_V2:
        size = sizeof(OrbisAudioOutMasteringParamsV2);
        break;
    default:
        LOG_ERROR(Lib_AudioOut, "Invalid params id {}", param->paramsId);
        return ORBIS_AUDIO_OUT_ERROR_MASTERING_INVALID_CONFIG;
    }

    mastering.params = {};
    std::memcpy(&mastering.params, param, size);
    mastering.params_id = param->paramsId;
    mastering.swap_eq_compressor =
        (flags & ORBIS_AUDIO_OUT_MASTERING_MODULE_FLAG_SWAP_EQ_COMPRESSOR) != 0;

    mastering.generation.fetch_add(1, std::memory_order_release);

    // Both versions share the layout up to and including moduleBypassFlags.
    LOG_INFO(Lib_AudioOut, "Mastering params set (id = {}, bypass = {:#x}, swap = {})",
             param->paramsId, mastering.params.moduleBypassFlags, mastering.swap_eq_compressor);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutMasteringTerm() {
    LOG_INFO(Lib_AudioOut, "called");
    std::scoped_lock lock{mastering.mutex};
    mastering.initialized = false;
    mastering.generation.fetch_add(1, std::memory_order_release);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutMbusInit() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutPtClose() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutPtGetLastOutputTime() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutPtOpen() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetConnections() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetConnectionsForUser() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetDevConnection() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetHeadphoneOutMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetJediJackVolume() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetJediSpkVolume() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetMainOutput() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetMorpheusParam() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetMorpheusWorkingMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetPortConnections() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetPortStatuses() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetRecMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetSparkParam() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetUsbVolume() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetVolumeDown() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutStartAuxBroadcast() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutStartSharePlay() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutStopAuxBroadcast() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutStopSharePlay() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSuspendResume() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSysConfigureOutputMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSysGetHdmiMonitorInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSysGetSystemInfo() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSysHdmiMonitorInfoIsSupportedAudioOutMode() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSystemControlGet() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSystemControlSet() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSparkControlSetEqCoef() {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudioOutSetSystemDebugState(OrbisAudioOutSystemDebugStateElement element,
                                                OrbisAudioOutSystemDebugStateParam* param) {
    LOG_INFO(Lib_AudioOut, "called, element = {}, param = {}", static_cast<u32>(element),
             fmt::ptr(param));

    if (lazy_init.load(std::memory_order_relaxed) == 0 || audio == nullptr) {
        LOG_ERROR(Lib_AudioOut, "audio is not init");
        return ORBIS_AUDIO_OUT_ERROR_NOT_INIT;
    }

    if (param == nullptr) {
        LOG_ERROR(Lib_AudioOut, "Invalid pointer");
        return ORBIS_AUDIO_OUT_ERROR_INVALID_POINTER;
    }

    if (element != OrbisAudioOutSystemDebugStateElement::Loudness) {
        LOG_ERROR(Lib_AudioOut, "Invalid debug state element {}", static_cast<u32>(element));
        return ORBIS_AUDIO_OUT_ERROR_INVALID_ARG;
    }

    if (param->value > ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_START_QUIET) {
        LOG_ERROR(Lib_AudioOut, "Invalid loudness debug state {}", param->value);
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PARAM;
    }

    loudness_measurement.store(param->value, std::memory_order_relaxed);
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("cx2dYFbzIAg", "libSceAudioOutDeviceService", 1, "libSceAudioOut",
                 sceAudioOutDeviceIdOpen);
    LIB_FUNCTION("tKumjQSzhys", "libSceAudioDeviceControl", 1, "libSceAudioOut",
                 sceAudioDeviceControlGet);
    LIB_FUNCTION("5ChfcHOf3SM", "libSceAudioDeviceControl", 1, "libSceAudioOut",
                 sceAudioDeviceControlSet);
    LIB_FUNCTION("Iz9X7ISldhs", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutA3dControl);
    LIB_FUNCTION("9RVIoocOVAo", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutA3dExit);
    LIB_FUNCTION("n7KgxE8rOuE", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutA3dInit);
    LIB_FUNCTION("WBAO6-n0-4M", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutAttachToApplicationByPid);
    LIB_FUNCTION("O3FM2WXIJaI", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutChangeAppModuleState);
    LIB_FUNCTION("s1--uE9mBFw", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutClose);
    LIB_FUNCTION("ol4LbeTG8mc", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutDetachFromApplicationByPid);
    LIB_FUNCTION("r1V9IFEE+Ts", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutExConfigureOutputMode);
    LIB_FUNCTION("wZakRQsWGos", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutExGetSystemInfo);
    LIB_FUNCTION("xjjhT5uw08o", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutExPtClose);
    LIB_FUNCTION("DsST7TNsyfo", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutExPtGetLastOutputTime);
    LIB_FUNCTION("4UlW3CSuCa4", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutExPtOpen);
    LIB_FUNCTION("Xcj8VTtnZw0", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutExSystemInfoIsSupportedAudioOutExMode);
    LIB_FUNCTION("I3Fwcmkg5Po", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetFocusEnablePid);
    LIB_FUNCTION("Y3lXfCFEWFY", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetHandleStatusInfo);
    LIB_FUNCTION("-00OAutAw+c", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutGetInfo);
    LIB_FUNCTION("RqmKxBqB8B4", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutGetInfoOpenNum);
    LIB_FUNCTION("Ptlts326pds", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetLastOutputTime);
    LIB_FUNCTION("GrQ9s4IrNaQ", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutGetPortState);
    LIB_FUNCTION("c7mVozxJkPU", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetSimulatedBusUsableStatusByBusType);
    LIB_FUNCTION("pWmS7LajYlo", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetSimulatedHandleStatusInfo);
    LIB_FUNCTION("oPLghhAWgMM", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutGetSimulatedHandleStatusInfo2);
    LIB_FUNCTION("5+r7JYHpkXg", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutGetSparkVss);
    LIB_FUNCTION("R5hemoKKID8", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutGetSystemState);
    LIB_FUNCTION("JfEPXVxhFqA", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutInit);
    LIB_FUNCTION("n16Kdoxnvl0", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutInitIpmiGetSession);
    LIB_FUNCTION("r+qKw+ueD+Q", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutMasteringGetState);
    LIB_FUNCTION("xX4RLegarbg", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutMasteringInit);
    LIB_FUNCTION("4055yaUg3EY", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutMasteringSetParam);
    LIB_FUNCTION("RVWtUgoif5o", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutMasteringTerm);
    LIB_FUNCTION("-LXhcGARw3k", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutMbusInit);
    LIB_FUNCTION("ekNvsT22rsY", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutOpen);
    LIB_FUNCTION("qLpSK75lXI4", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutOpenEx);
    LIB_FUNCTION("QOQtbeDqsT4", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutOutput);
    LIB_FUNCTION("w3PdaSTSwGE", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutOutputs);
    LIB_FUNCTION("MapHTgeogbk", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutPtClose);
    LIB_FUNCTION("YZaq+UKbriQ", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutPtGetLastOutputTime);
    LIB_FUNCTION("xyT8IUCL3CI", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutPtOpen);
    LIB_FUNCTION("o4OLQQqqA90", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetConnections);
    LIB_FUNCTION("QHq2ylFOZ0k", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetConnectionsForUser);
    LIB_FUNCTION("r9KGqGpwTpg", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetDevConnection);
    LIB_FUNCTION("08MKi2E-RcE", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetHeadphoneOutMode);
    LIB_FUNCTION("18IVGrIQDU4", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetJediJackVolume);
    LIB_FUNCTION("h0o+D4YYr1k", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetJediSpkVolume);
    LIB_FUNCTION("KI9cl22to7E", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetMainOutput);
    LIB_FUNCTION("wVwPU50pS1c", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetMixLevelPadSpk);
    LIB_FUNCTION("eeRsbeGYe20", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetMorpheusParam);
    LIB_FUNCTION("IZrItPnflBM", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetMorpheusWorkingMode);
    LIB_FUNCTION("Gy0ReOgXW00", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetPortConnections);
    LIB_FUNCTION("oRBFflIrCg0", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetPortStatuses);
    LIB_FUNCTION("ae-IVPMSWjU", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetRecMode);
    LIB_FUNCTION("d3WL2uPE1eE", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetSparkParam);
    LIB_FUNCTION("X7Cfsiujm8Y", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetUsbVolume);
    LIB_FUNCTION("b+uAV89IlxE", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetVolume);
    LIB_FUNCTION("rho9DH-0ehs", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSetVolumeDown);
    LIB_FUNCTION("I91P0HAPpjw", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutStartAuxBroadcast);
    LIB_FUNCTION("uo+eoPzdQ-s", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutStartSharePlay);
    LIB_FUNCTION("AImiaYFrKdc", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutStopAuxBroadcast);
    LIB_FUNCTION("teCyKKZPjME", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutStopSharePlay);
    LIB_FUNCTION("95bdtHdNUic", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSuspendResume);
    LIB_FUNCTION("oRJZnXxok-M", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSysConfigureOutputMode);
    LIB_FUNCTION("Tf9-yOJwF-A", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSysGetHdmiMonitorInfo);
    LIB_FUNCTION("y2-hP-KoTMI", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSysGetSystemInfo);
    LIB_FUNCTION("YV+bnMvMfYg", "libSceAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSysHdmiMonitorInfoIsSupportedAudioOutMode);
    LIB_FUNCTION("JEHhANREcLs", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSystemControlGet);
    LIB_FUNCTION("9CHWVv6r3Dg", "libSceAudioOut", 1, "libSceAudioOut", sceAudioOutSystemControlSet);
    LIB_FUNCTION("Mt7JB3lOyJk", "libSceAudioOutSparkControl", 1, "libSceAudioOut",
                 sceAudioOutSparkControlSetEqCoef);
    LIB_FUNCTION("7UsdDOEvjlk", "libSceDbgAudioOut", 1, "libSceAudioOut",
                 sceAudioOutSetSystemDebugState);
};

} // namespace Libraries::AudioOut
