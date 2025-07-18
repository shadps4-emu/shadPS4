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
constexpr s32 SCE_AUDIO_OUT_NUM_PORTS = 22;
constexpr s32 SCE_AUDIO_OUT_VOLUME_0DB = 32768; // max volume value

enum class OrbisAudioOutPort {
    Main = 0,
    Bgm = 1,
    Voice = 2,
    Personal = 3,
    Padspk = 4,
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

    [[nodiscard]] u16 FrameSize() const {
        return sample_size * num_channels;
    }
};

struct PortOut {
    std::mutex mutex;
    std::unique_ptr<PortBackend> impl{};

    void* output_buffer;
    std::condition_variable_any output_cv;
    bool output_ready;
    Kernel::Thread output_thread{};

    OrbisAudioOutPort type;
    AudioFormatInfo format_info;
    u32 sample_rate;
    u32 buffer_frames;
    std::array<s32, 8> volume;

    [[nodiscard]] bool IsOpen() const {
        return impl != nullptr;
    }

    [[nodiscard]] u32 BufferSize() const {
        return buffer_frames * format_info.FrameSize();
    }
};

int PS4_SYSV_ABI sceAudioOutDeviceIdOpen();
int PS4_SYSV_ABI sceAudioDeviceControlGet();
int PS4_SYSV_ABI sceAudioDeviceControlSet();
int PS4_SYSV_ABI sceAudioOutA3dControl();
int PS4_SYSV_ABI sceAudioOutA3dExit();
int PS4_SYSV_ABI sceAudioOutA3dInit();
int PS4_SYSV_ABI sceAudioOutAttachToApplicationByPid();
int PS4_SYSV_ABI sceAudioOutChangeAppModuleState();
int PS4_SYSV_ABI sceAudioOutClose(s32 handle);
int PS4_SYSV_ABI sceAudioOutDetachFromApplicationByPid();
int PS4_SYSV_ABI sceAudioOutExConfigureOutputMode();
int PS4_SYSV_ABI sceAudioOutExGetSystemInfo();
int PS4_SYSV_ABI sceAudioOutExPtClose();
int PS4_SYSV_ABI sceAudioOutExPtGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutExPtOpen();
int PS4_SYSV_ABI sceAudioOutExSystemInfoIsSupportedAudioOutExMode();
int PS4_SYSV_ABI sceAudioOutGetFocusEnablePid();
int PS4_SYSV_ABI sceAudioOutGetHandleStatusInfo();
int PS4_SYSV_ABI sceAudioOutGetInfo();
int PS4_SYSV_ABI sceAudioOutGetInfoOpenNum();
int PS4_SYSV_ABI sceAudioOutGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutGetPortState(s32 handle, OrbisAudioOutPortState* state);
int PS4_SYSV_ABI sceAudioOutGetSimulatedBusUsableStatusByBusType();
int PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo();
int PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo2();
int PS4_SYSV_ABI sceAudioOutGetSparkVss();
int PS4_SYSV_ABI sceAudioOutGetSystemState();
int PS4_SYSV_ABI sceAudioOutInit();
int PS4_SYSV_ABI sceAudioOutInitIpmiGetSession();
int PS4_SYSV_ABI sceAudioOutMasteringGetState();
int PS4_SYSV_ABI sceAudioOutMasteringInit();
int PS4_SYSV_ABI sceAudioOutMasteringSetParam();
int PS4_SYSV_ABI sceAudioOutMasteringTerm();
int PS4_SYSV_ABI sceAudioOutMbusInit();
s32 PS4_SYSV_ABI sceAudioOutOpen(UserService::OrbisUserServiceUserId user_id,
                                 OrbisAudioOutPort port_type, s32 index, u32 length,
                                 u32 sample_rate, OrbisAudioOutParamExtendedInformation param_type);
int PS4_SYSV_ABI sceAudioOutOpenEx();
s32 PS4_SYSV_ABI sceAudioOutOutput(s32 handle, void* ptr);
s32 PS4_SYSV_ABI sceAudioOutOutputs(OrbisAudioOutOutputParam* param, u32 num);
int PS4_SYSV_ABI sceAudioOutPtClose();
int PS4_SYSV_ABI sceAudioOutPtGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutPtOpen();
int PS4_SYSV_ABI sceAudioOutSetConnections();
int PS4_SYSV_ABI sceAudioOutSetConnectionsForUser();
int PS4_SYSV_ABI sceAudioOutSetDevConnection();
int PS4_SYSV_ABI sceAudioOutSetHeadphoneOutMode();
int PS4_SYSV_ABI sceAudioOutSetJediJackVolume();
int PS4_SYSV_ABI sceAudioOutSetJediSpkVolume();
int PS4_SYSV_ABI sceAudioOutSetMainOutput();
int PS4_SYSV_ABI sceAudioOutSetMixLevelPadSpk();
int PS4_SYSV_ABI sceAudioOutSetMorpheusParam();
int PS4_SYSV_ABI sceAudioOutSetMorpheusWorkingMode();
int PS4_SYSV_ABI sceAudioOutSetPortConnections();
int PS4_SYSV_ABI sceAudioOutSetPortStatuses();
int PS4_SYSV_ABI sceAudioOutSetRecMode();
int PS4_SYSV_ABI sceAudioOutSetSparkParam();
int PS4_SYSV_ABI sceAudioOutSetUsbVolume();
s32 PS4_SYSV_ABI sceAudioOutSetVolume(s32 handle, s32 flag, s32* vol);
int PS4_SYSV_ABI sceAudioOutSetVolumeDown();
int PS4_SYSV_ABI sceAudioOutStartAuxBroadcast();
int PS4_SYSV_ABI sceAudioOutStartSharePlay();
int PS4_SYSV_ABI sceAudioOutStopAuxBroadcast();
int PS4_SYSV_ABI sceAudioOutStopSharePlay();
int PS4_SYSV_ABI sceAudioOutSuspendResume();
int PS4_SYSV_ABI sceAudioOutSysConfigureOutputMode();
int PS4_SYSV_ABI sceAudioOutSysGetHdmiMonitorInfo();
int PS4_SYSV_ABI sceAudioOutSysGetSystemInfo();
int PS4_SYSV_ABI sceAudioOutSysHdmiMonitorInfoIsSupportedAudioOutMode();
int PS4_SYSV_ABI sceAudioOutSystemControlGet();
int PS4_SYSV_ABI sceAudioOutSystemControlSet();
int PS4_SYSV_ABI sceAudioOutSparkControlSetEqCoef();
int PS4_SYSV_ABI sceAudioOutSetSystemDebugState();

void AdjustVol();
void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AudioOut
