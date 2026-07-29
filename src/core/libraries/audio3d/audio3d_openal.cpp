// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <alext.h>
#include <efx.h>
#include <magic_enum/magic_enum.hpp>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_error.h"
#include "core/libraries/audio/openal_manager.h"
#include "core/libraries/audio3d/audio3d_error.h"
#include "core/libraries/audio3d/audio3d_openal.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"

// Fallbacks in case the alext.h in use predates these extensions.
#ifndef ALC_OUTPUT_MODE_SOFT
#define ALC_OUTPUT_MODE_SOFT 0x19AC
#define ALC_SURROUND_5_1_SOFT 0x1504
#define ALC_SURROUND_6_1_SOFT 0x1505
#define ALC_SURROUND_7_1_SOFT 0x1506
#endif

namespace Libraries::Audio3dOpenAL {

static constexpr u32 AUDIO3D_SAMPLE_RATE = 48000;

static constexpr AudioOut::OrbisAudioOutParamFormat AUDIO3D_OUTPUT_FORMAT =
    AudioOut::OrbisAudioOutParamFormat::S16Stereo;
static constexpr u32 AUDIO3D_OUTPUT_NUM_CHANNELS = 2;

static constexpr float DOWNMIX_FRONT = 1.0f;
static constexpr float DOWNMIX_CENTER = 0.7071f;
static constexpr float DOWNMIX_SURROUND = 0.7071f;
static constexpr float DOWNMIX_LFE = 0.0f;

static std::unique_ptr<Audio3dState> state;

struct AudioOutBufferInfo {
    u32 channels;
    u32 sample_size;
};

static AudioOutBufferInfo GetAudioOutBufferInfo(const AudioOut::OrbisAudioOutParamFormat format) {
    using Format = AudioOut::OrbisAudioOutParamFormat;
    switch (format) {
    case Format::S16Mono:
        return {1, sizeof(s16)};
    case Format::S16Stereo:
        return {2, sizeof(s16)};
    case Format::S16_8CH:
    case Format::S16_8CH_Std:
        return {8, sizeof(s16)};
    case Format::FloatMono:
        return {1, sizeof(float)};
    case Format::FloatStereo:
        return {2, sizeof(float)};
    case Format::Float_8CH:
    case Format::Float_8CH_Std:
        return {8, sizeof(float)};
    default:
        return {2, sizeof(s16)};
    }
}

static s32 DrainAssociatedPorts(Port& port) {
    while (true) {
        s32 handle = -1;
        std::vector<u8> buffer;
        {
            std::scoped_lock lock{port.mutex};
            const auto it =
                std::find_if(port.audioout_ports.begin(), port.audioout_ports.end(),
                             [](const AssociatedAudioOutPort& p) { return !p.pending.empty(); });
            if (it == port.audioout_ports.end()) {
                return ORBIS_OK;
            }
            handle = it->handle;
            buffer = std::move(it->pending.front());
            it->pending.pop_front();
        }

        const s32 ret = AudioOut::sceAudioOutOutput(handle, buffer.data());
        if (ret < 0) {
            return ret;
        }
    }
}

static constexpr u32 SPATIAL_RING_SLACK = 2;
static constexpr u64 SPATIAL_VOLUME_CHECK_INTERVAL_US = 50000;
// Give up on a frame if the device hasn't consumed anything for this long.
static constexpr u64 SPATIAL_SUBMIT_TIMEOUT_US = 1000000;
// Minimum sleep while waiting for a free ring slot.
static constexpr u64 SPATIAL_MIN_SLEEP_US = 500;

static const char* ALErrorString(const ALenum error) {
    switch (error) {
    case AL_NO_ERROR:
        return "AL_NO_ERROR";
    case AL_INVALID_NAME:
        return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
        return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
        return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
        return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
        return "AL_OUT_OF_MEMORY";
    default:
        return "Unknown AL error";
    }
}

static void SpatialReclaimSource(SpatialSource& src) {
    ALint processed = 0;
    alGetSourcei(src.source, AL_BUFFERS_PROCESSED, &processed);

    while (processed-- > 0) {
        ALuint buffer_id = 0;
        alSourceUnqueueBuffers(src.source, 1, &buffer_id);
        if (alGetError() != AL_NO_ERROR) {
            break;
        }
        src.available.push_back(buffer_id);
    }
}

static bool CreateSpatialSource(const Port& port, SpatialSource& src) {
    alGetError();

    ALuint source = 0;
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        LOG_ERROR(Lib_Audio3d, "Failed to generate spatial source");
        return false;
    }

    const u32 ring_size = port.parameters.queue_depth + SPATIAL_RING_SLACK;
    src.buffers.resize(ring_size);
    alGenBuffers(static_cast<ALsizei>(ring_size), reinterpret_cast<ALuint*>(src.buffers.data()));
    if (alGetError() != AL_NO_ERROR) {
        LOG_ERROR(Lib_Audio3d, "Failed to generate spatial buffers");
        alDeleteSources(1, &source);
        src.buffers.clear();
        return false;
    }
    src.available = src.buffers;

    alSourcef(source, AL_PITCH, 1.0f);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_MAX_GAIN, 16.0f);

    src.source = source;
    return true;
}

static void DestroySpatialSourceLocked(SpatialSource& src) {
    if (src.source != 0) {
        alSourceStop(src.source);

        ALint queued = 0;
        alGetSourcei(src.source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0) {
            ALuint buffer_id = 0;
            alSourceUnqueueBuffers(src.source, 1, &buffer_id);
        }

        alDeleteSources(1, &src.source);
    }
    if (!src.buffers.empty()) {
        alDeleteBuffers(static_cast<ALsizei>(src.buffers.size()),
                        reinterpret_cast<ALuint*>(src.buffers.data()));
    }
    src = SpatialSource{};
}

static void FreeBundle(SpatialFrameBundle& bundle) {
    std::free(bundle.bed.sample_buffer);
    bundle.bed.sample_buffer = nullptr;
    for (auto& frame : bundle.objects) {
        std::free(frame.pcm.sample_buffer);
        frame.pcm.sample_buffer = nullptr;
    }
    bundle.objects.clear();
}

static LPALGENEFFECTS p_alGenEffects;
static LPALDELETEEFFECTS p_alDeleteEffects;
static LPALEFFECTI p_alEffecti;
static LPALEFFECTF p_alEffectf;
static LPALGENAUXILIARYEFFECTSLOTS p_alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS p_alDeleteAuxiliaryEffectSlots;
static LPALAUXILIARYEFFECTSLOTI p_alAuxiliaryEffectSloti;
static LPALAUXILIARYEFFECTSLOTF p_alAuxiliaryEffectSlotf;

static bool LoadEfxFunctions() {
    static bool attempted = false;
    static bool ok = false;
    if (attempted) {
        return ok;
    }
    attempted = true;

    p_alGenEffects = reinterpret_cast<LPALGENEFFECTS>(alGetProcAddress("alGenEffects"));
    p_alDeleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(alGetProcAddress("alDeleteEffects"));
    p_alEffecti = reinterpret_cast<LPALEFFECTI>(alGetProcAddress("alEffecti"));
    p_alEffectf = reinterpret_cast<LPALEFFECTF>(alGetProcAddress("alEffectf"));
    p_alGenAuxiliaryEffectSlots = reinterpret_cast<LPALGENAUXILIARYEFFECTSLOTS>(
        alGetProcAddress("alGenAuxiliaryEffectSlots"));
    p_alDeleteAuxiliaryEffectSlots = reinterpret_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(
        alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
    p_alAuxiliaryEffectSloti =
        reinterpret_cast<LPALAUXILIARYEFFECTSLOTI>(alGetProcAddress("alAuxiliaryEffectSloti"));
    p_alAuxiliaryEffectSlotf =
        reinterpret_cast<LPALAUXILIARYEFFECTSLOTF>(alGetProcAddress("alAuxiliaryEffectSlotf"));

    ok = p_alGenEffects && p_alDeleteEffects && p_alEffecti && p_alEffectf &&
         p_alGenAuxiliaryEffectSlots && p_alDeleteAuxiliaryEffectSlots &&
         p_alAuxiliaryEffectSloti && p_alAuxiliaryEffectSlotf;
    return ok;
}

static void CreateSpatialReverbLocked(Port& port) {
    ALCcontext* context = alcGetCurrentContext();
    ALCdevice* alc_device = context ? alcGetContextsDevice(context) : nullptr;
    if (!alc_device || !alcIsExtensionPresent(alc_device, "ALC_EXT_EFX") || !LoadEfxFunctions()) {
        LOG_INFO(Lib_Audio3d, "EFX unavailable; LATE_REVERB_LEVEL will have no effect");
        return;
    }

    alGetError();

    ALuint effect = 0;
    ALuint slot = 0;
    p_alGenEffects(1, &effect);
    p_alGenAuxiliaryEffectSlots(1, &slot);
    if (alGetError() != AL_NO_ERROR) {
        if (effect != 0) {
            p_alDeleteEffects(1, &effect);
        }
        if (slot != 0) {
            p_alDeleteAuxiliaryEffectSlots(1, &slot);
        }
        LOG_WARNING(Lib_Audio3d, "Failed to create EFX reverb objects");
        return;
    }

    p_alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
    p_alEffectf(effect, AL_REVERB_DECAY_TIME, 1.8f);
    p_alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(effect));
    p_alAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN,
                             std::clamp(port.late_reverb_level, 0.0f, 1.0f));

    if (alGetError() != AL_NO_ERROR) {
        p_alDeleteAuxiliaryEffectSlots(1, &slot);
        p_alDeleteEffects(1, &effect);
        LOG_WARNING(Lib_Audio3d, "Failed to configure EFX reverb");
        return;
    }

    port.reverb_effect = effect;
    port.reverb_slot = slot;
    port.reverb_supported = true;
    LOG_INFO(Lib_Audio3d, "EFX late reverb ready (level {})", port.late_reverb_level);
}

static void SpatialApplyReverbLevelLocked(Port& port) {
    if (!port.reverb_supported || !port.spatial_ready ||
        !AudioOut::OpenALDevice::GetInstance().MakeCurrent(port.device_name)) {
        return;
    }
    p_alAuxiliaryEffectSlotf(port.reverb_slot, AL_EFFECTSLOT_GAIN,
                             std::clamp(port.late_reverb_level, 0.0f, 1.0f));
}

static void ResetObjectSpatialLocked(Port& port, ObjectState& obj) {
    if (obj.al.source == 0 || !port.spatial_ready ||
        !AudioOut::OpenALDevice::GetInstance().MakeCurrent(port.device_name)) {
        return;
    }
    alSourceStop(obj.al.source);
    SpatialReclaimSource(obj.al);
    alSourcei(obj.al.source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(obj.al.source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcef(obj.al.source, AL_GAIN, 0.0f);
    if (port.source_radius_supported) {
        alSourcef(obj.al.source, AL_SOURCE_RADIUS, 0.0f);
    }
    if (port.direct_channels_supported) {
        alSourcei(obj.al.source, AL_DIRECT_CHANNELS_SOFT, AL_FALSE);
    }
}

static void SpatialUpdateVolumeLocked(Port& port) {
    const u64 now = Kernel::sceKernelGetProcessTime();
    if (now - port.last_volume_check_us < SPATIAL_VOLUME_CHECK_INTERVAL_US) {
        return;
    }
    port.last_volume_check_us = now;

    const float slider_gain = EmulatorSettings.GetVolumeSlider() * 0.01f;
    if (std::abs(slider_gain - port.current_gain) < 0.001f) {
        return;
    }

    alSourcef(port.bed.source, AL_GAIN, slider_gain);
    if (const ALenum err = alGetError(); err == AL_NO_ERROR) {
        port.current_gain = slider_gain;
    } else {
        LOG_ERROR(Lib_Audio3d, "Failed to set spatial gain: {}", ALErrorString(err));
    }
}

static void DestroySpatial(Port& port) {
    if (!port.spatial_init_attempted) {
        return;
    }
    port.spatial_init_attempted = false;

    if (!port.spatial_ready) {
        return;
    }
    port.spatial_ready = false;

    auto& device = AudioOut::OpenALDevice::GetInstance();
    if (device.MakeCurrent(port.device_name)) {
        DestroySpatialSourceLocked(port.bed);
        for (auto& [object_id, obj] : port.objects) {
            DestroySpatialSourceLocked(obj.al);
        }
        if (port.reverb_supported) {
            ALuint slot = port.reverb_slot;
            ALuint effect = port.reverb_effect;
            p_alDeleteAuxiliaryEffectSlots(1, &slot);
            p_alDeleteEffects(1, &effect);
        }
    } else {
        port.bed = SpatialSource{};
        for (auto& [object_id, obj] : port.objects) {
            obj.al = SpatialSource{};
        }
    }

    port.reverb_supported = false;
    port.reverb_slot = 0;
    port.reverb_effect = 0;

    for (auto& bundle : port.spatial_queue) {
        FreeBundle(bundle);
    }
    port.spatial_queue.clear();

    device.UnregisterPort(port.device_name);
}

static bool EnsureSpatial(Port& port) {
    if (port.spatial_init_attempted) {
        return port.spatial_ready;
    }
    port.spatial_init_attempted = true;

    port.device_name = EmulatorSettings.GetOpenALMainOutputDevice();
    auto& device = AudioOut::OpenALDevice::GetInstance();

    if (!device.RegisterPort(port.device_name)) {
        LOG_WARNING(Lib_Audio3d,
                    "OpenAL device '{}' unavailable for spatial output, using AudioOut path",
                    port.device_name);
        return false;
    }

    if (!device.MakeCurrent(port.device_name)) {
        LOG_ERROR(Lib_Audio3d, "Failed to make OpenAL context current for spatial output");
        device.UnregisterPort(port.device_name);
        return false;
    }

    alGetError();
    alDistanceModel(AL_NONE);
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);

    port.source_radius_supported = alIsExtensionPresent("AL_EXT_SOURCE_RADIUS") == AL_TRUE;
    port.direct_channels_supported = alIsExtensionPresent("AL_SOFT_direct_channels") == AL_TRUE;

    port.bed_channels = 2;
    if (ALCcontext* ctx = alcGetCurrentContext()) {
        if (ALCdevice* alc_dev = alcGetContextsDevice(ctx);
            alc_dev && alcIsExtensionPresent(alc_dev, "ALC_SOFT_output_mode")) {
            ALCint output_mode = 0;
            alcGetIntegerv(alc_dev, ALC_OUTPUT_MODE_SOFT, 1, &output_mode);
            // Only claim native surround when the AL 7.1/5.1 buffer format and
            // the multichannel extension are actually usable.
            const bool have_mc = alIsExtensionPresent("AL_EXT_MCFORMATS") == AL_TRUE;
            if (have_mc && output_mode == ALC_SURROUND_7_1_SOFT &&
                alGetEnumValue("AL_FORMAT_71CHN16") != 0) {
                port.bed_channels = 8;
            } else if (have_mc &&
                       (output_mode == ALC_SURROUND_5_1_SOFT ||
                        output_mode == ALC_SURROUND_6_1_SOFT) &&
                       alGetEnumValue("AL_FORMAT_51CHN16") != 0) {
                port.bed_channels = 6;
            }
        }
    }
    if (port.bed_channels != 2) {
        LOG_INFO(Lib_Audio3d, "Surround endpoint: bed kept native at {} channels",
                 port.bed_channels);
    }

    if (!CreateSpatialSource(port, port.bed)) {
        device.UnregisterPort(port.device_name);
        return false;
    }

    const float slider_gain = EmulatorSettings.GetVolumeSlider() * 0.01f;
    alSourcef(port.bed.source, AL_GAIN, slider_gain);
    port.current_gain = slider_gain;
    if (port.direct_channels_supported) {
        alSourcei(port.bed.source, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);
    }

    CreateSpatialReverbLocked(port);

    port.period_us =
        (1000000ULL * port.parameters.granularity + AUDIO3D_SAMPLE_RATE / 2) / AUDIO3D_SAMPLE_RATE;
    port.spatial_ready = true;

    LOG_INFO(Lib_Audio3d,
             "Spatial output initialized on OpenAL device '{}' (granularity {}, ring {})",
             port.device_name.empty() ? "Default Device" : port.device_name,
             port.parameters.granularity, port.parameters.queue_depth + SPATIAL_RING_SLACK);
    return true;
}

static float GetObjectGain(const ObjectState& obj) {
    float gain = 0.0f;
    const auto gain_key = static_cast<u32>(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_GAIN);
    if (obj.persistent_attributes.contains(gain_key)) {
        const auto& blob = obj.persistent_attributes.at(gain_key);
        if (blob.size() >= sizeof(float)) {
            std::memcpy(&gain, blob.data(), sizeof(float));
        }
    }
    return gain;
}

static bool GetObjectPosition(const ObjectState& obj, OrbisAudio3dPosition& out) {
    const auto key = static_cast<u32>(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_POSITION);
    if (!obj.persistent_attributes.contains(key)) {
        return false;
    }
    const auto& blob = obj.persistent_attributes.at(key);
    if (blob.size() < sizeof(OrbisAudio3dPosition)) {
        return false;
    }
    std::memcpy(&out, blob.data(), sizeof(OrbisAudio3dPosition));
    return true;
}

static float GetObjectSpread(const ObjectState& obj) {
    float spread = 0.0f;
    const auto key = static_cast<u32>(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_SPREAD);
    if (obj.persistent_attributes.contains(key)) {
        const auto& blob = obj.persistent_attributes.at(key);
        if (blob.size() >= sizeof(float)) {
            std::memcpy(&spread, blob.data(), sizeof(float));
        }
    }
    return spread;
}

static OrbisAudio3dPassthrough GetObjectPassthrough(const ObjectState& obj) {
    u32 value = 0;
    const auto key = static_cast<u32>(OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_PASSTHROUGH);
    if (obj.persistent_attributes.contains(key)) {
        const auto& blob = obj.persistent_attributes.at(key);
        if (blob.size() >= sizeof(u32)) {
            std::memcpy(&value, blob.data(), sizeof(u32));
        }
    }
    if (value > static_cast<u32>(OrbisAudio3dPassthrough::ORBIS_AUDIO3D_PASSTHROUGH_RIGHT)) {
        value = 0;
    }
    return static_cast<OrbisAudio3dPassthrough>(value);
}

static float SpreadToRadius(const OrbisAudio3dPosition& pos, float spread,
                            const float near_field_radius) {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float MAX_RADIUS = 100.0f;

    const float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    if (distance <= near_field_radius) {
        return MAX_RADIUS;
    }

    spread = std::clamp(spread, 0.0f, 2.0f * PI);
    if (spread >= PI * 0.98f) {
        return MAX_RADIUS;
    }

    return std::min(distance * std::tan(spread * 0.5f), MAX_RADIUS);
}

static const s16* ConvertObjectFrameToMonoS16(Port& port, const AudioData& data,
                                              const u32 granularity) {
    auto& out = port.spatial_scratch;
    out.assign(granularity, 0);

    const u32 frames = std::min(granularity, data.num_samples);
    const u32 channels = std::max<u32>(data.num_channels, 1);

    if (data.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
        const s16* src = reinterpret_cast<const s16*>(data.sample_buffer);
        if (channels == 1) {
            std::memcpy(out.data(), src, frames * sizeof(s16));
        } else {
            for (u32 i = 0; i < frames; i++) {
                s32 sum = 0;
                for (u32 c = 0; c < channels; c++) {
                    sum += src[i * channels + c];
                }
                out[i] = static_cast<s16>(sum / static_cast<s32>(channels));
            }
        }
    } else { // FLOAT input
        const float* src = reinterpret_cast<const float*>(data.sample_buffer);
        for (u32 i = 0; i < frames; i++) {
            float sum = 0.0f;
            for (u32 c = 0; c < channels; c++) {
                sum += src[i * channels + c];
            }
            const float v = std::clamp(sum / static_cast<float>(channels), -1.0f, 1.0f);
            out[i] = static_cast<s16>(v * 32767.0f);
        }
    }

    return out.data();
}

static const s16* ConvertObjectFrameToPassthroughS16(Port& port, const AudioData& data,
                                                     const u32 granularity,
                                                     const OrbisAudio3dPassthrough passthrough) {
    const s16* mono = ConvertObjectFrameToMonoS16(port, data, granularity);

    auto& out = port.spatial_scratch_stereo;
    out.assign(granularity * 2, 0);

    const u32 ear =
        passthrough == OrbisAudio3dPassthrough::ORBIS_AUDIO3D_PASSTHROUGH_RIGHT ? 1u : 0u;
    for (u32 i = 0; i < granularity; i++) {
        out[i * 2 + ear] = mono[i];
    }

    return out.data();
}

static ALenum BedALFormat(const u32 channels) {
    switch (channels) {
    case 8:
        return static_cast<ALenum>(alGetEnumValue("AL_FORMAT_71CHN16"));
    case 6:
        return static_cast<ALenum>(alGetEnumValue("AL_FORMAT_51CHN16"));
    default:
        return AL_FORMAT_STEREO16;
    }
}

static bool SpatialUploadLocked(SpatialSource& src, const void* samples, const u32 bytes,
                                const ALenum format) {
    SpatialReclaimSource(src);
    if (src.available.empty()) {
        return false;
    }

    const ALuint buffer_id = src.available.back();
    src.available.pop_back();

    alGetError();
    alBufferData(buffer_id, format, samples, static_cast<ALsizei>(bytes), AUDIO3D_SAMPLE_RATE);
    ALuint queue_id = buffer_id;
    alSourceQueueBuffers(src.source, 1, &queue_id);

    if (const ALenum err = alGetError(); err != AL_NO_ERROR) {
        LOG_ERROR(Lib_Audio3d, "Failed to queue spatial buffer: {}", ALErrorString(err));
        src.available.push_back(buffer_id);
        return true;
    }

    ALint al_state = 0;
    alGetSourcei(src.source, AL_SOURCE_STATE, &al_state);
    if (al_state != AL_PLAYING) {
        alSourcePlay(src.source);
    }

    return true;
}

static s32 SpatialSubmitBundle(Port& port, SpatialFrameBundle& bundle,
                               const bool nonblocking = false) {
    const u32 bed_channels = bundle.bed.num_channels;
    const u32 bed_bytes = bundle.bed.num_samples * bed_channels * sizeof(s16);
    const ALenum bed_format = BedALFormat(bed_channels);
    const u32 granularity = port.parameters.granularity;
    u64 waited_us = 0;

    while (true) {
        {
            std::scoped_lock lock{port.mutex};

            if (!port.spatial_ready ||
                !AudioOut::OpenALDevice::GetInstance().MakeCurrent(port.device_name)) {
                LOG_ERROR(Lib_Audio3d, "Spatial output lost, dropping frame");
                FreeBundle(bundle);
                return ORBIS_OK;
            }

            SpatialUpdateVolumeLocked(port);

            if (SpatialUploadLocked(port.bed, bundle.bed.sample_buffer, bed_bytes, bed_format)) {
                for (auto& frame : bundle.objects) {
                    const auto it = port.objects.find(frame.object_id);
                    if (it == port.objects.end()) {
                        continue;
                    }

                    auto& src = it->second.al;
                    if (src.source == 0 && !CreateSpatialSource(port, src)) {
                        continue;
                    }

                    alSourcef(src.source, AL_GAIN, frame.gain * port.current_gain);

                    const bool passthrough =
                        frame.passthrough !=
                        OrbisAudio3dPassthrough::ORBIS_AUDIO3D_PASSTHROUGH_NONE;

                    if (port.direct_channels_supported) {
                        alSourcei(src.source, AL_DIRECT_CHANNELS_SOFT,
                                  passthrough ? AL_TRUE : AL_FALSE);
                    }

                    if (port.reverb_supported) {
                        alSource3i(src.source, AL_AUXILIARY_SEND_FILTER,
                                   passthrough ? AL_EFFECTSLOT_NULL
                                               : static_cast<ALint>(port.reverb_slot),
                                   0, AL_FILTER_NULL);
                    }

                    if (passthrough) {
                        alSourcei(src.source, AL_SOURCE_RELATIVE, AL_TRUE);
                        alSource3f(src.source, AL_POSITION, 0.0f, 0.0f, 0.0f);

                        const s16* stereo = ConvertObjectFrameToPassthroughS16(
                            port, frame.pcm, granularity, frame.passthrough);
                        if (!SpatialUploadLocked(src, stereo, granularity * 2 * sizeof(s16),
                                                 AL_FORMAT_STEREO16)) {
                            LOG_WARNING(Lib_Audio3d, "Object {} ring full, dropping frame",
                                        frame.object_id);
                        }
                        continue;
                    }

                    if (frame.has_position) {
                        alSourcei(src.source, AL_SOURCE_RELATIVE, AL_FALSE);
                        alSource3f(src.source, AL_POSITION, frame.position.x, frame.position.y,
                                   -frame.position.z);
                        if (port.source_radius_supported) {
                            alSourcef(src.source, AL_SOURCE_RADIUS,
                                      SpreadToRadius(frame.position, frame.spread,
                                                     port.downmix_spread_radius));
                        }
                    }

                    const s16* mono = ConvertObjectFrameToMonoS16(port, frame.pcm, granularity);
                    if (!SpatialUploadLocked(src, mono, granularity * sizeof(s16),
                                             AL_FORMAT_MONO16)) {
                        LOG_WARNING(Lib_Audio3d, "Object {} ring full, dropping frame",
                                    frame.object_id);
                    }
                }

                FreeBundle(bundle);
                return ORBIS_OK;
            }
        }

        if (nonblocking) {
            return ORBIS_AUDIO3D_ERROR_NOT_READY;
        }
        if (waited_us >= SPATIAL_SUBMIT_TIMEOUT_US) {
            LOG_WARNING(Lib_Audio3d, "Spatial submit timed out, dropping frame");
            FreeBundle(bundle);
            return ORBIS_OK;
        }

        const u64 sleep_us = std::max<u64>(port.period_us / 2, SPATIAL_MIN_SLEEP_US);
        std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
        waited_us += sleep_us;
    }
}

static s32 SpatialSubmitFrame(Port& port, const AudioData& frame) {
    SpatialFrameBundle bundle{};
    bundle.bed = frame;
    return SpatialSubmitBundle(port, bundle);
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutClose(const s32 handle) {
    LOG_INFO(Lib_Audio3d, "called, handle = {}", handle);

    bool found = false;
    if (state) {
        for (auto& [port_id, port] : state->ports) {
            std::scoped_lock lock{port.mutex};
            found |= std::erase_if(port.audioout_ports, [&](const AssociatedAudioOutPort& p) {
                         return p.handle == handle;
                     }) != 0;
        }
    }
    if (!found) {
        LOG_ERROR(Lib_Audio3d, "handle {} was not opened via sceAudio3dAudioOutOpen", handle);
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

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

    std::scoped_lock lock{state->ports[port_id].mutex};
    if (len != state->ports[port_id].parameters.granularity) {
        LOG_ERROR(Lib_Audio3d, "len != state->ports[port_id].parameters.granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    const s32 handle = sceAudioOutOpen(user_id, static_cast<AudioOut::OrbisAudioOutPort>(type),
                                       index, len, freq, param);
    if (handle < 0) {
        return handle;
    }

    const auto info = GetAudioOutBufferInfo(param.data_format.Value());
    AssociatedAudioOutPort aout{};
    aout.handle = handle;
    aout.buffer_bytes = len * info.channels * info.sample_size;
    aout.samples_per_buffer = len * info.channels;
    aout.is_float = info.sample_size == sizeof(float);
    state->ports[port_id].audioout_ports.push_back(std::move(aout));
    return handle;
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

    if (state) {
        for (auto& [port_id, port] : state->ports) {
            std::scoped_lock lock{port.mutex};
            for (auto& aout : port.audioout_ports) {
                if (aout.handle != handle) {
                    continue;
                }

                const uintptr_t align_mask = aout.is_float ? 3u : 1u;
                if ((reinterpret_cast<uintptr_t>(ptr) & align_mask) != 0) {
                    LOG_ERROR(Lib_Audio3d, "sample buffer for handle {} is misaligned", handle);
                    return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
                }

                if (aout.pending.size() >= port.parameters.queue_depth) {
                    return ORBIS_AUDIO3D_ERROR_NOT_READY;
                }

                const u8* src = static_cast<const u8*>(ptr);
                aout.pending.emplace_back(src, src + aout.buffer_bytes);

                return static_cast<s32>(aout.samples_per_buffer);
            }
        }
    }

    LOG_ERROR(Lib_Audio3d, "handle {} was not opened via sceAudio3dAudioOutOpen", handle);
    return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOutputs(AudioOut::OrbisAudioOutOutputParam* param,
                                           const u32 num) {
    LOG_DEBUG(Lib_Audio3d, "called, param = {}, num = {}", static_cast<void*>(param), num);

    if (!param || !num) {
        LOG_ERROR(Lib_Audio3d, "!param || !num");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    for (u32 i = 0; i < num; i++) {
        const s32 ret = sceAudio3dAudioOutOutput(param[i].handle, param[i].ptr);
        if (ret < 0) {
            return ret;
        }
    }

    return ORBIS_OK;
}

static s32 ConvertAndEnqueue(std::deque<AudioData>& queue, const OrbisAudio3dPcm& pcm,
                             const u32 num_channels, const u32 granularity, const u32 max_entries) {
    if (!pcm.sample_buffer || !pcm.num_samples) {
        LOG_ERROR(Lib_Audio3d, "!pcm.sample_buffer || !pcm.num_samples");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    const u32 bytes_per_sample =
        (pcm.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) ? sizeof(s16) : sizeof(float);

    if ((reinterpret_cast<uintptr_t>(pcm.sample_buffer) & (bytes_per_sample - 1)) != 0) {
        LOG_ERROR(Lib_Audio3d, "pcm.sample_buffer is misaligned");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (queue.size() >= max_entries) {
        LOG_ERROR(Lib_Audio3d, "queue.size() >= max_entries");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    const u32 dst_bytes = granularity * num_channels * bytes_per_sample;
    u8* copy = static_cast<u8*>(std::calloc(1, dst_bytes));
    if (!copy) {
        LOG_ERROR(Lib_Audio3d, "out of memory");
        return ORBIS_AUDIO3D_ERROR_OUT_OF_MEMORY;
    }

    const u32 samples_to_copy = std::min(pcm.num_samples, granularity);
    std::memcpy(copy, pcm.sample_buffer, samples_to_copy * num_channels * bytes_per_sample);

    queue.emplace_back(AudioData{
        .sample_buffer = copy,
        .num_samples = granularity,
        .num_channels = num_channels,
        .format = pcm.format,
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

    if (num_channels != 2 && num_channels != 6 && num_channels != 8) {
        LOG_ERROR(Lib_Audio3d, "num_channels must be 2, 6, or 8");
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

    std::scoped_lock lock{state->ports[port_id].mutex};
    return ConvertAndEnqueue(state->ports[port_id].bed_queue,
                             OrbisAudio3dPcm{
                                 .format = format,
                                 .sample_buffer = buffer,
                                 .num_samples = num_samples,
                             },
                             num_channels, state->ports[port_id].parameters.granularity,
                             state->ports[port_id].parameters.queue_depth);
}

static constexpr u32 AUDIO3D_MAX_SPEAKERS = 16;

s32 PS4_SYSV_ABI sceAudio3dCreateSpeakerArray(
    OrbisAudio3dSpeakerArrayHandle* handle, const OrbisAudio3dSpeakerArrayParameters* parameters) {
    LOG_INFO(Lib_Audio3d, "called");

    if (!handle || !parameters || !parameters->speaker_positions || parameters->num_speakers == 0 ||
        parameters->num_speakers > AUDIO3D_MAX_SPEAKERS) {
        LOG_ERROR(Lib_Audio3d, "invalid speaker array parameters");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    SpeakerArray array{};
    array.is_3d = parameters->is_3d;
    array.speakers.assign(parameters->speaker_positions,
                          parameters->speaker_positions + parameters->num_speakers);

    std::scoped_lock lock{state->speaker_arrays_mutex};
    const u64 id = state->next_speaker_array_id++;
    state->speaker_arrays.emplace(id, std::move(array));
    *handle = reinterpret_cast<OrbisAudio3dSpeakerArrayHandle>(id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray(OrbisAudio3dSpeakerArrayHandle handle) {
    LOG_INFO(Lib_Audio3d, "called");

    const u64 id = reinterpret_cast<u64>(handle);
    std::scoped_lock lock{state->speaker_arrays_mutex};
    if (state->speaker_arrays.erase(id) == 0) {
        LOG_ERROR(Lib_Audio3d, "invalid speaker array handle");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
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

u64 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize(const u32 num_speakers, const bool is_3d) {
    LOG_DEBUG(Lib_Audio3d, "called, num_speakers = {}, is_3d = {}", num_speakers, is_3d);
    const u64 per_speaker = is_3d ? 64 : 32;
    return sizeof(void*) * 4 + static_cast<u64>(num_speakers) * per_speaker;
}

static void ComputeVbapCoefficients(const SpeakerArray& array, const OrbisAudio3dPosition& pos,
                                    float spread, float* coefficients, const u32 count) {
    for (u32 i = 0; i < count; i++) {
        coefficients[i] = 0.0f;
    }
    const float src_len = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    if (src_len < 1e-6f) {
        const float g = 1.0f / std::sqrt(static_cast<float>(count));
        for (u32 i = 0; i < count; i++) {
            coefficients[i] = g;
        }
        return;
    }
    const float sx = pos.x / src_len, sy = pos.y / src_len, sz = pos.z / src_len;

    constexpr float PI = 3.14159265358979323846f;
    spread = std::clamp(spread, 0.0f, 2.0f * PI);
    const float sharpness = std::max(0.05f, 1.0f - spread / PI);

    float power = 0.0f;
    for (u32 i = 0; i < count && i < array.speakers.size(); i++) {
        const auto& sp = array.speakers[i];
        const float len = std::sqrt(sp.x * sp.x + sp.y * sp.y + sp.z * sp.z);
        if (len < 1e-6f) {
            continue;
        }
        float dot = (sx * sp.x + sy * sp.y + sz * sp.z) / len;
        dot = std::clamp(dot, -1.0f, 1.0f);
        const float floor = spread / (2.0f * PI); // 0 at tight, 0.5 at pi, 1 at 2*pi
        const float shaped = 0.5f * (dot + 1.0f); // 0 (opposite) .. 1 (aligned)
        float weight = std::max(floor, shaped);
        // Concentrate on the nearest speakers when spread is tight.
        weight = std::pow(weight, 1.0f + sharpness * 8.0f);
        coefficients[i] = weight;
        power += weight * weight;
    }

    if (power > 1e-12f) {
        const float norm = 1.0f / std::sqrt(power);
        for (u32 i = 0; i < count; i++) {
            coefficients[i] *= norm;
        }
    } else {
        // Source points away from every speaker; fall back to uniform.
        const float g = 1.0f / std::sqrt(static_cast<float>(count));
        for (u32 i = 0; i < count; i++) {
            coefficients[i] = g;
        }
    }
}

s32 PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients(
    const OrbisAudio3dSpeakerArrayHandle handle, const OrbisAudio3dPosition pos, const float spread,
    float* coefficients, const u32 num_coefficients) {
    LOG_DEBUG(Lib_Audio3d, "called, num_coefficients = {}", num_coefficients);

    if (!coefficients || num_coefficients == 0) {
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    std::scoped_lock lock{state->speaker_arrays_mutex};
    const auto it = state->speaker_arrays.find(reinterpret_cast<u64>(handle));
    if (it == state->speaker_arrays.end()) {
        LOG_ERROR(Lib_Audio3d, "invalid speaker array handle");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    // The docs require num_coefficients == the array's speaker count.
    if (num_coefficients != it->second.speakers.size()) {
        LOG_ERROR(Lib_Audio3d, "num_coefficients {} != speaker count {}", num_coefficients,
                  it->second.speakers.size());
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    ComputeVbapCoefficients(it->second, pos, spread, coefficients, num_coefficients);
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

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dObjectReserve(const OrbisAudio3dPortId port_id,
                                         OrbisAudio3dObjectId* object_id) {
    LOG_INFO(Lib_Audio3d, "called, port_id = {}, object_id = {}", port_id,
             static_cast<void*>(object_id));

    if (!object_id) {
        LOG_ERROR(Lib_Audio3d, "!object_id");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    *object_id = ORBIS_AUDIO3D_OBJECT_INVALID;

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};

    if (port.objects.size() >= port.parameters.max_objects) {
        LOG_ERROR(Lib_Audio3d, "port has no available objects (max_objects = {})",
                  port.parameters.max_objects);
        return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
    }

    do {
        ++port.next_object_id;
    } while (port.next_object_id == 0 ||
             port.next_object_id == static_cast<u32>(ORBIS_AUDIO3D_OBJECT_INVALID) ||
             port.objects.contains(port.next_object_id));

    *object_id = port.next_object_id;
    port.objects.emplace(*object_id, ObjectState{});
    LOG_INFO(Lib_Audio3d, "reserved object_id = {}", *object_id);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dObjectSetAttribute(const OrbisAudio3dPortId port_id,
                                              const OrbisAudio3dObjectId object_id,
                                              const OrbisAudio3dAttributeId attribute_id,
                                              const void* attribute, const u64 attribute_size) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}, object_id = {}, attribute_id = {:#x}, size = {}",
              port_id, object_id, static_cast<u32>(attribute_id), attribute_size);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};
    if (!port.objects.contains(object_id)) {
        LOG_DEBUG(Lib_Audio3d, "object_id {} not reserved (race with Unreserve?), no-op",
                  object_id);
        return ORBIS_OK;
    }

    if (!attribute_size &&
        attribute_id != OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE) {
        LOG_ERROR(Lib_Audio3d, "!attribute_size for non-reset attribute");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    auto& obj = port.objects[object_id];
    if (obj.unreserved) {
        LOG_ERROR(Lib_Audio3d, "object_id not reserved");
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
    }

    if (attribute_id == OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE) {
        for (auto& data : obj.pcm_queue) {
            std::free(data.sample_buffer);
        }
        obj.pcm_queue.clear();
        obj.persistent_attributes.clear();
        ResetObjectSpatialLocked(port, obj);
        LOG_DEBUG(Lib_Audio3d, "RESET_STATE for object {}", object_id);
        return ORBIS_OK;
    }

    const auto* src = static_cast<const u8*>(attribute);
    obj.persistent_attributes[static_cast<u32>(attribute_id)].assign(src, src + attribute_size);

    return ORBIS_OK;
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

    if (!num_attributes || !attribute_array) {
        LOG_ERROR(Lib_Audio3d, "!num_attributes || !attribute_array");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};
    if (!port.objects.contains(object_id)) {
        LOG_DEBUG(Lib_Audio3d, "object_id {} not reserved", object_id);
        return ORBIS_OK;
    }

    auto& obj = port.objects[object_id];
    if (obj.unreserved) {
        LOG_ERROR(Lib_Audio3d, "object_id not reserved");
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
    }

    for (u64 i = 0; i < num_attributes; i++) {
        if (attribute_array[i].attribute_id ==
            OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE) {
            for (auto& data : obj.pcm_queue) {
                std::free(data.sample_buffer);
            }
            obj.pcm_queue.clear();
            obj.persistent_attributes.clear();
            ResetObjectSpatialLocked(port, obj);
            LOG_DEBUG(Lib_Audio3d, "RESET_STATE for object {}", object_id);
            break; // Only one reset is needed even if listed multiple times.
        }
    }

    for (u64 i = 0; i < num_attributes; i++) {
        const auto& attr = attribute_array[i];

        switch (attr.attribute_id) {
        case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE:
            break; // Already applied in first pass above.
        case OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_PCM: {
            if (attr.value_size < sizeof(OrbisAudio3dPcm)) {
                LOG_ERROR(Lib_Audio3d, "PCM attribute value_size too small");
                continue;
            }
            const auto pcm = static_cast<OrbisAudio3dPcm*>(attr.value);
            // Object audio is always mono (1 channel).
            if (const auto ret =
                    ConvertAndEnqueue(obj.pcm_queue, *pcm, 1, port.parameters.granularity,
                                      port.parameters.queue_depth);
                ret != ORBIS_OK) {
                return ret;
            }
            break;
        }
        default: {
            if (attr.value && attr.value_size > 0) {
                const auto* src = static_cast<const u8*>(attr.value);
                obj.persistent_attributes[static_cast<u32>(attr.attribute_id)].assign(
                    src, src + attr.value_size);
            }
            LOG_DEBUG(Lib_Audio3d, "Stored attribute {:#x} for object {}",
                      static_cast<u32>(attr.attribute_id), object_id);
            break;
        }
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dObjectUnreserve(const OrbisAudio3dPortId port_id,
                                           const OrbisAudio3dObjectId object_id) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}, object_id = {}", port_id, object_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};

    if (!port.objects.contains(object_id)) {
        LOG_ERROR(Lib_Audio3d, "object_id not reserved");
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
    }

    auto& obj = port.objects[object_id];
    if (obj.unreserved) {
        LOG_ERROR(Lib_Audio3d, "object_id not reserved");
        return ORBIS_AUDIO3D_ERROR_INVALID_OBJECT;
    }

    if (!obj.pcm_queue.empty()) {
        obj.unreserved = true;
        return ORBIS_OK;
    }

    if (obj.al.source != 0 && port.spatial_ready &&
        AudioOut::OpenALDevice::GetInstance().MakeCurrent(port.device_name)) {
        DestroySpatialSourceLocked(obj.al);
    }

    port.objects.erase(object_id);
    return ORBIS_OK;
}

static void RetireDrainedObjectsLocked(Port& port) {
    std::vector<OrbisAudio3dObjectId> retired;
    for (const auto& [obj_id, obj] : port.objects) {
        if (!obj.unreserved || !obj.pcm_queue.empty()) {
            continue;
        }
        const bool referenced = std::any_of(
            port.spatial_queue.begin(), port.spatial_queue.end(), [&](const SpatialFrameBundle& b) {
                return std::any_of(
                    b.objects.begin(), b.objects.end(),
                    [&](const SpatialObjectFrame& f) { return f.object_id == obj_id; });
            });
        if (!referenced) {
            retired.push_back(obj_id);
        }
    }
    for (const auto obj_id : retired) {
        auto& obj = port.objects[obj_id];
        if (obj.al.source != 0 &&
            AudioOut::OpenALDevice::GetInstance().MakeCurrent(port.device_name)) {
            DestroySpatialSourceLocked(obj.al);
        }
        port.objects.erase(obj_id);
    }
}

static u32 ProcessMixQueue(Port& port) {
    const u32 granularity = port.parameters.granularity;
    const u32 out_samples = granularity * AUDIO3D_OUTPUT_NUM_CHANNELS;

    float* mix_float = static_cast<float*>(std::calloc(out_samples, sizeof(float)));
    if (!mix_float)
        return ORBIS_AUDIO3D_ERROR_OUT_OF_MEMORY;

    auto mix_in = [&](std::deque<AudioData>& queue, const float gain) {
        if (queue.empty())
            return;

        if (gain == 0.0f) {
            AudioData data = queue.front();
            queue.pop_front();
            std::free(data.sample_buffer);
            return;
        }

        AudioData data = queue.front();
        queue.pop_front();

        const u32 frames = std::min(granularity, data.num_samples);
        const u32 channels = data.num_channels;

        if (data.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
            const s16* src = reinterpret_cast<const s16*>(data.sample_buffer);

            for (u32 i = 0; i < frames; i++) {
                float left = 0.0f;
                float right = 0.0f;

                if (channels == 1) {
                    float v = src[i] / 32768.0f;
                    left = v;
                    right = v;
                } else {
                    const auto sample = [&](const u32 c) {
                        return src[i * channels + c] / 32768.0f;
                    };
                    left = DOWNMIX_FRONT * sample(0);
                    right = DOWNMIX_FRONT * sample(1);
                    if (channels >= 3) {
                        const float center = DOWNMIX_CENTER * sample(2);
                        left += center;
                        right += center;
                    }
                    if constexpr (DOWNMIX_LFE != 0.0f) {
                        if (channels >= 4) {
                            const float lfe = DOWNMIX_LFE * sample(3);
                            left += lfe;
                            right += lfe;
                        }
                    }
                    if (channels >= 6) {
                        left += DOWNMIX_SURROUND * sample(4);
                        right += DOWNMIX_SURROUND * sample(5);
                    }
                    if (channels >= 8) {
                        left += DOWNMIX_SURROUND * sample(6);
                        right += DOWNMIX_SURROUND * sample(7);
                    }
                }

                mix_float[i * 2 + 0] += left * gain;
                mix_float[i * 2 + 1] += right * gain;
            }
        } else { // FLOAT input
            const float* src = reinterpret_cast<const float*>(data.sample_buffer);

            for (u32 i = 0; i < frames; i++) {
                float left = 0.0f;
                float right = 0.0f;

                if (channels == 1) {
                    left = src[i];
                    right = src[i];
                } else {
                    // Same multichannel fold as the S16 branch above.
                    const auto sample = [&](const u32 c) { return src[i * channels + c]; };
                    left = DOWNMIX_FRONT * sample(0);
                    right = DOWNMIX_FRONT * sample(1);
                    if (channels >= 3) {
                        const float center = DOWNMIX_CENTER * sample(2);
                        left += center;
                        right += center;
                    }
                    if constexpr (DOWNMIX_LFE != 0.0f) {
                        if (channels >= 4) {
                            const float lfe = DOWNMIX_LFE * sample(3);
                            left += lfe;
                            right += lfe;
                        }
                    }
                    if (channels >= 6) {
                        left += DOWNMIX_SURROUND * sample(4);
                        right += DOWNMIX_SURROUND * sample(5);
                    }
                    if (channels >= 8) {
                        left += DOWNMIX_SURROUND * sample(6);
                        right += DOWNMIX_SURROUND * sample(7);
                    }
                }

                mix_float[i * 2 + 0] += left * gain;
                mix_float[i * 2 + 1] += right * gain;
            }
        }

        std::free(data.sample_buffer);
    };

    const bool spatial = EnsureSpatial(port);
    const bool native_bed = spatial && port.bed_channels != 2 && !port.bed_queue.empty();
    if (!native_bed) {
        mix_in(port.bed_queue, 1.0f);
    }

    SpatialFrameBundle bundle{};

    if (spatial) {
        for (auto& [obj_id, obj] : port.objects) {
            if (obj.pcm_queue.empty()) {
                continue;
            }
            SpatialObjectFrame frame{};
            frame.object_id = obj_id;
            frame.pcm = obj.pcm_queue.front();
            obj.pcm_queue.pop_front();
            frame.gain = GetObjectGain(obj);
            frame.has_position = GetObjectPosition(obj, frame.position);
            frame.spread = GetObjectSpread(obj);
            frame.passthrough = GetObjectPassthrough(obj);
            bundle.objects.push_back(std::move(frame));
        }
    } else {
        for (auto& [obj_id, obj] : port.objects) {
            mix_in(obj.pcm_queue, GetObjectGain(obj));
        }
    }

    s16* mix_s16 = static_cast<s16*>(std::malloc(out_samples * sizeof(s16)));
    if (!mix_s16) {
        std::free(mix_float);
        return ORBIS_AUDIO3D_ERROR_OUT_OF_MEMORY;
    }

    for (u32 i = 0; i < out_samples; i++) {
        float v = std::clamp(mix_float[i], -1.0f, 1.0f);
        mix_s16[i] = static_cast<s16>(v * 32767.0f);
    }

    std::free(mix_float);

    const AudioData out_frame{.sample_buffer = reinterpret_cast<u8*>(mix_s16),
                              .num_samples = granularity,
                              .num_channels = AUDIO3D_OUTPUT_NUM_CHANNELS,
                              .format = OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16};

    if (spatial) {
        if (native_bed) {
            const u32 bed_ch = port.bed_channels;
            const u32 n = granularity * bed_ch;
            s16* native = static_cast<s16*>(std::calloc(n, sizeof(s16)));
            if (native) {
                AudioData data = port.bed_queue.front();
                port.bed_queue.pop_front();
                const u32 frames = std::min(granularity, data.num_samples);
                const u32 src_ch = std::max<u32>(data.num_channels, 1);
                const u32 copy_ch = std::min(src_ch, bed_ch);

                if (data.format == OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16) {
                    const s16* src = reinterpret_cast<const s16*>(data.sample_buffer);
                    for (u32 i = 0; i < frames; i++) {
                        for (u32 c = 0; c < copy_ch; c++) {
                            native[i * bed_ch + c] = src[i * src_ch + c];
                        }
                    }
                } else { // FLOAT
                    const float* src = reinterpret_cast<const float*>(data.sample_buffer);
                    for (u32 i = 0; i < frames; i++) {
                        for (u32 c = 0; c < copy_ch; c++) {
                            const float v = std::clamp(src[i * src_ch + c], -1.0f, 1.0f);
                            native[i * bed_ch + c] = static_cast<s16>(v * 32767.0f);
                        }
                    }
                }
                std::free(data.sample_buffer);
                std::free(mix_s16);

                bundle.bed = AudioData{.sample_buffer = reinterpret_cast<u8*>(native),
                                       .num_samples = granularity,
                                       .num_channels = bed_ch,
                                       .format = OrbisAudio3dFormat::ORBIS_AUDIO3D_FORMAT_S16};
                port.spatial_queue.push_back(std::move(bundle));

                RetireDrainedObjectsLocked(port);
                return ORBIS_OK;
            }
        }

        bundle.bed = out_frame;
        port.spatial_queue.push_back(std::move(bundle));

        RetireDrainedObjectsLocked(port);
    } else {
        port.mixed_queue.push_back(out_frame);

        std::erase_if(port.objects, [](const auto& kv) {
            return kv.second.unreserved && kv.second.pcm_queue.empty();
        });
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortAdvance(const OrbisAudio3dPortId port_id) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}", port_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};

    if (port.parameters.buffer_mode == OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_NO_ADVANCE) {
        LOG_ERROR(Lib_Audio3d, "port doesn't have advance capability");
        return ORBIS_AUDIO3D_ERROR_NOT_SUPPORTED;
    }

    const bool spatial = EnsureSpatial(port);

    if ((spatial ? port.spatial_queue.size() : port.mixed_queue.size()) >=
        port.parameters.queue_depth) {
        LOG_WARNING(Lib_Audio3d, "queue full (depth={}), dropping advance",
                    port.parameters.queue_depth);
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    return ProcessMixQueue(port);
}

s32 PS4_SYSV_ABI sceAudio3dPortClose(const OrbisAudio3dPortId port_id) {
    LOG_INFO(Lib_Audio3d, "called, port_id = {}", port_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    {
        std::scoped_lock lock{port.mutex};

        DestroySpatial(port);

        if (port.audio_out_handle >= 0) {
            AudioOut::sceAudioOutClose(port.audio_out_handle);
            port.audio_out_handle = -1;
        }

        for (const auto& aout : port.audioout_ports) {
            AudioOut::sceAudioOutClose(aout.handle);
        }
        port.audioout_ports.clear();

        for (auto& data : port.mixed_queue) {
            std::free(data.sample_buffer);
        }

        for (auto& data : port.bed_queue) {
            std::free(data.sample_buffer);
        }

        for (auto& [obj_id, obj] : port.objects) {
            for (auto& data : obj.pcm_queue) {
                std::free(data.sample_buffer);
            }
        }
    }

    state->ports.erase(port_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortCreate(u32 granularity, u32 rate, s64 reserved,
                                      OrbisAudio3dPortId* port_id) {

    LOG_INFO(Lib_Audio3d, "called, granularity = {}, rate = {}, reserved = {}, port_id = {}",
             granularity, rate, reserved, static_cast<void*>(port_id));

    if (!port_id || reserved) {
        LOG_INFO(Lib_Audio3d, "!port_id || reserved");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    OrbisAudio3dOpenParameters local_params{};
    local_params.size_this = 0x10;
    local_params.granularity = granularity;
    local_params.rate = static_cast<OrbisAudio3dRate>(rate);
    return sceAudio3dPortOpen(static_cast<Libraries::UserService::OrbisUserServiceUserId>(0xFF),
                              &local_params, port_id);
}

s32 PS4_SYSV_ABI sceAudio3dPortDestroy() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFlush(const OrbisAudio3dPortId port_id) {
    LOG_DEBUG(Lib_Audio3d, "called, port_id = {}", port_id);

    if (!state->ports.contains(port_id)) {
        LOG_ERROR(Lib_Audio3d, "!state->ports.contains(port_id)");
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};

    if (!port.audioout_ports.empty()) {
        if (const s32 ret = DrainAssociatedPorts(port); ret < 0) {
            return ret;
        }
        for (const auto& aout : port.audioout_ports) {
            const s32 ret = AudioOut::sceAudioOutOutput(aout.handle, nullptr);
            if (ret < 0) {
                return ret;
            }
        }
        return ORBIS_OK;
    }

    if (port.mixed_queue.empty() && port.spatial_queue.empty()) {
        if (!port.bed_queue.empty() ||
            std::any_of(port.objects.begin(), port.objects.end(),
                        [](const auto& kv) { return !kv.second.pcm_queue.empty(); })) {
            const s32 ret = ProcessMixQueue(port);
            if (ret != ORBIS_OK && ret != ORBIS_AUDIO3D_ERROR_NOT_READY) {
                return ret;
            }
        }
    }

    if (port.mixed_queue.empty() && port.spatial_queue.empty()) {
        return ORBIS_OK;
    }

    const bool spatial = EnsureSpatial(port);

    if (!spatial && port.audio_out_handle < 0) {
        AudioOut::OrbisAudioOutParamExtendedInformation ext_info{};
        ext_info.data_format.Assign(AUDIO3D_OUTPUT_FORMAT);
        port.audio_out_handle =
            AudioOut::sceAudioOutOpen(0xFF, AudioOut::OrbisAudioOutPort::Audio3d, 0,
                                      port.parameters.granularity, AUDIO3D_SAMPLE_RATE, ext_info);
        if (port.audio_out_handle < 0) {
            return port.audio_out_handle;
        }
    }

    while (!port.mixed_queue.empty()) {
        AudioData frame = port.mixed_queue.front();
        port.mixed_queue.pop_front();

        s32 ret;
        if (spatial) {
            ret = SpatialSubmitFrame(port, frame);
        } else {
            ret = AudioOut::sceAudioOutOutput(port.audio_out_handle, frame.sample_buffer);
            std::free(frame.sample_buffer);
        }
        if (ret < 0) {
            return ret;
        }
    }

    while (!port.spatial_queue.empty()) {
        SpatialFrameBundle bundle = std::move(port.spatial_queue.front());
        port.spatial_queue.pop_front();

        const s32 ret = SpatialSubmitBundle(port, bundle);
        if (ret < 0) {
            return ret;
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortFreeState() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported(OrbisAudio3dPortId port_id,
                                                      OrbisAudio3dAttributeId* capabilities,
                                                      u32* num_capabilities) {
    LOG_DEBUG(Lib_Audio3d, "called");
    if (!num_capabilities) {
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (!state->ports.contains(port_id)) {
        return ORBIS_AUDIO3D_ERROR_INVALID_PORT;
    }

    static constexpr std::array supported = {
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_PCM,
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_POSITION,
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_GAIN,
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_SPREAD,
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_PASSTHROUGH,
        OrbisAudio3dAttributeId::ORBIS_AUDIO3D_ATTRIBUTE_RESET_STATE,
    };

    if (capabilities) {
        const u32 caps_to_write = std::min<u32>(*num_capabilities, supported.size());
        for (u32 i = 0; i < caps_to_write; i++) {
            capabilities[i] = supported[i];
        }
        *num_capabilities = caps_to_write;
    } else {
        *num_capabilities = static_cast<u32>(supported.size());
    }
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

    const auto& port = state->ports[port_id];
    std::scoped_lock lock{port.mutex};
    const size_t size = port.mixed_queue.size() + port.spatial_queue.size();

    if (queue_level) {
        *queue_level = static_cast<u32>(size);
    }

    if (queue_available) {
        const u32 depth = port.parameters.queue_depth;
        *queue_available = (size < depth) ? static_cast<u32>(depth - size) : 0u;
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
    LOG_INFO(Lib_Audio3d, "called, user_id = {}, parameters = {}, port_id = {}", user_id,
             static_cast<const void*>(parameters), static_cast<void*>(port_id));

    if (user_id != 0xFF || !parameters || !port_id) {
        LOG_ERROR(Lib_Audio3d, "user_id != 0xFF || !parameters || !port_id");
        if (port_id)
            *port_id = ORBIS_AUDIO3D_PORT_INVALID;

        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    *port_id = ORBIS_AUDIO3D_PORT_INVALID;

    if (!state) {
        LOG_ERROR(Lib_Audio3d, "!initialized");
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    OrbisAudio3dOpenParameters effective{
        .size_this = 0x28,
        .granularity = parameters->granularity,
        .rate = parameters->rate,
        .max_objects = 512,
        .queue_depth = 2,
        .buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_NO_ADVANCE,
        ._pad = 0,
        .num_beds = 2,
    };

    switch (parameters->size_this & ~0x7ull) {
    case 0x10:
        break;

    case 0x18:
        effective.max_objects = parameters->max_objects;
        effective.queue_depth = parameters->queue_depth;
        effective.buffer_mode = OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_NO_PUSH;
        break;

    case 0x20:
        effective.max_objects = parameters->max_objects;
        effective.queue_depth = parameters->queue_depth;
        effective.buffer_mode = parameters->buffer_mode;
        break;

    case 0x28:
        effective.max_objects = parameters->max_objects;
        effective.queue_depth = parameters->queue_depth;
        effective.buffer_mode = parameters->buffer_mode;
        effective.num_beds = parameters->num_beds;
        break;

    default:
        LOG_ERROR(Lib_Audio3d, "invalid size_this");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.rate != OrbisAudio3dRate::ORBIS_AUDIO3D_RATE_48000) {
        LOG_ERROR(Lib_Audio3d, "unsupported rate");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.granularity < 0x100) {
        LOG_ERROR(Lib_Audio3d, "granularity < 0x100");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if ((effective.granularity & 0xFF) != 0) {
        LOG_ERROR(Lib_Audio3d, "granularity not aligned to 0x100");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.max_objects == 0) {
        LOG_ERROR(Lib_Audio3d, "max_objects == 0");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.queue_depth == 0) {
        LOG_ERROR(Lib_Audio3d, "queue_depth == 0");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.granularity == 0x100 && effective.queue_depth > 0x40) {
        LOG_ERROR(Lib_Audio3d, "queue_depth too large for 0x100 granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.granularity == 0x200 && effective.queue_depth > 0x1F) {
        LOG_ERROR(Lib_Audio3d, "queue_depth too large for 0x200 granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.granularity == 0x300 && effective.queue_depth > 0x14) {
        LOG_ERROR(Lib_Audio3d, "queue_depth too large for 0x300 granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.queue_depth > 0xF && effective.granularity > 0x3FF) {
        LOG_ERROR(Lib_Audio3d, "queue_depth invalid for large granularity");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (static_cast<u32>(effective.buffer_mode) > 2) {
        LOG_ERROR(Lib_Audio3d, "invalid buffer_mode");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if ((effective.num_beds & 0xfffffffe) != 2) {
        LOG_ERROR(Lib_Audio3d, "invalid num_beds");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }

    if (effective.max_objects > 0x200) {
        LOG_WARNING(Lib_Audio3d, "max_objects {} exceeds limit, clamping to 512",
                    effective.max_objects);
        effective.max_objects = 0x200;
    }

    std::scoped_lock lock{state->ports_mutex};

    OrbisAudio3dPortId id = ORBIS_AUDIO3D_PORT_INVALID;
    for (OrbisAudio3dPortId i = 0; i < MaxPorts; i++) {
        if (!state->ports.contains(i)) {
            id = i;
            break;
        }
    }

    if (id == ORBIS_AUDIO3D_PORT_INVALID) {
        LOG_ERROR(Lib_Audio3d, "no free ports");
        return ORBIS_AUDIO3D_ERROR_OUT_OF_RESOURCES;
    }

    auto& port = state->ports.try_emplace(id).first->second;
    port.parameters = effective;

    *port_id = id;

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

    if (port.parameters.buffer_mode !=
        OrbisAudio3dBufferMode::ORBIS_AUDIO3D_BUFFER_ADVANCE_AND_PUSH) {
        LOG_ERROR(Lib_Audio3d, "port doesn't have push capability");
        return ORBIS_AUDIO3D_ERROR_NOT_SUPPORTED;
    }

    const u32 depth = port.parameters.queue_depth;

    if (const s32 ret = DrainAssociatedPorts(port); ret < 0) {
        return ret;
    }

    bool spatial;
    {
        std::scoped_lock lock{port.mutex};
        spatial = EnsureSpatial(port);
    }

    if (!spatial && port.audio_out_handle < 0) {
        AudioOut::OrbisAudioOutParamExtendedInformation ext_info{};
        ext_info.data_format.Assign(AUDIO3D_OUTPUT_FORMAT);

        port.audio_out_handle =
            AudioOut::sceAudioOutOpen(0xFF, AudioOut::OrbisAudioOutPort::Audio3d, 0,
                                      port.parameters.granularity, AUDIO3D_SAMPLE_RATE, ext_info);

        if (port.audio_out_handle < 0)
            return port.audio_out_handle;
    }

    auto submit_one_frame = [&](bool& submitted, const bool nonblocking) -> s32 {
        if (spatial) {
            SpatialFrameBundle bundle{};
            bool from_spatial_queue = false;
            {
                std::scoped_lock lock{port.mutex};

                if (!port.mixed_queue.empty()) {
                    bundle.bed = port.mixed_queue.front();
                    port.mixed_queue.pop_front();
                } else if (!port.spatial_queue.empty()) {
                    bundle = std::move(port.spatial_queue.front());
                    port.spatial_queue.pop_front();
                    from_spatial_queue = true;
                } else {
                    submitted = false;
                    return ORBIS_OK;
                }
            }

            const s32 ret = SpatialSubmitBundle(port, bundle, nonblocking);
            if (ret == ORBIS_AUDIO3D_ERROR_NOT_READY) {
                std::scoped_lock lock{port.mutex};
                if (from_spatial_queue) {
                    port.spatial_queue.push_front(std::move(bundle));
                } else {
                    port.mixed_queue.push_front(bundle.bed);
                }
                submitted = false;
                return ORBIS_OK;
            }
            if (ret < 0)
                return ret;

            submitted = true;
            return ORBIS_OK;
        }

        AudioData frame;
        {
            std::scoped_lock lock{port.mutex};

            if (port.mixed_queue.empty()) {
                submitted = false;
                return ORBIS_OK;
            }

            frame = port.mixed_queue.front();
            port.mixed_queue.pop_front();
        }

        const s32 ret = AudioOut::sceAudioOutOutput(port.audio_out_handle, frame.sample_buffer);
        std::free(frame.sample_buffer);

        if (ret < 0)
            return ret;

        submitted = true;
        return ORBIS_OK;
    };

    {
        std::scoped_lock lock{port.mutex};
        if (port.mixed_queue.size() + port.spatial_queue.size() < depth) {
            return ORBIS_OK;
        }
    }

    const bool async = blocking == OrbisAudio3dBlocking::ORBIS_AUDIO3D_BLOCKING_ASYNC;
    bool submitted = false;
    s32 ret = submit_one_frame(submitted, async);
    if (ret < 0)
        return ret;

    if (!submitted)
        return ORBIS_OK;

    if (async) {
        return ORBIS_OK;
    }

    while (true) {
        {
            std::scoped_lock lock{port.mutex};
            if (port.mixed_queue.size() + port.spatial_queue.size() < depth)
                break;
        }

        bool drained = false;
        ret = submit_one_frame(drained, /*nonblocking=*/false);
        if (ret < 0)
            return ret;

        if (!drained)
            break;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortSetAttribute(const OrbisAudio3dPortId port_id,
                                            const OrbisAudio3dPortAttributeId attribute_id,
                                            void* attribute, const u64 attribute_size) {
    LOG_DEBUG(Lib_Audio3d,
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
    std::scoped_lock lock{port.mutex};

    switch (attribute_id) {
    case OrbisAudio3dPortAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_LATE_REVERB_LEVEL: {
        if (attribute_size < sizeof(float)) {
            LOG_ERROR(Lib_Audio3d, "LATE_REVERB_LEVEL attribute_size < sizeof(float)");
            return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
        }
        float level = 0.0f;
        std::memcpy(&level, attribute, sizeof(float));
        level = std::clamp(level, 0.0f, 1.0f);

        if (std::abs(level - port.late_reverb_level) < 0.001f) {
            return ORBIS_OK;
        }

        port.late_reverb_level = level;
        SpatialApplyReverbLevelLocked(port);
        LOG_INFO(Lib_Audio3d, "late reverb level = {}", port.late_reverb_level);
        return ORBIS_OK;
    }
    case OrbisAudio3dPortAttributeId::ORBIS_AUDIO3D_PORT_ATTRIBUTE_DOWNMIX_SPREAD_RADIUS: {
        if (attribute_size < sizeof(float)) {
            LOG_ERROR(Lib_Audio3d, "DOWNMIX_SPREAD_RADIUS attribute_size < sizeof(float)");
            return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
        }
        float radius = 2.0f;
        std::memcpy(&radius, attribute, sizeof(float));
        port.downmix_spread_radius = std::clamp(radius, 0.1f, 2.0f);
        LOG_DEBUG(Lib_Audio3d, "downmix spread radius = {}", port.downmix_spread_radius);
        return ORBIS_OK;
    }
    default:
        LOG_WARNING(Lib_Audio3d, "unknown port attribute {:#x} (size {})",
                    static_cast<u32>(attribute_id), attribute_size);
        return ORBIS_OK;
    }
}

s32 PS4_SYSV_ABI sceAudio3dTerminate() {
    LOG_INFO(Lib_Audio3d, "called");

    if (!state) {
        return ORBIS_AUDIO3D_ERROR_NOT_READY;
    }

    std::vector<OrbisAudio3dPortId> port_ids;
    for (const auto& [id, _] : state->ports) {
        port_ids.push_back(id);
    }
    for (const auto id : port_ids) {
        sceAudio3dPortClose(id);
    }

    state.reset();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dPortQueryDebug() {
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
    LIB_FUNCTION("V1FBFpNIAzk", "libSceAudio3d", 1, "libSceAudio3d", sceAudio3dObjectSetAttribute);
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
