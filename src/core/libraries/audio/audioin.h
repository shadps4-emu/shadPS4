// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <core/libraries/system/userservice.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AudioIn {

class PortInBackend;

constexpr s32 ORBIS_AUDIO_IN_NUM_PORTS = 7;

enum class OrbisAudioInParamFormat : u32 { S16Mono = 0, S16Stereo = 2 };

enum class OrbisAudioInType : u32 { VoiceChat = 0, General = 1, VoiceRecognition = 5 };

constexpr int ORBIS_AUDIO_IN_SILENT_STATE_DEVICE_NONE = 0x00000001;
constexpr int ORBIS_AUDIO_IN_SILENT_STATE_PRIORITY_LOW = 0x00000002;
constexpr int ORBIS_AUDIO_IN_SILENT_STATE_USER_SETTING = 0x0000000;
constexpr int ORBIS_AUDIO_IN_SILENT_STATE_UNABLE_FORMAT = 0x00000008;

struct PortIn {
    std::mutex mutex;
    std::unique_ptr<PortInBackend> impl{};
    OrbisAudioInType type;
    OrbisAudioInParamFormat format;

    u32 samples_num = 0;
    u32 freq = 0;
    u32 channels_num = 0;
    u32 sample_size = 0;
};

int PS4_SYSV_ABI sceAudioInChangeAppModuleState();
int PS4_SYSV_ABI sceAudioInClose(s32 handle);
int PS4_SYSV_ABI sceAudioInCountPorts();
int PS4_SYSV_ABI sceAudioInDeviceHqOpen();
int PS4_SYSV_ABI sceAudioInDeviceIdHqOpen();
int PS4_SYSV_ABI sceAudioInDeviceIdOpen();
int PS4_SYSV_ABI sceAudioInDeviceOpen();
int PS4_SYSV_ABI sceAudioInDeviceOpenEx();
int PS4_SYSV_ABI sceAudioInExtClose();
int PS4_SYSV_ABI sceAudioInExtCtrl();
int PS4_SYSV_ABI sceAudioInExtInput();
int PS4_SYSV_ABI sceAudioInExtOpen();
int PS4_SYSV_ABI sceAudioInExtSetAecMode();
int PS4_SYSV_ABI sceAudioInGetGain();
int PS4_SYSV_ABI sceAudioInGetHandleStatusInfo();
int PS4_SYSV_ABI sceAudioInGetRerouteCount();
int PS4_SYSV_ABI sceAudioInGetSilentState(s32 handle);
int PS4_SYSV_ABI sceAudioInHqOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                  u32 index, u32 len, u32 freq, u32 param);
int PS4_SYSV_ABI sceAudioInHqOpenEx();
int PS4_SYSV_ABI sceAudioInInit();
int PS4_SYSV_ABI sceAudioInInput(s32 handle, void* dest);
int PS4_SYSV_ABI sceAudioInInputs();
int PS4_SYSV_ABI sceAudioInIsSharedDevice();
int PS4_SYSV_ABI sceAudioInOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                u32 index, u32 len, u32 freq, u32 param);
int PS4_SYSV_ABI sceAudioInOpenEx();
int PS4_SYSV_ABI sceAudioInSetAllMute();
int PS4_SYSV_ABI sceAudioInSetCompressorPreGain();
int PS4_SYSV_ABI sceAudioInSetConnections();
int PS4_SYSV_ABI sceAudioInSetConnectionsForUser();
int PS4_SYSV_ABI sceAudioInSetDevConnection();
int PS4_SYSV_ABI sceAudioInSetFocusForUser();
int PS4_SYSV_ABI sceAudioInSetMode();
int PS4_SYSV_ABI sceAudioInSetMode2();
int PS4_SYSV_ABI sceAudioInSetPortConnections();
int PS4_SYSV_ABI sceAudioInSetPortStatuses();
int PS4_SYSV_ABI sceAudioInSetSparkParam();
int PS4_SYSV_ABI sceAudioInSetSparkSideTone();
int PS4_SYSV_ABI sceAudioInSetUsbGain();
int PS4_SYSV_ABI sceAudioInSetUserMute();
int PS4_SYSV_ABI sceAudioInVmicCreate();
int PS4_SYSV_ABI sceAudioInVmicDestroy();
int PS4_SYSV_ABI sceAudioInVmicWrite();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AudioIn
