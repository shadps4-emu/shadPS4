// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "common/bit_field.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::AudioOut {

class PortBackend;

// Main up to 8 ports, BGM 1 port, voice up to 4 ports,
// personal up to 4 ports, padspk up to 5 ports, aux 1 port
constexpr s32 ORBIS_AUDIO_OUT_NUM_PORTS = 25;
constexpr s32 ORBIS_AUDIO_OUT_VOLUME_0DB = 32768; // max volume value

constexpr s32 ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_DEFAULT = 11626; // default -9db
constexpr s32 ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_0DB = 32768;     // max volume

constexpr s32 ORBIS_AUDIO_OUT_PARAM_ATTR_RESTRICTED = 0x00010000;
constexpr s32 ORBIS_AUDIO_OUT_PARAM_ATTR_MIX_TO_MAIN = 0x00020000;

// Volume flags
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_L_CH = (1u << 0);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_R_CH = (1u << 1);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_C_CH = (1u << 2);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LFE_CH = (1u << 3);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LS_CH = (1u << 4);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_RS_CH = (1u << 5);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_LE_CH = (1u << 6);
constexpr u32 ORBIS_AUDIO_VOLUME_FLAG_RE_CH = (1u << 7);

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
};

union OrbisAudioOutParamExtendedInformation {
    BitField<0, 8, OrbisAudioOutParamFormat> data_format;
    BitField<8, 8, u32> reserve0;
    BitField<16, 4, OrbisAudioOutParamAttr> attributes;
    BitField<20, 10, u32> reserve1;
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
    Kernel::Thread output_thread{};

    OrbisAudioOutPort type;
    AudioFormatInfo format_info;
    u32 sample_rate = 48000;
    u32 buffer_frames = 1024;
    u64 last_output_time = 0;
    std::array<s32, 8> volume;
    s32 userId = 0;
    s32 mixLevelPadSpk = ORBIS_AUDIO_OUT_MIXLEVEL_PADSPK_DEFAULT;
    bool is_restricted = false;
    bool is_mix_to_main = false;

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
s32 PS4_SYSV_ABI sceAudioOutGetSystemState();
s32 PS4_SYSV_ABI sceAudioOutInit();
s32 PS4_SYSV_ABI sceAudioOutInitIpmiGetSession();
s32 PS4_SYSV_ABI sceAudioOutMasteringGetState();
s32 PS4_SYSV_ABI sceAudioOutMasteringInit(u32 flags);
s32 PS4_SYSV_ABI sceAudioOutMasteringSetParam();
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
s32 PS4_SYSV_ABI sceAudioOutSetSystemDebugState();

void AdjustVol();
void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AudioOut
