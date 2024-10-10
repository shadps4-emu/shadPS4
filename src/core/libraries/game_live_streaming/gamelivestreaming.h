// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::GameLiveStreaming {

struct OrbisGameLiveStreamingStatus {
    bool isOnAir;
    u8 align[3];
    u32 spectatorCounts;
    s32 userId;
    u8 reserved[60];
};
struct OrbisGameLiveStreamingStatus2 {
    s32 userId;
    bool isOnAir;
    u8 align[3];
    u32 spectatorCounts;
    u32 textMessageCounts;
    u32 commandMessageCounts;
    u32 broadcastVideoResolution;
    u8 reserved[48];
};

int PS4_SYSV_ABI sceGameLiveStreamingStartDebugBroadcast();
int PS4_SYSV_ABI sceGameLiveStreamingStopDebugBroadcast();
int PS4_SYSV_ABI sceGameLiveStreamingApplySocialFeedbackMessageFilter();
int PS4_SYSV_ABI sceGameLiveStreamingCheckCallback();
int PS4_SYSV_ABI sceGameLiveStreamingClearPresetSocialFeedbackCommands();
int PS4_SYSV_ABI sceGameLiveStreamingClearSocialFeedbackMessages();
int PS4_SYSV_ABI sceGameLiveStreamingClearSpoilerTag();
int PS4_SYSV_ABI sceGameLiveStreamingEnableLiveStreaming();
int PS4_SYSV_ABI sceGameLiveStreamingEnableSocialFeedback();
int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentBroadcastScreenLayout();
int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentStatus(OrbisGameLiveStreamingStatus* status);
int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentStatus2();
int PS4_SYSV_ABI sceGameLiveStreamingGetProgramInfo();
int PS4_SYSV_ABI sceGameLiveStreamingGetSocialFeedbackMessages();
int PS4_SYSV_ABI sceGameLiveStreamingGetSocialFeedbackMessagesCount();
int PS4_SYSV_ABI sceGameLiveStreamingInitialize();
int PS4_SYSV_ABI sceGameLiveStreamingLaunchLiveViewer();
int PS4_SYSV_ABI sceGameLiveStreamingLaunchLiveViewerA();
int PS4_SYSV_ABI sceGameLiveStreamingPermitLiveStreaming();
int PS4_SYSV_ABI sceGameLiveStreamingPermitServerSideRecording();
int PS4_SYSV_ABI sceGameLiveStreamingPostSocialMessage();
int PS4_SYSV_ABI sceGameLiveStreamingRegisterCallback();
int PS4_SYSV_ABI sceGameLiveStreamingScreenCloseSeparateMode();
int PS4_SYSV_ABI sceGameLiveStreamingScreenConfigureSeparateMode();
int PS4_SYSV_ABI sceGameLiveStreamingScreenInitialize();
int PS4_SYSV_ABI sceGameLiveStreamingScreenInitializeSeparateModeParameter();
int PS4_SYSV_ABI sceGameLiveStreamingScreenOpenSeparateMode();
int PS4_SYSV_ABI sceGameLiveStreamingScreenSetMode();
int PS4_SYSV_ABI sceGameLiveStreamingScreenTerminate();
int PS4_SYSV_ABI sceGameLiveStreamingSetCameraFrameSetting();
int PS4_SYSV_ABI sceGameLiveStreamingSetDefaultServiceProviderPermission();
int PS4_SYSV_ABI sceGameLiveStreamingSetGuardAreas();
int PS4_SYSV_ABI sceGameLiveStreamingSetInvitationSessionId();
int PS4_SYSV_ABI sceGameLiveStreamingSetLinkCommentPreset();
int PS4_SYSV_ABI sceGameLiveStreamingSetMaxBitrate();
int PS4_SYSV_ABI sceGameLiveStreamingSetMetadata();
int PS4_SYSV_ABI sceGameLiveStreamingSetPresetSocialFeedbackCommands();
int PS4_SYSV_ABI sceGameLiveStreamingSetPresetSocialFeedbackCommandsDescription();
int PS4_SYSV_ABI sceGameLiveStreamingSetServiceProviderPermission();
int PS4_SYSV_ABI sceGameLiveStreamingSetSpoilerTag();
int PS4_SYSV_ABI sceGameLiveStreamingSetStandbyScreenResource();
int PS4_SYSV_ABI sceGameLiveStreamingStartGenerateStandbyScreenResource();
int PS4_SYSV_ABI sceGameLiveStreamingStartSocialFeedbackMessageFiltering();
int PS4_SYSV_ABI sceGameLiveStreamingStopGenerateStandbyScreenResource();
int PS4_SYSV_ABI sceGameLiveStreamingStopSocialFeedbackMessageFiltering();
int PS4_SYSV_ABI sceGameLiveStreamingTerminate();
int PS4_SYSV_ABI sceGameLiveStreamingUnregisterCallback();

void RegisterlibSceGameLiveStreaming(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::GameLiveStreaming