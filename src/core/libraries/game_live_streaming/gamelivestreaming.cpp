// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gamelivestreaming.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::GameLiveStreaming {

int PS4_SYSV_ABI sceGameLiveStreamingStartDebugBroadcast() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingStopDebugBroadcast() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingApplySocialFeedbackMessageFilter() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingCheckCallback() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingClearPresetSocialFeedbackCommands() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingClearSocialFeedbackMessages() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingClearSpoilerTag() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingEnableLiveStreaming() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingEnableSocialFeedback() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentBroadcastScreenLayout() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentStatus(OrbisGameLiveStreamingStatus* status) {
    memset(status, 0, sizeof(*status));
    status->isOnAir = false;
    LOG_DEBUG(Lib_GameLiveStreaming, "(STUBBED) called userid = {}", status->userId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetCurrentStatus2() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetProgramInfo() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetSocialFeedbackMessages() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingGetSocialFeedbackMessagesCount() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingInitialize() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingLaunchLiveViewer() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingLaunchLiveViewerA() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingPermitLiveStreaming() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingPermitServerSideRecording() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingPostSocialMessage() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingRegisterCallback() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenCloseSeparateMode() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenConfigureSeparateMode() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenInitialize() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenInitializeSeparateModeParameter() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenOpenSeparateMode() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenSetMode() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingScreenTerminate() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetCameraFrameSetting() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetDefaultServiceProviderPermission() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetGuardAreas() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetInvitationSessionId() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetLinkCommentPreset() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetMaxBitrate() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetMetadata() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetPresetSocialFeedbackCommands() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetPresetSocialFeedbackCommandsDescription() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetServiceProviderPermission() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetSpoilerTag() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingSetStandbyScreenResource() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingStartGenerateStandbyScreenResource() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingStartSocialFeedbackMessageFiltering() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingStopGenerateStandbyScreenResource() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingStopSocialFeedbackMessageFiltering() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingTerminate() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGameLiveStreamingUnregisterCallback() {
    LOG_ERROR(Lib_GameLiveStreaming, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("caqgDl+V9qA", "libSceGameLiveStreaming_debug", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStartDebugBroadcast);
    LIB_FUNCTION("0i8Lrllxwow", "libSceGameLiveStreaming_debug", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStopDebugBroadcast);
    LIB_FUNCTION("NqkTzemliC0", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingApplySocialFeedbackMessageFilter);
    LIB_FUNCTION("PC4jq87+YQI", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingCheckCallback);
    LIB_FUNCTION("FcHBfHjFXkA", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingClearPresetSocialFeedbackCommands);
    LIB_FUNCTION("lZ2Sd0uEvpo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingClearSocialFeedbackMessages);
    LIB_FUNCTION("6c2zGtThFww", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingClearSpoilerTag);
    LIB_FUNCTION("dWM80AX39o4", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingEnableLiveStreaming);
    LIB_FUNCTION("wBOQWjbWMfU", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingEnableSocialFeedback);
    LIB_FUNCTION("aRSQNqbats4", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetCurrentBroadcastScreenLayout);
    LIB_FUNCTION("CoPMx369EqM", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetCurrentStatus);
    LIB_FUNCTION("lK8dLBNp9OE", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetCurrentStatus2);
    LIB_FUNCTION("OIIm19xu+NM", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetProgramInfo);
    LIB_FUNCTION("PMx7N4WqNdo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetSocialFeedbackMessages);
    LIB_FUNCTION("yeQKjHETi40", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetSocialFeedbackMessagesCount);
    LIB_FUNCTION("kvYEw2lBndk", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingInitialize);
    LIB_FUNCTION("ysWfX5PPbfc", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingLaunchLiveViewer);
    LIB_FUNCTION("cvRCb7DTAig", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingLaunchLiveViewerA);
    LIB_FUNCTION("K0QxEbD7q+c", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingPermitLiveStreaming);
    LIB_FUNCTION("-EHnU68gExU", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingPermitServerSideRecording);
    LIB_FUNCTION("hggKhPySVgI", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingPostSocialMessage);
    LIB_FUNCTION("nFP8qT9YXbo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingRegisterCallback);
    LIB_FUNCTION("b5RaMD2J0So", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenCloseSeparateMode);
    LIB_FUNCTION("hBdd8n6kuvE", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenConfigureSeparateMode);
    LIB_FUNCTION("uhCmn81s-mU", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenInitialize);
    LIB_FUNCTION("fo5B8RUaBxQ", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenInitializeSeparateModeParameter);
    LIB_FUNCTION("iorzW0pKOiA", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenOpenSeparateMode);
    LIB_FUNCTION("gDSvt78H3Oo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenSetMode);
    LIB_FUNCTION("HE93dr-5rx4", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingScreenTerminate);
    LIB_FUNCTION("3PSiwAzFISE", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetCameraFrameSetting);
    LIB_FUNCTION("TwuUzTKKeek", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetDefaultServiceProviderPermission);
    LIB_FUNCTION("Gw6S4oqlY7E", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetGuardAreas);
    LIB_FUNCTION("QmQYwQ7OTJI", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetInvitationSessionId);
    LIB_FUNCTION("Sb5bAXyUt5c", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetLinkCommentPreset);
    LIB_FUNCTION("q-kxuaF7URU", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetMaxBitrate);
    LIB_FUNCTION("hUY-mSOyGL0", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetMetadata);
    LIB_FUNCTION("ycodiP2I0xo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetPresetSocialFeedbackCommands);
    LIB_FUNCTION("x6deXUpQbBo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetPresetSocialFeedbackCommandsDescription);
    LIB_FUNCTION("mCoz3k3zPmA", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetServiceProviderPermission);
    LIB_FUNCTION("ZuX+zzz2DkA", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetSpoilerTag);
    LIB_FUNCTION("MLvYI86FFAo", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingSetStandbyScreenResource);
    LIB_FUNCTION("y0KkAydy9xE", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStartGenerateStandbyScreenResource);
    LIB_FUNCTION("Y1WxX7dPMCw", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStartSocialFeedbackMessageFiltering);
    LIB_FUNCTION("D7dg5QJ4FlE", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStopGenerateStandbyScreenResource);
    LIB_FUNCTION("bYuGUBuIsaY", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingStopSocialFeedbackMessageFiltering);
    LIB_FUNCTION("9yK6Fk8mKOQ", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingTerminate);
    LIB_FUNCTION("5XHaH3kL+bA", "libSceGameLiveStreaming", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingUnregisterCallback);
    LIB_FUNCTION("caqgDl+V9qA", "libSceGameLiveStreaming_direct_streaming", 1,
                 "libSceGameLiveStreaming", sceGameLiveStreamingStartDebugBroadcast);
    LIB_FUNCTION("0i8Lrllxwow", "libSceGameLiveStreaming_direct_streaming", 1,
                 "libSceGameLiveStreaming", sceGameLiveStreamingStopDebugBroadcast);
    LIB_FUNCTION("CoPMx369EqM", "libSceGameLiveStreamingCompat", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingGetCurrentStatus);
    LIB_FUNCTION("ysWfX5PPbfc", "libSceGameLiveStreamingCompat", 1, "libSceGameLiveStreaming",
                 sceGameLiveStreamingLaunchLiveViewer);
};

} // namespace Libraries::GameLiveStreaming