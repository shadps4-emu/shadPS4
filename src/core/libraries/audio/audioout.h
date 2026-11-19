// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "common/bit_field.h"
#include "core/libraries/audio/audioout_mastering.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::AudioOut {

class PortBackend;

// Main up to 8 ports, BGM 1 port, voice up to 4 ports,
// personal up to 4 ports, padspk up to 4 ports, aux 1 port
constexpr s32 ORBIS_AUDIO_OUT_NUM_PORTS = 25;
constexpr s32 ORBIS_AUDIO_OUT_VOLUME_0DB = 32768; // max volume value

constexpr s32 ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_DEFAULT = 11626; // default -9db
constexpr s32 ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_0DB = 32768;     // max volume

constexpr s32 ORBIS_AUDIO_OUT_PARAM_ATTR_RESTRICTED = 0x00010000;
constexpr s32 ORBIS_AUDIO_OUT_PARAM_ATTR_MIX_TO_MAIN = 0x00020000;
constexpr u32 ORBIS_AUDIO_OUT_PARAM_ATTR_MASK = 0x000F0000;
constexpr u32 ORBIS_AUDIO_OUT_PARAM_ATTR_SHIFT = 16;

// Channel indices into the vol[] array of sceAudioOutSetVolume
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_L = 0;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_R = 1;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_C = 2;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_LFE = 3;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_LS = 4;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_RS = 5;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_LE = 6;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_RE = 7;
constexpr u32 ORBIS_AUDIO_OUT_CHANNEL_MAX = 8;

// Volume flags (bits 4-7 are swapped pairwise for the *_8CH_STD formats)
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_L_CH = (1u << 0);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_R_CH = (1u << 1);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_C_CH = (1u << 2);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LFE_CH = (1u << 3);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LS_CH = (1u << 4);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_RS_CH = (1u << 5);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LE_CH = (1u << 6);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_RE_CH = (1u << 7);

constexpr float ORBIS_AUDIO_OUT_STATE_LOUDNESS_MIN = -70.0f;

enum class OrbisAudioOutSystemDebugStateElement : u32 {
    Loudness = 1,
};

constexpr u32 ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_STOP = 0;
constexpr u32 ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_START = 1;
constexpr u32 ORBIS_AUDIO_OUT_DEBUG_STATE_LOUDNESS_START_QUIET = 2;

struct OrbisAudioOutSystemDebugStateParam {
    u32 value;
    u32 reserved[3];
};

// Port state constants
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_UNKNOWN = 0x00;
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_PRIMARY = 0x01;
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_SECONDARY = 0x02;
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_TERTIARY = 0x04;
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_HEADPHONE = 0x40;
constexpr u16 ORBIS_AUDIO_OUT_STATE_OUTPUT_CONNECTED_EXTERNAL = 0x80;

enum class OrbisAudioOutPort {
    Main = 0,
    Bgm = 1,
    Voice = 2,
    Personal = 3,
    PadSpk = 4,
    Audio3d = 126,
    Aux = 127,
};

enum class OrbisAudioOutParamFormat : u32 {
    S16Mono = 0,
    S16Stereo = 1,
    S16_8CH = 2,
    FloatMono = 3,
    FloatStereo = 4,
    Float_8CH = 5,
    S16_8CH_Std = 6,
    Float_8CH_Std = 7
};

enum class OrbisAudioOutParamAttr : u32 {
    None = 0,
    Restricted = 1,
    MixToMain = 2,
    RestrictedMixToMain = 3,
};

union OrbisAudioOutParamExtendedInformation {
    BitField<0, 8, OrbisAudioOutParamFormat> data_format;
    BitField<8, 8, u32> reserve0;
    BitField<16, 4, OrbisAudioOutParamAttr> attributes;
    BitField<20, 11, u32> reserve1;
    BitField<31, 1, u32> unused;
    u32 Unpack() const {
        return *reinterpret_cast<const u32*>(this);
    }
};

struct OrbisAudioOutOutputParam {
    s32 handle;
    void* ptr;
};

struct OrbisAudioOutPortState {
    u16 output;
    u8 channel;
    u8 reserved8_1[1];
    s16 volume;
    u16 rerouteCounter;
    u64 flag;
    u64 reserved64[2];
};

struct OrbisAudioOutSystemState {
    float loudness;
    u8 reserved8[4];
    u64 reserved64[3];
};

// Mastering
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_DEFAULT = 1;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_V2 = 2;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_STATES_ID_DEFAULT = 1;

constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_STRUCT_ID_PARAM_EQ_DEFAULT = 0x01010000;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_STRUCT_ID_COMPRESSOR_DEFAULT = 0x01010001;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_STRUCT_ID_MASTER_VOLUME_DEFAULT = 0x01010002;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_STRUCT_ID_LIMITER_DEFAULT = 0x01010003;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_PARAMS_STRUCT_ID_MASTER_VOLUME_V2 = 0x01020002;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_STATES_STRUCT_ID_COMPRESSOR_DEFAULT = 0x01010001;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_STATES_STRUCT_ID_LIMITER_DEFAULT = 0x01010003;

// moduleBypassFlags
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_PARAM_EQ = 1;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_COMPRESSOR = 2;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_MASTER_VOLUME = 4;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_LIMITER = 8;

// Compressor bandsBypassFlags
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_BAND0 = 1;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_BAND1 = 2;
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_BYPASSED_BAND2 = 4;

// sceAudioOutMasteringSetParam flags
constexpr u32 ORBIS_AUDIO_OUT_MASTERING_MODULE_FLAG_SWAP_EQ_COMPRESSOR = 0x00000001;

enum class OrbisAudioOutMasteringFilterMode : u32 {
    Off = 0,
    LowpassResonant = 1,
    HighpassResonant = 2,
    Peak = 3,
    HighShelf = 4,
    LowShelf = 5,
    LowpassOnePole = 6,
    HighpassOnePole = 7,
};

enum class OrbisAudioOutMasteringCompressorLinkMode : u32 {
    NoLink = 0,
    AllChannels = 1,
    PartialLink = 2,
};

enum class OrbisAudioOutMasteringLimiterMode : u32 {
    Soft = 0,
    Hard = 1,
};

struct OrbisAudioOutMasteringParamsHeader {
    u32 paramsId;
};

struct OrbisAudioOutMasteringParamsDescriptor {
    u32 id;
    u32 size;
};

struct OrbisAudioOutMasteringParamEqFilterParams {
    OrbisAudioOutMasteringFilterMode filterMode;
    float frequency; // Hz, 10.0 to 24000.0
    float resonance; // Q, 0.1 to 12.0
    float gain;      // dB, -20.0 to 20.0
};

struct OrbisAudioOutMasteringParamEqParams {
    OrbisAudioOutMasteringParamsDescriptor descriptor;
    u32 numBands; // 1 to 4
    u32 reserved;
    OrbisAudioOutMasteringParamEqFilterParams filterParams[4];
};

struct OrbisAudioOutMasteringCompressorBandParams {
    float threshold;  // dB, -60.0 to 0.0
    float ratio;      // 1.0 to 30.0
    float attack;     // seconds, 0.0 to 0.5
    float release;    // seconds, 0.0 to 5.0
    float makeupGain; // dB, -18.0 to 18.0
    float knee;       // dB, 0.0 to 20.0 (0.0 = hard knee)
};

struct OrbisAudioOutMasteringCompressorParams {
    OrbisAudioOutMasteringParamsDescriptor descriptor;
    u32 numBands; // 1 to 3
    OrbisAudioOutMasteringCompressorLinkMode linkMode;
    float linkStrength; // %, 0.0 to 100.0
    u32 linkStereoPairs;
    u32 bandsBypassFlags;
    u32 reserved[2];
    float crossoverFrequencies[2]; // Hz, 10.0 to 24000.0
    OrbisAudioOutMasteringCompressorBandParams bandParams[3];
};

struct OrbisAudioOutMasteringMasterVolumeParams {
    OrbisAudioOutMasteringParamsDescriptor descriptor;
    float volume; // dB, -100.0 to 12.0
    u32 reserved;
};

struct OrbisAudioOutMasteringMasterVolumeParamsV2 {
    OrbisAudioOutMasteringParamsDescriptor descriptor;
    float volume[8]; // dB, -100.0 to 12.0
    u32 reserved[2];
};

struct OrbisAudioOutMasteringLimiterParams {
    OrbisAudioOutMasteringParamsDescriptor descriptor;
    OrbisAudioOutMasteringLimiterMode mode;
    float threshold; // dB, -30.0 to 0.0
    float attack;    // seconds, 0.0 to 2.6
    float release;   // seconds, 0.0 to 5.0
    u32 reserved[2];
};

struct OrbisAudioOutMasteringParams {
    OrbisAudioOutMasteringParamsHeader paramsHeader;
    u32 moduleBypassFlags;
    OrbisAudioOutMasteringParamEqParams paramEqParams;
    OrbisAudioOutMasteringCompressorParams compressorParams;
    OrbisAudioOutMasteringMasterVolumeParams masterVolumeParams;
    OrbisAudioOutMasteringLimiterParams limiterParams;
    u32 reserved[2];
};

struct OrbisAudioOutMasteringParamsV2 {
    OrbisAudioOutMasteringParamsHeader paramsHeader;
    u32 moduleBypassFlags;
    OrbisAudioOutMasteringParamEqParams paramEqParams;
    OrbisAudioOutMasteringCompressorParams compressorParams;
    OrbisAudioOutMasteringMasterVolumeParamsV2 masterVolumeParams;
    OrbisAudioOutMasteringLimiterParams limiterParams;
    u32 reserved[2];
};

struct OrbisAudioOutMasteringStatesHeader {
    u32 statesId;
};

struct OrbisAudioOutMasteringStatesDescriptor {
    u32 id;
    u32 size;
};

struct OrbisAudioOutMasteringCompressorStates {
    OrbisAudioOutMasteringStatesDescriptor descriptor;
    u32 reserved[2];
    float inputRms[3][8];         // [band][channel], linear scale
    float compressionCoeff[3][8]; // [band][channel]
};

struct OrbisAudioOutMasteringLimiterStates {
    OrbisAudioOutMasteringStatesDescriptor descriptor;
    u32 reserved[2];
    float inputPeak[8];  // absolute linear value
    float outputPeak[8]; // absolute linear value
    float gainPeak[8];   // linear gain applied
};

struct OrbisAudioOutMasteringStates {
    OrbisAudioOutMasteringStatesHeader statesHeader;
    u32 reserved[3];
    OrbisAudioOutMasteringCompressorStates compressorStates;
    OrbisAudioOutMasteringLimiterStates limiterStates;
};

static_assert(sizeof(OrbisAudioOutMasteringParamEqParams) == 80);
static_assert(sizeof(OrbisAudioOutMasteringCompressorParams) == 116);
static_assert(sizeof(OrbisAudioOutMasteringLimiterParams) == 32);
static_assert(sizeof(OrbisAudioOutMasteringParams) == 260);
static_assert(sizeof(OrbisAudioOutMasteringParamsV2) == 292);
static_assert(sizeof(OrbisAudioOutMasteringStates) == 336);

struct AudioFormatInfo {
    bool is_float;
    u8 sample_size;
    u8 num_channels;
    /// Layout array remapping channel indices, specified in this order:
    /// FL, FR, FC, LFE, BL, BR, SL, SR
    std::array<int, 8> channel_layout;
    bool is_std;

    [[nodiscard]] u16 FrameSize() const {
        return sample_size * num_channels;
    }
};

struct PortOut {
    std::mutex mutex;
    std::unique_ptr<PortBackend> impl{};

    void* output_buffer = nullptr;
    std::condition_variable_any output_cv;
    bool output_ready = false;
    bool closing = false;
    u32 output_waiters = 0;
    Kernel::Thread output_thread{};

    OrbisAudioOutPort type;
    AudioFormatInfo format_info;
    u32 sample_rate = 48000;
    u32 buffer_frames = 1024;
    u64 last_output_time = 0;
    /// Per-channel volume, indexed by ORBIS_AUDIO_OUT_CHANNEL_*.
    std::array<s32, 8> volume;
    s32 userId = 0;
    s32 mixLevelPadSpk = ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_DEFAULT;
    bool is_restricted = false;
    bool is_mix_to_main = false;
    /// PADSPK only: no controller speaker device is available, so the port is mixed with
    /// the MAIN output at mixLevelPadSpk instead.
    bool padspk_mixed_to_main = false;
    /// MAIN/BGM only: mastering chain for this port and the parameter generation it has
    /// loaded
    MasteringProcessor mastering;
    u32 mastering_generation = 0;

    [[nodiscard]] u32 BufferSize() const {
        return buffer_frames * format_info.FrameSize();
    }
};

s32 PS4_SYSV_ABI sceAudioOutDeviceIdOpen();
s32 PS4_SYSV_ABI sceAudioDeviceControlGet();
s32 PS4_SYSV_ABI sceAudioDeviceControlSet();
s32 PS4_SYSV_ABI sceAudioOutA3dControl();
s32 PS4_SYSV_ABI sceAudioOutA3dExit();
s32 PS4_SYSV_ABI sceAudioOutA3dInit();
s32 PS4_SYSV_ABI sceAudioOutAttachToApplicationByPid();
s32 PS4_SYSV_ABI sceAudioOutChangeAppModuleState();
s32 PS4_SYSV_ABI sceAudioOutClose(s32 handle);
s32 PS4_SYSV_ABI sceAudioOutDetachFromApplicationByPid();
s32 PS4_SYSV_ABI sceAudioOutExConfigureOutputMode();
s32 PS4_SYSV_ABI sceAudioOutExGetSystemInfo();
s32 PS4_SYSV_ABI sceAudioOutExPtClose();
s32 PS4_SYSV_ABI sceAudioOutExPtGetLastOutputTime();
s32 PS4_SYSV_ABI sceAudioOutExPtOpen();
s32 PS4_SYSV_ABI sceAudioOutExSystemInfoIsSupportedAudioOutExMode();
s32 PS4_SYSV_ABI sceAudioOutGetFocusEnablePid();
s32 PS4_SYSV_ABI sceAudioOutGetHandleStatusInfo();
s32 PS4_SYSV_ABI sceAudioOutGetInfo();
s32 PS4_SYSV_ABI sceAudioOutGetInfoOpenNum();
s32 PS4_SYSV_ABI sceAudioOutGetLastOutputTime(s32 handle, u64* output_time);
s32 PS4_SYSV_ABI sceAudioOutGetPortState(s32 handle, OrbisAudioOutPortState* state);
s32 PS4_SYSV_ABI sceAudioOutGetSimulatedBusUsableStatusByBusType();
s32 PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo();
s32 PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo2();
s32 PS4_SYSV_ABI sceAudioOutGetSparkVss();
s32 PS4_SYSV_ABI sceAudioOutGetSystemState(OrbisAudioOutSystemState* state);
s32 PS4_SYSV_ABI sceAudioOutInit();
s32 PS4_SYSV_ABI sceAudioOutInitIpmiGetSession();
s32 PS4_SYSV_ABI sceAudioOutMasteringGetState(OrbisAudioOutMasteringStatesHeader* state);
s32 PS4_SYSV_ABI sceAudioOutMasteringInit(u32 flags);
s32 PS4_SYSV_ABI sceAudioOutMasteringSetParam(const OrbisAudioOutMasteringParamsHeader* param,
                                              u32 flags);
s32 PS4_SYSV_ABI sceAudioOutMasteringTerm();
s32 PS4_SYSV_ABI sceAudioOutMbusInit();
s32 PS4_SYSV_ABI sceAudioOutOpen(UserService::OrbisUserServiceUserId user_id,
                                 OrbisAudioOutPort port_type, s32 index, u32 length,
                                 u32 sample_rate, OrbisAudioOutParamExtendedInformation param_type);
s32 PS4_SYSV_ABI sceAudioOutOpenEx();
s32 PS4_SYSV_ABI sceAudioOutOutput(s32 handle, void* ptr);
s32 PS4_SYSV_ABI sceAudioOutOutputs(OrbisAudioOutOutputParam* param, u32 num);
s32 PS4_SYSV_ABI sceAudioOutPtClose();
s32 PS4_SYSV_ABI sceAudioOutPtGetLastOutputTime();
s32 PS4_SYSV_ABI sceAudioOutPtOpen();
s32 PS4_SYSV_ABI sceAudioOutSetConnections();
s32 PS4_SYSV_ABI sceAudioOutSetConnectionsForUser();
s32 PS4_SYSV_ABI sceAudioOutSetDevConnection();
s32 PS4_SYSV_ABI sceAudioOutSetHeadphoneOutMode();
s32 PS4_SYSV_ABI sceAudioOutSetJediJackVolume();
s32 PS4_SYSV_ABI sceAudioOutSetJediSpkVolume();
s32 PS4_SYSV_ABI sceAudioOutSetMainOutput();
s32 PS4_SYSV_ABI sceAudioOutSetMixLevelPadSpk(s32 handle, s32 mixLevel);
s32 PS4_SYSV_ABI sceAudioOutSetMorpheusParam();
s32 PS4_SYSV_ABI sceAudioOutSetMorpheusWorkingMode();
s32 PS4_SYSV_ABI sceAudioOutSetPortConnections();
s32 PS4_SYSV_ABI sceAudioOutSetPortStatuses();
s32 PS4_SYSV_ABI sceAudioOutSetRecMode();
s32 PS4_SYSV_ABI sceAudioOutSetSparkParam();
s32 PS4_SYSV_ABI sceAudioOutSetUsbVolume();
s32 PS4_SYSV_ABI sceAudioOutSetVolume(s32 handle, s32 flag, s32* vol);
s32 PS4_SYSV_ABI sceAudioOutSetVolumeDown();
s32 PS4_SYSV_ABI sceAudioOutStartAuxBroadcast();
s32 PS4_SYSV_ABI sceAudioOutStartSharePlay();
s32 PS4_SYSV_ABI sceAudioOutStopAuxBroadcast();
s32 PS4_SYSV_ABI sceAudioOutStopSharePlay();
s32 PS4_SYSV_ABI sceAudioOutSuspendResume();
s32 PS4_SYSV_ABI sceAudioOutSysConfigureOutputMode();
s32 PS4_SYSV_ABI sceAudioOutSysGetHdmiMonitorInfo();
s32 PS4_SYSV_ABI sceAudioOutSysGetSystemInfo();
s32 PS4_SYSV_ABI sceAudioOutSysHdmiMonitorInfoIsSupportedAudioOutMode();
s32 PS4_SYSV_ABI sceAudioOutSystemControlGet();
s32 PS4_SYSV_ABI sceAudioOutSystemControlSet();
s32 PS4_SYSV_ABI sceAudioOutSparkControlSetEqCoef();
s32 PS4_SYSV_ABI sceAudioOutSetSystemDebugState(OrbisAudioOutSystemDebugStateElement element,
                                                OrbisAudioOutSystemDebugStateParam* param);
s32 ForceClosePort(s32 handle);
void AdjustVol();
void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AudioOut
