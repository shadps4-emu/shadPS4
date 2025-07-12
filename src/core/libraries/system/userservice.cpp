// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"

#include "core/libraries/libs.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/system/userservice_error.h"

namespace Libraries::UserService {

int PS4_SYSV_ABI sceUserServiceInitializeForShellCore() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceTerminateForShellCore() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceDestroyUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityKeyremapData() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityKeyremapEnable() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityPressAndHoldDelay() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityVibration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccessibilityZoomEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAccountRemarks() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAgeVerified() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAppearOfflineSetting() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAppSortOrder() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetAutoLoginEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetCreatedVersion() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetCurrentUserGroupIndex() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetDefaultNewUserGroupName() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetDeletedUserInfo() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetDiscPlayerFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetEvent(OrbisUserServiceEvent* event) {
    LOG_TRACE(Lib_UserService, "(DUMMY) called");
    // fake a loggin event
    static bool logged_in = false;

    if (!logged_in) {
        logged_in = true;
        event->event = OrbisUserServiceEventType::Login;
        event->userId = 1;
        return ORBIS_OK;
    }

    return ORBIS_USER_SERVICE_ERROR_NO_EVENT;
}

int PS4_SYSV_ABI sceUserServiceGetEventCalendarType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEventFilterTeamEvent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEventSortEvent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEventSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEventUiFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEventVsh() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFaceRecognitionDeleteCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFaceRecognitionRegisterCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileBrowserFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileBrowserSortContent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileBrowserSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileSelectorFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileSelectorSortContent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFileSelectorSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetForegroundUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFriendCustomListLastFocus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetFriendFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsAccessTokenNiconicoLive() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsAccessTokenTwitch() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsAccessTokenUstream() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsAnonymousUserId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBcTags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBcTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBroadcastChannel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBroadcastersComment() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBroadcastersCommentColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBroadcastService() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsBroadcastUiLayout() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCamCrop() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraBgFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraChromaKeyLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraContrast() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraDepthLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraEdgeLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraEffect() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraEliminationLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraPosition() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraReflection() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraSize() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCameraTransparency() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsCommunityId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsFloatingMessage() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsHintFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsInitSpectating() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsCameraHidden() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsFacebookEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsMuteEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsRecDisabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsRecievedMessageHidden() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsIsTwitterEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLanguageFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLfpsSortOrder() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLiveQuality() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLiveQuality2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLiveQuality3() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLiveQuality4() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsLiveQuality5() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsMessageFilterLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsTtsFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsTtsPitch() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsTtsSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetGlsTtsVolume() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetHmuBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetHmuZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetHoldAudioOutDevice() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetHomeDirectory() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImeAutoCapitalEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImeInitFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImeInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImeLastUnit() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImePointerMode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImePredictiveTextEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetImeRunCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetInitialUser(int* user_id) {
    LOG_DEBUG(Lib_UserService, "called");
    if (user_id == nullptr) {
        LOG_ERROR(Lib_UserService, "user_id is null");
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    // select first user (TODO add more)
    *user_id = 1;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetIPDLeft() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetIPDRight() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetIsFakePlus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetIsQuickSignup() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetIsRemotePlayAllowed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetJapaneseInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetKeyboardType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetKeyRepeatSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetKeyRepeatStartingTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetKratosPrimaryUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetLastLoginOrder() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetLightBarBaseBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetLoginFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(OrbisUserServiceLoginUserIdList* userIdList) {
    LOG_DEBUG(Lib_UserService, "called");
    if (userIdList == nullptr) {
        LOG_ERROR(Lib_UserService, "user_id is null");
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    // TODO only first user, do the others as well
    userIdList->user_id[0] = 1;
    userIdList->user_id[1] = ORBIS_USER_SERVICE_USER_ID_INVALID;
    userIdList->user_id[2] = ORBIS_USER_SERVICE_USER_ID_INVALID;
    userIdList->user_id[3] = ORBIS_USER_SERVICE_USER_ID_INVALID;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetMicLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetMouseHandType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetMousePointerSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNotificationBehavior() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNotificationSettings() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpAccountUpgradeFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpAge() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpAuthErrorFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpCountryCode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpDateOfBirth() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpEnv() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpLanguageCode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpLanguageCode2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpLoginId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpMAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpNpId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpOfflineAccountAdult() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpOfflineAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpOnlineId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetNpSubAccount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPadSpeakerVolume() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalBdAge() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalBrowser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalDvd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalDvdRegion() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalGame() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalGameAgeLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetParentalMorpheus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPartyMuteList() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPartyMuteListA() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPartySettingFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPasscode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcAdditionalTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcFridayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcFridayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcFridayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcMode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcMondayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcMondayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcMondayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcPlayTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcPlayTimeLastUpdated() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSaturdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSaturdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSaturdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSundayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSundayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcSundayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcThursdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcThursdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcThursdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcTuesdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcTuesdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcTuesdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcTzOffset() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcWednesdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcWednesdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPbtcWednesdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPlayTogetherFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetPsnPasswordForDebug() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetRegisteredHomeUserIdList() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetRegisteredUserIdList() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSaveDataAutoUpload() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSaveDataSort() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSaveDataTutorialFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSecureHomeDirectory() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetShareButtonAssign() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetShareDailymotionAccessToken() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetShareDailymotionRefreshToken() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSharePlayFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSharePlayFramerateHost() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSharePlayResolutionHost() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetShareStatus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetShareStatus2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSystemLoggerHashedAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSystemLoggerHashedAccountIdClockType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSystemLoggerHashedAccountIdParam() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetSystemLoggerHashedAccountIdTtl() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetTeamShowAboutTeam() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeBgImageDimmer() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeBgImageWaveColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeBgImageZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeEntitlementId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeHomeShareOwner() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeTextShadow() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetThemeWaveColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetTopMenuLimitItem() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetTopMenuNotificationFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetTopMenuTutorialFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetTraditionalChineseInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetUserColor(int user_id, OrbisUserServiceUserColor* color) {
    // TODO fix me better
    LOG_DEBUG(Lib_UserService, "called user_id = {}", user_id);
    if (color == nullptr) {
        LOG_ERROR(Lib_UserService, "color is null");
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    *color = OrbisUserServiceUserColor::Blue;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetUserGroupName() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetUserGroupNameList() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetUserGroupNum() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetUserName(int user_id, char* user_name, std::size_t size) {
    LOG_DEBUG(Lib_UserService, "called user_id = {} ,size = {} ", user_id, size);
    if (user_name == nullptr) {
        LOG_ERROR(Lib_UserService, "user_name is null");
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    std::string name = Config::getUserName();
    if (size < name.length()) {
        LOG_ERROR(Lib_UserService, "buffer is too short");
        return ORBIS_USER_SERVICE_ERROR_BUFFER_TOO_SHORT;
    }
    snprintf(user_name, size, "%s", name.c_str());
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetUserStatus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVibrationEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVoiceRecognitionLastUsedOsk() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVoiceRecognitionTutorialState() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVolumeForController() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVolumeForGenericUSB() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVolumeForMorpheusSidetone() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceGetVolumeForSidetone() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUserServiceInitialize(const OrbisUserServiceInitializeParams* initParams) {
    LOG_WARNING(Lib_UserService, "(dummy) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceInitialize2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsGuestUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsKratosPrimaryUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsKratosUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsLoggedIn() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsLoggedInWithoutLock() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsSharePlayClientUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceIsUserStorageAccountBound() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceLogin() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceLogout() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceRegisterEventCallback() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAccessibilityKeyremapData() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAccessibilityKeyremapEnable() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAccessibilityZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAccountRemarks() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAgeVerified() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAppearOfflineSetting() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAppSortOrder() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetAutoLoginEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetCreatedVersion() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetDiscPlayerFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetEventCalendarType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetEventFilterTeamEvent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetEventSortEvent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetEventSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetEventUiFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFaceRecognitionDeleteCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFaceRecognitionRegisterCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileBrowserFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileBrowserSortContent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileBrowserSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileSelectorFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileSelectorSortContent() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFileSelectorSortTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetForegroundUser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFriendCustomListLastFocus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetFriendFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsAccessTokenNiconicoLive() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsAccessTokenTwitch() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsAccessTokenUstream() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsAnonymousUserId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBcTags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBcTitle() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBroadcastChannel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBroadcastersComment() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBroadcastersCommentColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBroadcastService() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsBroadcastUiLayout() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCamCrop() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraBgFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraChromaKeyLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraContrast() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraDepthLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraEdgeLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraEffect() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraEliminationLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraPosition() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraReflection() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraSize() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCameraTransparency() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsCommunityId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsFloatingMessage() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsHintFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsInitSpectating() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsCameraHidden() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsFacebookEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsMuteEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsRecDisabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsRecievedMessageHidden() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsIsTwitterEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLanguageFilter() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLfpsSortOrder() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLiveQuality() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLiveQuality2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLiveQuality3() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLiveQuality4() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsLiveQuality5() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsMessageFilterLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsTtsFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsTtsPitch() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsTtsSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetGlsTtsVolume() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetHmuBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetHmuZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetHoldAudioOutDevice() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImeAutoCapitalEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImeInitFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImeInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImeLastUnit() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImePointerMode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImePredictiveTextEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetImeRunCount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetIPDLeft() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetIPDRight() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetIsFakePlus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetIsQuickSignup() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetIsRemotePlayAllowed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetJapaneseInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetKeyboardType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetKeyRepeatSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetKeyRepeatStartingTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetLightBarBaseBrightness() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetLoginFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetMicLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetMouseHandType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetMousePointerSpeed() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNotificationBehavior() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNotificationSettings() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpAccountUpgradeFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpAge() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpAuthErrorFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpCountryCode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpDateOfBirth() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpEnv() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpLanguageCode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpLanguageCode2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpLoginId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpMAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpNpId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpOfflineAccountAdult() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpOnlineId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetNpSubAccount() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPadSpeakerVolume() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalBdAge() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalBrowser() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalDvd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalDvdRegion() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalGame() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalGameAgeLevel() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetParentalMorpheus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPartyMuteList() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPartyMuteListA() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPartySettingFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPasscode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcAdditionalTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcFridayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcFridayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcFridayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcMode() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcMondayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcMondayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcMondayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcPlayTime() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcPlayTimeLastUpdated() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSaturdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSaturdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSaturdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSundayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSundayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcSundayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcThursdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcThursdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcThursdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcTuesdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcTuesdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcTuesdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcTzOffset() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcWednesdayDuration() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcWednesdayHoursEnd() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPbtcWednesdayHoursStart() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPlayTogetherFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetPsnPasswordForDebug() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSaveDataAutoUpload() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSaveDataSort() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSaveDataTutorialFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetShareButtonAssign() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetShareDailymotionAccessToken() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetShareDailymotionRefreshToken() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSharePlayFlags() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSharePlayFramerateHost() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSharePlayResolutionHost() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetShareStatus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetShareStatus2() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSystemLoggerHashedAccountId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSystemLoggerHashedAccountIdClockType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSystemLoggerHashedAccountIdParam() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetSystemLoggerHashedAccountIdTtl() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetTeamShowAboutTeam() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeBgImageDimmer() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeBgImageWaveColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeBgImageZoom() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeEntitlementId() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeHomeShareOwner() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeTextShadow() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetThemeWaveColor() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetTopMenuLimitItem() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetTopMenuNotificationFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetTopMenuTutorialFlag() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetTraditionalChineseInputType() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetUserGroupIndex() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetUserGroupName() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetUserName() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetUserStatus() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVibrationEnabled() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVoiceRecognitionLastUsedOsk() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVoiceRecognitionTutorialState() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVolumeForController() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVolumeForGenericUSB() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVolumeForMorpheusSidetone() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceSetVolumeForSidetone() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceTerminate() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUserServiceUnregisterEventCallback() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8AC6DC4168D5FEA5() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A6BDC9DFDAFD02B4() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BB9491DFE6B4953C() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D2B814603E7B4477() {
    LOG_ERROR(Lib_UserService, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Psl9mfs3duM", "libSceUserServiceForShellCore", 1, "libSceUserService", 1, 1,
                 sceUserServiceInitializeForShellCore);
    LIB_FUNCTION("CydP+QtA0KI", "libSceUserServiceForShellCore", 1, "libSceUserService", 1, 1,
                 sceUserServiceTerminateForShellCore);
    LIB_FUNCTION("GC18r56Bp7Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceDestroyUser);
    LIB_FUNCTION("g6ojqW3c8Z4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityKeyremapData);
    LIB_FUNCTION("xrtki9sUopg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityKeyremapEnable);
    LIB_FUNCTION("ZKJtxdgvzwg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityPressAndHoldDelay);
    LIB_FUNCTION("qWYHOFwqCxY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityVibration);
    LIB_FUNCTION("1zDEFUmBdoo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityZoom);
    LIB_FUNCTION("hD-H81EN9Vg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccessibilityZoomEnabled);
    LIB_FUNCTION("7zu3F7ykVeo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAccountRemarks);
    LIB_FUNCTION("oJzfZxZchX4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAgeVerified);
    LIB_FUNCTION("6r4hDyrRUGg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAppearOfflineSetting);
    LIB_FUNCTION("PhXZbj4wVhE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAppSortOrder);
    LIB_FUNCTION("nqDEnj7M0QE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetAutoLoginEnabled);
    LIB_FUNCTION("WGXOvoUwrOs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetCreatedVersion);
    LIB_FUNCTION("5G-MA1x5utw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetCurrentUserGroupIndex);
    LIB_FUNCTION("1U5cFdTdso0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetDefaultNewUserGroupName);
    LIB_FUNCTION("NiTGNLkBc-Q", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetDeletedUserInfo);
    LIB_FUNCTION("RdpmnHZ3Q9M", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetDiscPlayerFlag);
    LIB_FUNCTION("yH17Q6NWtVg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEvent);
    LIB_FUNCTION("zs60MvClEkc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventCalendarType);
    LIB_FUNCTION("TwELPoqW8tA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventFilterTeamEvent);
    LIB_FUNCTION("ygVuZ1Hb-nc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventSortEvent);
    LIB_FUNCTION("aaC3005VtY4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventSortTitle);
    LIB_FUNCTION("kUaJUV1b+PM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventUiFlag);
    LIB_FUNCTION("3wTtZ3c2+0A", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetEventVsh);
    LIB_FUNCTION("uRU0lQe+9xY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFaceRecognitionDeleteCount);
    LIB_FUNCTION("fbCC0yo2pVQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFaceRecognitionRegisterCount);
    LIB_FUNCTION("k-7kxXGr+r0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileBrowserFilter);
    LIB_FUNCTION("fCBpPJbELDk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileBrowserSortContent);
    LIB_FUNCTION("UYR9fcPXDUE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileBrowserSortTitle);
    LIB_FUNCTION("FsOBy3JfbrM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileSelectorFilter);
    LIB_FUNCTION("IAB7wscPwio", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileSelectorSortContent);
    LIB_FUNCTION("6Et3d4p1u8c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFileSelectorSortTitle);
    LIB_FUNCTION("eNb53LQJmIM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetForegroundUser);
    LIB_FUNCTION("eMGF77hKF6U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFriendCustomListLastFocus);
    LIB_FUNCTION("wBGmrRTUC14", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetFriendFlag);
    LIB_FUNCTION("64PEUYPuK98", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsAccessTokenNiconicoLive);
    LIB_FUNCTION("8Y+aDvVGLiw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsAccessTokenTwitch);
    LIB_FUNCTION("V7ZG7V+dd08", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsAccessTokenUstream);
    LIB_FUNCTION("QqZ1A3vukFM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsAnonymousUserId);
    LIB_FUNCTION("FP4TKrdRXXM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBcTags);
    LIB_FUNCTION("yX-TpbFAYxo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBcTitle);
    LIB_FUNCTION("Mm4+PSflHbM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBroadcastChannel);
    LIB_FUNCTION("NpEYVDOyjRk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBroadcastersComment);
    LIB_FUNCTION("WvM21J1SI0U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBroadcastersCommentColor);
    LIB_FUNCTION("HxNRiCWfVFw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBroadcastService);
    LIB_FUNCTION("6ZQ4kfhM37c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsBroadcastUiLayout);
    LIB_FUNCTION("YmmFiEoegko", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCamCrop);
    LIB_FUNCTION("Y5U66nk0bUc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraBgFilter);
    LIB_FUNCTION("LbQ-jU9jOsk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraBrightness);
    LIB_FUNCTION("91kOKRnkrhE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraChromaKeyLevel);
    LIB_FUNCTION("1ppzHkQhiNs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraContrast);
    LIB_FUNCTION("jIe8ZED06XI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraDepthLevel);
    LIB_FUNCTION("0H51EFxR3mc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraEdgeLevel);
    LIB_FUNCTION("rLEw4n5yI40", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraEffect);
    LIB_FUNCTION("+Prbx5iagl0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraEliminationLevel);
    LIB_FUNCTION("F0wuEvioQd4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraPosition);
    LIB_FUNCTION("GkcHilidQHk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraReflection);
    LIB_FUNCTION("zBLxX8JRMoo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraSize);
    LIB_FUNCTION("O1nURsxyYmk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCameraTransparency);
    LIB_FUNCTION("4TOEFdmFVcI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsCommunityId);
    LIB_FUNCTION("+29DSndZ9Oc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsFloatingMessage);
    LIB_FUNCTION("ki81gh1yZDM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsHintFlag);
    LIB_FUNCTION("zR+J2PPJgSU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsInitSpectating);
    LIB_FUNCTION("8IqdtMmc5Uc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsCameraHidden);
    LIB_FUNCTION("f5lAVp0sFNo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsFacebookEnabled);
    LIB_FUNCTION("W3neFYAvZss", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsMuteEnabled);
    LIB_FUNCTION("4IXuUaBxzEg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsRecDisabled);
    LIB_FUNCTION("hyW5w855fk4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsRecievedMessageHidden);
    LIB_FUNCTION("Xp9Px0V0tas", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsIsTwitterEnabled);
    LIB_FUNCTION("uMkqgm70thg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLanguageFilter);
    LIB_FUNCTION("LyXzCtzleAQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLfpsSortOrder);
    LIB_FUNCTION("CvwCMJtzp1I", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLiveQuality);
    LIB_FUNCTION("Z+dzNaClq7w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLiveQuality2);
    LIB_FUNCTION("X5On-7hVCs0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLiveQuality3);
    LIB_FUNCTION("+qAE4tRMrXk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLiveQuality4);
    LIB_FUNCTION("4ys00CRU6V8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsLiveQuality5);
    LIB_FUNCTION("75cwn1y2ffk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsMessageFilterLevel);
    LIB_FUNCTION("+NVJMeISrM4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsTtsFlags);
    LIB_FUNCTION("eQrBbMmZ1Ss", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsTtsPitch);
    LIB_FUNCTION("BCDA6jn4HVY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsTtsSpeed);
    LIB_FUNCTION("SBurFYk7M74", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetGlsTtsVolume);
    LIB_FUNCTION("YVzw4T1fnS4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetHmuBrightness);
    LIB_FUNCTION("O8ONJV3b8jg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetHmuZoom);
    LIB_FUNCTION("VjLkKY0CQew", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetHoldAudioOutDevice);
    LIB_FUNCTION("J-KEr4gUEvQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetHomeDirectory);
    LIB_FUNCTION("yLNm3n7fgpw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImeAutoCapitalEnabled);
    LIB_FUNCTION("gnViUj0ab8U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImeInitFlag);
    LIB_FUNCTION("zru8Zhuy1UY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImeInputType);
    LIB_FUNCTION("2-b8QbU+HNc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImeLastUnit);
    LIB_FUNCTION("NNblpSGxrY8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImePointerMode);
    LIB_FUNCTION("YUhBM-ASEcA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImePredictiveTextEnabled);
    LIB_FUNCTION("IWEla-izyTs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetImeRunCount);
    LIB_FUNCTION("CdWp0oHWGr0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetInitialUser);
    LIB_FUNCTION("PQlF4cjUz9U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetIPDLeft);
    LIB_FUNCTION("UDx67PTzB20", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetIPDRight);
    LIB_FUNCTION("IKk3EGj+xRI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetIsFakePlus);
    LIB_FUNCTION("MzVmbq2IVCo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetIsQuickSignup);
    LIB_FUNCTION("Lgi5A4fQwHc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetIsRemotePlayAllowed);
    LIB_FUNCTION("u-dCVE6fQAU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetJapaneseInputType);
    LIB_FUNCTION("Ta52bXx5Tek", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetKeyboardType);
    LIB_FUNCTION("XUT7ad-BUMc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetKeyRepeatSpeed);
    LIB_FUNCTION("iWpzXixD0UE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetKeyRepeatStartingTime);
    LIB_FUNCTION("uAPBw-7641s", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetKratosPrimaryUser);
    LIB_FUNCTION("4nUbGGBcGco", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetLastLoginOrder);
    LIB_FUNCTION("q+7UTGELzj4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetLightBarBaseBrightness);
    LIB_FUNCTION("QNk7qD4dlD4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetLoginFlag);
    LIB_FUNCTION("fPhymKNvK-A", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetLoginUserIdList);
    LIB_FUNCTION("YfDgKz5SolU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetMicLevel);
    LIB_FUNCTION("sukPd-xBDjM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetMouseHandType);
    LIB_FUNCTION("Y5zgw69ndoE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetMousePointerSpeed);
    LIB_FUNCTION("3oqgIFPVkV8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNotificationBehavior);
    LIB_FUNCTION("5iqtUryI-hI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNotificationSettings);
    LIB_FUNCTION("6dfDreosXGY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpAccountId);
    LIB_FUNCTION("Veo1PbQZzG4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpAccountUpgradeFlag);
    LIB_FUNCTION("OySMIASmH0Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpAge);
    LIB_FUNCTION("nlOWAiRyxkA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpAuthErrorFlag);
    LIB_FUNCTION("8vhI2SwEfes", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpCountryCode);
    LIB_FUNCTION("YyC7QCLoSxY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpDateOfBirth);
    LIB_FUNCTION("-YcNkLzNGmY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpEnv);
    LIB_FUNCTION("J4ten1IOe5w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpLanguageCode);
    LIB_FUNCTION("ruF+U6DexT4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpLanguageCode2);
    LIB_FUNCTION("W5RgPUuv35Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpLoginId);
    LIB_FUNCTION("j-CnRJn3K+Q", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpMAccountId);
    LIB_FUNCTION("5Ds-y6A1nAI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpNpId);
    LIB_FUNCTION("auc64RJAcus", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpOfflineAccountAdult);
    LIB_FUNCTION("fEy0EW0AR18", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpOfflineAccountId);
    LIB_FUNCTION("if-BeWwY0aU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpOnlineId);
    LIB_FUNCTION("wCGnkXhpRL4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetNpSubAccount);
    LIB_FUNCTION("zNvCnHpkPmM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPadSpeakerVolume);
    LIB_FUNCTION("lXKtAHMrwig", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalBdAge);
    LIB_FUNCTION("t04S4aC0LCM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalBrowser);
    LIB_FUNCTION("5vtFYXFJ7OU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalDvd);
    LIB_FUNCTION("d9DOmIk9-y4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalDvdRegion);
    LIB_FUNCTION("OdiXSuoIK7c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalGame);
    LIB_FUNCTION("oXARzvLAiyc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalGameAgeLevel);
    LIB_FUNCTION("yXvfR+AcgaY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetParentalMorpheus);
    LIB_FUNCTION("UeIv6aNXlOw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPartyMuteList);
    LIB_FUNCTION("aq1jwlgyOV4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPartyMuteListA);
    LIB_FUNCTION("yARnQeWzhdM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPartySettingFlags);
    LIB_FUNCTION("X5rJZNDZ2Ss", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPasscode);
    LIB_FUNCTION("m1h-E6BU6CA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcAdditionalTime);
    LIB_FUNCTION("HsOlaoGngDc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcFlag);
    LIB_FUNCTION("3DuTkVXaj9Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcFridayDuration);
    LIB_FUNCTION("5dM-i0Ox2d8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcFridayHoursEnd);
    LIB_FUNCTION("vcd5Kfs1QeA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcFridayHoursStart);
    LIB_FUNCTION("Q5Um9Yri-VA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcMode);
    LIB_FUNCTION("NnvYm9PFJiw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcMondayDuration);
    LIB_FUNCTION("42K0F17ml9c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcMondayHoursEnd);
    LIB_FUNCTION("WunW7G5bHYo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcMondayHoursStart);
    LIB_FUNCTION("JrFGcFUL0lg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcPlayTime);
    LIB_FUNCTION("R6ldE-2ON1w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcPlayTimeLastUpdated);
    LIB_FUNCTION("DembpCGx9DU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSaturdayDuration);
    LIB_FUNCTION("Cf8NftzheE4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSaturdayHoursEnd);
    LIB_FUNCTION("+1qj-S-k6m0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSaturdayHoursStart);
    LIB_FUNCTION("JVMIyR8vDec", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSundayDuration);
    LIB_FUNCTION("J+bKHRzY4nw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSundayHoursEnd);
    LIB_FUNCTION("J+cECJ7CBFM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcSundayHoursStart);
    LIB_FUNCTION("z-hJNdfLRN0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcThursdayDuration);
    LIB_FUNCTION("BkOBCo0sdLM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcThursdayHoursEnd);
    LIB_FUNCTION("T70Qyzo51uw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcThursdayHoursStart);
    LIB_FUNCTION("UPDgXiV1Zp0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcTuesdayDuration);
    LIB_FUNCTION("Kpds+6CpTus", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcTuesdayHoursEnd);
    LIB_FUNCTION("azCh0Ibz8ls", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcTuesdayHoursStart);
    LIB_FUNCTION("NjEMsEjXlTY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcTzOffset);
    LIB_FUNCTION("VwF4r--aouQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcWednesdayDuration);
    LIB_FUNCTION("nxGZSi5FEwc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcWednesdayHoursEnd);
    LIB_FUNCTION("7Wes8MVwuoM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPbtcWednesdayHoursStart);
    LIB_FUNCTION("yAWUqugjPvE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPlayTogetherFlags);
    LIB_FUNCTION("VSQR9qYpaCM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetPsnPasswordForDebug);
    LIB_FUNCTION("OVdVBcejvmQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetRegisteredHomeUserIdList);
    LIB_FUNCTION("5EiQCnL2G1Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetRegisteredUserIdList);
    LIB_FUNCTION("UxrSdH6jA3E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSaveDataAutoUpload);
    LIB_FUNCTION("pVsEKLk5bIA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSaveDataSort);
    LIB_FUNCTION("88+nqBN-SQM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSaveDataTutorialFlag);
    LIB_FUNCTION("xzQVBcKYoI8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSecureHomeDirectory);
    LIB_FUNCTION("zsJcWtE81Rk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetShareButtonAssign);
    LIB_FUNCTION("NjhK36GfEGQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetShareDailymotionAccessToken);
    LIB_FUNCTION("t-I2Lbj8a+0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetShareDailymotionRefreshToken);
    LIB_FUNCTION("lrPF-kNBPro", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSharePlayFlags);
    LIB_FUNCTION("eC88db1i-f8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSharePlayFramerateHost);
    LIB_FUNCTION("ttiSviAPLXI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSharePlayResolutionHost);
    LIB_FUNCTION("YnXM2saZkl4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetShareStatus);
    LIB_FUNCTION("wMtSHLNAVj0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetShareStatus2);
    LIB_FUNCTION("8no2rlDjl7o", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSystemLoggerHashedAccountId);
    LIB_FUNCTION("vW2qWKYmlvw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSystemLoggerHashedAccountIdClockType);
    LIB_FUNCTION("Zr4h+Bbx0do", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSystemLoggerHashedAccountIdParam);
    LIB_FUNCTION("cf9BIMy4muY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetSystemLoggerHashedAccountIdTtl);
    LIB_FUNCTION("AGDKupLjTZM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetTeamShowAboutTeam);
    LIB_FUNCTION("EZJecX+WvJc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeBgImageDimmer);
    LIB_FUNCTION("POVfvCDcVUw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeBgImageWaveColor);
    LIB_FUNCTION("qI2HG1pV+OA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeBgImageZoom);
    LIB_FUNCTION("x6m8P9DBPSc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeEntitlementId);
    LIB_FUNCTION("K8Nh6fhmYkc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeHomeShareOwner);
    LIB_FUNCTION("EgEPXDie5XQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeTextShadow);
    LIB_FUNCTION("WaHZGp0Vn2k", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetThemeWaveColor);
    LIB_FUNCTION("IxCpDYsiTX0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetTopMenuLimitItem);
    LIB_FUNCTION("SykFcJEGvz4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetTopMenuNotificationFlag);
    LIB_FUNCTION("MG+ObGDYePw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetTopMenuTutorialFlag);
    LIB_FUNCTION("oXVAQutr3Ns", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetTraditionalChineseInputType);
    LIB_FUNCTION("lUoqwTQu4Go", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserColor);
    LIB_FUNCTION("1+nxJ4awLH8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserGroupName);
    LIB_FUNCTION("ga2z3AAn8XI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserGroupNameList);
    LIB_FUNCTION("xzdhJrL3Hns", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserGroupNum);
    LIB_FUNCTION("1xxcMiGu2fo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserName);
    LIB_FUNCTION("RJX7T4sjNgI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetUserStatus);
    LIB_FUNCTION("O0mtfoE5Cek", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVibrationEnabled);
    LIB_FUNCTION("T4L2vVa0zuA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVoiceRecognitionLastUsedOsk);
    LIB_FUNCTION("-jRGLt2Dbe4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVoiceRecognitionTutorialState);
    LIB_FUNCTION("ld396XJQPgM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVolumeForController);
    LIB_FUNCTION("TEsQ0HWJ8R4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVolumeForGenericUSB);
    LIB_FUNCTION("r2QuHIT8u9I", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVolumeForMorpheusSidetone);
    LIB_FUNCTION("3UZADLBXpiA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetVolumeForSidetone);
    LIB_FUNCTION("j3YMu1MVNNo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceInitialize);
    LIB_FUNCTION("az-0R6eviZ0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceInitialize2);
    LIB_FUNCTION("FnWkLNOmJXw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsGuestUser);
    LIB_FUNCTION("mNnB2PWMSgw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsKratosPrimaryUser);
    LIB_FUNCTION("pZL154KvMjU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsKratosUser);
    LIB_FUNCTION("MZxH8029+Wg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsLoggedIn);
    LIB_FUNCTION("hTdcWcUUcrk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsLoggedInWithoutLock);
    LIB_FUNCTION("-7XgCmEwKrs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsSharePlayClientUser);
    LIB_FUNCTION("TLrDgrPYTDo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceIsUserStorageAccountBound);
    LIB_FUNCTION("uvVR70ZxFrQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceLogin);
    LIB_FUNCTION("3T9y5xDcfOk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceLogout);
    LIB_FUNCTION("wuI7c7UNk0A", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceRegisterEventCallback);
    LIB_FUNCTION("SfGVfyEN8iw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAccessibilityKeyremapData);
    LIB_FUNCTION("ZP0ti1CRxNA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAccessibilityKeyremapEnable);
    LIB_FUNCTION("HKu68cVzctg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAccessibilityZoom);
    LIB_FUNCTION("vC-uSETCFUY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAccountRemarks);
    LIB_FUNCTION("gBLMGhB6B9E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAgeVerified);
    LIB_FUNCTION("7IiUdURpH0k", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAppearOfflineSetting);
    LIB_FUNCTION("b5-tnLcyUQE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAppSortOrder);
    LIB_FUNCTION("u-E+6d9PiP8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetAutoLoginEnabled);
    LIB_FUNCTION("feqktbQD1eo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetCreatedVersion);
    LIB_FUNCTION("m8VtSd5I5og", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetDiscPlayerFlag);
    LIB_FUNCTION("wV3jlvsT5jA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetEventCalendarType);
    LIB_FUNCTION("rez819wV7AU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetEventFilterTeamEvent);
    LIB_FUNCTION("uhwssTtt3yo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetEventSortEvent);
    LIB_FUNCTION("XEgdhGfqRpI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetEventSortTitle);
    LIB_FUNCTION("Ty9wanVDC9k", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetEventUiFlag);
    LIB_FUNCTION("snOzH0NQyO0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFaceRecognitionDeleteCount);
    LIB_FUNCTION("jiMNYgxzT-4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFaceRecognitionRegisterCount);
    LIB_FUNCTION("M9noOXMhlGo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileBrowserFilter);
    LIB_FUNCTION("Xy4rq8gpYHU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileBrowserSortContent);
    LIB_FUNCTION("wN5zRLw4J6A", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileBrowserSortTitle);
    LIB_FUNCTION("hP2q9Eb5hf0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileSelectorFilter);
    LIB_FUNCTION("Fl52JeSLPyw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileSelectorSortContent);
    LIB_FUNCTION("Llv693Nx+nU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFileSelectorSortTitle);
    LIB_FUNCTION("MgBIXUkGtpE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetForegroundUser);
    LIB_FUNCTION("fK4AIM0knFQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFriendCustomListLastFocus);
    LIB_FUNCTION("5cK+UC54Oz4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetFriendFlag);
    LIB_FUNCTION("VEUKQumI5B8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsAccessTokenNiconicoLive);
    LIB_FUNCTION("0D2xtHQYxII", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsAccessTokenTwitch);
    LIB_FUNCTION("vdBd3PMBFp4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsAccessTokenUstream);
    LIB_FUNCTION("TerdSx+FXrc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsAnonymousUserId);
    LIB_FUNCTION("UdZhN1nVYfw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBcTags);
    LIB_FUNCTION("hJ5gj+Pv3-M", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBcTitle);
    LIB_FUNCTION("OALd6SmF220", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBroadcastChannel);
    LIB_FUNCTION("ZopdvNlYFHc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBroadcastersComment);
    LIB_FUNCTION("f5DDIXCTxww", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBroadcastersCommentColor);
    LIB_FUNCTION("LIBEeNNfeQo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBroadcastService);
    LIB_FUNCTION("RdAvEmks-ZE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsBroadcastUiLayout);
    LIB_FUNCTION("HYMgE5B62QY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCamCrop);
    LIB_FUNCTION("N-xzO5-livc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraBgFilter);
    LIB_FUNCTION("GxqMYA60BII", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraBrightness);
    LIB_FUNCTION("Di05lHWmCLU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraChromaKeyLevel);
    LIB_FUNCTION("gGbu3TZiXeU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraContrast);
    LIB_FUNCTION("8PXQIdRsZIE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraDepthLevel);
    LIB_FUNCTION("56bliV+tc0Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraEdgeLevel);
    LIB_FUNCTION("ghjrbwjC0VE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraEffect);
    LIB_FUNCTION("YnBnZpr3UJg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraEliminationLevel);
    LIB_FUNCTION("wWZzH-BwWuA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraPosition);
    LIB_FUNCTION("pnHR-aj9edo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraReflection);
    LIB_FUNCTION("rriXMS0a7BM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraSize);
    LIB_FUNCTION("0e0wzFADy0I", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCameraTransparency);
    LIB_FUNCTION("wQDizdO49CA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsCommunityId);
    LIB_FUNCTION("t1oU0+93b+s", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsFloatingMessage);
    LIB_FUNCTION("bdJdX2bKo2E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsHintFlag);
    LIB_FUNCTION("vRgpAhKJJ+M", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsInitSpectating);
    LIB_FUNCTION("EjxE+-VvuJ4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsCameraHidden);
    LIB_FUNCTION("HfQTiMSCHJk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsFacebookEnabled);
    LIB_FUNCTION("63t6w0MgG8I", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsMuteEnabled);
    LIB_FUNCTION("6oZ3DZGzjIE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsRecDisabled);
    LIB_FUNCTION("AmJ3FJxT7r8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsRecievedMessageHidden);
    LIB_FUNCTION("lsdxBeRnEes", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsIsTwitterEnabled);
    LIB_FUNCTION("wgVAwa31l0E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLanguageFilter);
    LIB_FUNCTION("rDkflpHzrRE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLfpsSortOrder);
    LIB_FUNCTION("qT8-eJKe+rI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLiveQuality);
    LIB_FUNCTION("hQ72M-YRb8g", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLiveQuality2);
    LIB_FUNCTION("ZWAUCzgSQ2Q", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLiveQuality3);
    LIB_FUNCTION("HwFpasG4+kM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLiveQuality4);
    LIB_FUNCTION("Ov8hs+c1GNY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsLiveQuality5);
    LIB_FUNCTION("fm7XpsO++lk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsMessageFilterLevel);
    LIB_FUNCTION("Lge4s3h8BFA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsTtsFlags);
    LIB_FUNCTION("NB9-D-o3hN0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsTtsPitch);
    LIB_FUNCTION("2EWfAroUQE4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsTtsSpeed);
    LIB_FUNCTION("QzeIQXyavtU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetGlsTtsVolume);
    LIB_FUNCTION("WU5s+cPzO8Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetHmuBrightness);
    LIB_FUNCTION("gQh8NaCbRqo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetHmuZoom);
    LIB_FUNCTION("7pif5RySi+s", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetHoldAudioOutDevice);
    LIB_FUNCTION("8TGeI5PAabg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImeAutoCapitalEnabled);
    LIB_FUNCTION("3fcBoTACkWY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImeInitFlag);
    LIB_FUNCTION("Ghu0khDguq8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImeInputType);
    LIB_FUNCTION("hjlUn9UCgXg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImeLastUnit);
    LIB_FUNCTION("19uCF96mfos", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImePointerMode);
    LIB_FUNCTION("NiwMhCbg764", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImePredictiveTextEnabled);
    LIB_FUNCTION("AZFXXpZJEPI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetImeRunCount);
    LIB_FUNCTION("Izy+4XmTBB8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetIPDLeft);
    LIB_FUNCTION("z-lbCrpteB4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetIPDRight);
    LIB_FUNCTION("7SE4sjhlOCI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetIsFakePlus);
    LIB_FUNCTION("nNn8Gnn+E6Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetIsQuickSignup);
    LIB_FUNCTION("AQ680L4Sr74", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetIsRemotePlayAllowed);
    LIB_FUNCTION("lAR1nkEoMBo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetJapaneseInputType);
    LIB_FUNCTION("dCdhOJIOtR4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetKeyboardType);
    LIB_FUNCTION("zs4i9SEHy0g", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetKeyRepeatSpeed);
    LIB_FUNCTION("FfXgMSmZLfk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetKeyRepeatStartingTime);
    LIB_FUNCTION("dlBQfiDOklQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetLightBarBaseBrightness);
    LIB_FUNCTION("Zdd5gybtsi0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetLoginFlag);
    LIB_FUNCTION("c9U2pk4Ao9w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetMicLevel);
    LIB_FUNCTION("lg2I8bETiZo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetMouseHandType);
    LIB_FUNCTION("omf6BE2-FPo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetMousePointerSpeed);
    LIB_FUNCTION("uisYUWMn-+U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNotificationBehavior);
    LIB_FUNCTION("X9Jgur0QtLE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNotificationSettings);
    LIB_FUNCTION("SkE5SnCFjQk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpAccountUpgradeFlag);
    LIB_FUNCTION("nGacpiUONQ0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpAge);
    LIB_FUNCTION("om4jx+pJlQo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpAuthErrorFlag);
    LIB_FUNCTION("Z5t2LiajkAQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpCountryCode);
    LIB_FUNCTION("cGvpAO63abg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpDateOfBirth);
    LIB_FUNCTION("JifncjTlXV8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpEnv);
    LIB_FUNCTION("D7lbcn6Uxho", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpLanguageCode);
    LIB_FUNCTION("oHRrt1cfbBI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpLanguageCode2);
    LIB_FUNCTION("Zgq19lM+u2U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpLoginId);
    LIB_FUNCTION("8W+8vFlIPuA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpMAccountId);
    LIB_FUNCTION("0Xsfib8bq3M", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpNpId);
    LIB_FUNCTION("j6FgkXhxp1Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpOfflineAccountAdult);
    LIB_FUNCTION("pubVXAG+Juc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpOnlineId);
    LIB_FUNCTION("ng4XlNFMiCo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetNpSubAccount);
    LIB_FUNCTION("41kc2YhzZoU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPadSpeakerVolume);
    LIB_FUNCTION("KJw6rahYNdQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalBdAge);
    LIB_FUNCTION("6jPYBCGQgiQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalBrowser);
    LIB_FUNCTION("UT8+lb5fypc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalDvd);
    LIB_FUNCTION("NJpUvo+rezg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalDvdRegion);
    LIB_FUNCTION("gRI+BnPA6UI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalGame);
    LIB_FUNCTION("BPFs-TiU+8Q", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalGameAgeLevel);
    LIB_FUNCTION("mmFgyjXMQBs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetParentalMorpheus);
    LIB_FUNCTION("ZsyQjvVFHnk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPartyMuteList);
    LIB_FUNCTION("97ZkWubtMk0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPartyMuteListA);
    LIB_FUNCTION("IiwhRynrDnQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPartySettingFlags);
    LIB_FUNCTION("7LCq4lSlmw4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPasscode);
    LIB_FUNCTION("dukLb11bY9c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcAdditionalTime);
    LIB_FUNCTION("JK0fCuBEWJM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcFlag);
    LIB_FUNCTION("RUrfnne6Dds", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcFridayDuration);
    LIB_FUNCTION("YWmKJ8pWEkw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcFridayHoursEnd);
    LIB_FUNCTION("GMLAWOO7I2Y", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcFridayHoursStart);
    LIB_FUNCTION("94ZcZmcnXK4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcMode);
    LIB_FUNCTION("SoxZWGb3l0U", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcMondayDuration);
    LIB_FUNCTION("uBDKFasVr2c", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcMondayHoursEnd);
    LIB_FUNCTION("7XIlJQQZ2fg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcMondayHoursStart);
    LIB_FUNCTION("ABoN0o46u8E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcPlayTime);
    LIB_FUNCTION("VXdkxm-AaIg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcPlayTimeLastUpdated);
    LIB_FUNCTION("RTrsbjUnFNo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSaturdayDuration);
    LIB_FUNCTION("8wVUn7AO8mA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSaturdayHoursEnd);
    LIB_FUNCTION("p2NKAA3BS6k", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSaturdayHoursStart);
    LIB_FUNCTION("hGnwgvLREHM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSundayDuration);
    LIB_FUNCTION("rp4DB+ICfcg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSundayHoursEnd);
    LIB_FUNCTION("cTpHiHGMWpk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcSundayHoursStart);
    LIB_FUNCTION("R9vnyf-B1pU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcThursdayDuration);
    LIB_FUNCTION("W3oNrewI7bc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcThursdayHoursEnd);
    LIB_FUNCTION("JO5QXiyBcjQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcThursdayHoursStart);
    LIB_FUNCTION("YX-64Vjk5oM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcTuesdayDuration);
    LIB_FUNCTION("MtE3Me0UJKc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcTuesdayHoursEnd);
    LIB_FUNCTION("bLfjqFmN4s4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcTuesdayHoursStart);
    LIB_FUNCTION("HsjvaxD7veE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcTzOffset);
    LIB_FUNCTION("EqfGtRCryNg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcWednesdayDuration);
    LIB_FUNCTION("uZG5rmROeg4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcWednesdayHoursEnd);
    LIB_FUNCTION("dDaO7svUM8w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPbtcWednesdayHoursStart);
    LIB_FUNCTION("pmW5v9hORos", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPlayTogetherFlags);
    LIB_FUNCTION("nCfhbtuZbk8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetPsnPasswordForDebug);
    LIB_FUNCTION("ksUJCL0Hq20", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSaveDataAutoUpload);
    LIB_FUNCTION("pfz4rzKJc6g", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSaveDataSort);
    LIB_FUNCTION("zq45SROKj9Q", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSaveDataTutorialFlag);
    LIB_FUNCTION("bFzA3t6muvU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetShareButtonAssign);
    LIB_FUNCTION("B-WW6mNtp2s", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetShareDailymotionAccessToken);
    LIB_FUNCTION("OANH5P9lV4I", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetShareDailymotionRefreshToken);
    LIB_FUNCTION("CMl8mUJvSf8", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSharePlayFlags);
    LIB_FUNCTION("rB70KuquYxs", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSharePlayFramerateHost);
    LIB_FUNCTION("BhRxR+R0NFA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSharePlayResolutionHost);
    LIB_FUNCTION("EYvRF1VUpUU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetShareStatus);
    LIB_FUNCTION("II+V6wXKS-E", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetShareStatus2);
    LIB_FUNCTION("5jL7UM+AdbQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSystemLoggerHashedAccountId);
    LIB_FUNCTION("tNZY3tIIo0M", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSystemLoggerHashedAccountIdClockType);
    LIB_FUNCTION("U07X36vgbA0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSystemLoggerHashedAccountIdParam);
    LIB_FUNCTION("qSgs-wwrlLU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetSystemLoggerHashedAccountIdTtl);
    LIB_FUNCTION("b6+TytWccPE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetTeamShowAboutTeam);
    LIB_FUNCTION("JZ5NzN-TGIQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeBgImageDimmer);
    LIB_FUNCTION("N4qrFLcXLpY", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeBgImageWaveColor);
    LIB_FUNCTION("a41mGTpWvY4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeBgImageZoom);
    LIB_FUNCTION("ALyjUuyowuI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeEntitlementId);
    LIB_FUNCTION("jhy6fa5a4k4", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeHomeShareOwner);
    LIB_FUNCTION("HkuBuYhYaPg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeTextShadow);
    LIB_FUNCTION("PKHZK960qZE", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetThemeWaveColor);
    LIB_FUNCTION("f7VSHQHB6Ys", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetTopMenuLimitItem);
    LIB_FUNCTION("Tib8zgDd+V0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetTopMenuNotificationFlag);
    LIB_FUNCTION("8Q71i3u9lN0", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetTopMenuTutorialFlag);
    LIB_FUNCTION("ZfUouUx2h8w", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetTraditionalChineseInputType);
    LIB_FUNCTION("IcM2f5EoRRA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetUserGroupIndex);
    LIB_FUNCTION("QfYasZZPvoQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetUserGroupName);
    LIB_FUNCTION("Jqu2XFr5UvA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetUserName);
    LIB_FUNCTION("cBgv9pnmunI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetUserStatus);
    LIB_FUNCTION("CokWh8qGANk", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVibrationEnabled);
    LIB_FUNCTION("z1Uh28yzDzI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVoiceRecognitionLastUsedOsk);
    LIB_FUNCTION("1JNYgwRcANI", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVoiceRecognitionTutorialState);
    LIB_FUNCTION("4nEjiZH1LKM", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVolumeForController);
    LIB_FUNCTION("bkQ7aNx62Qg", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVolumeForGenericUSB);
    LIB_FUNCTION("7EnjUtnAN+o", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVolumeForMorpheusSidetone);
    LIB_FUNCTION("WQ-l-i2gJko", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceSetVolumeForSidetone);
    LIB_FUNCTION("bwFjS+bX9mA", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceTerminate);
    LIB_FUNCTION("spW--yoLQ9o", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceUnregisterEventCallback);
    LIB_FUNCTION("isbcQWjV-qU", "libSceUserService", 1, "libSceUserService", 1, 1,
                 Func_8AC6DC4168D5FEA5);
    LIB_FUNCTION("pr3J39r9ArQ", "libSceUserService", 1, "libSceUserService", 1, 1,
                 Func_A6BDC9DFDAFD02B4);
    LIB_FUNCTION("u5SR3+a0lTw", "libSceUserService", 1, "libSceUserService", 1, 1,
                 Func_BB9491DFE6B4953C);
    LIB_FUNCTION("0rgUYD57RHc", "libSceUserService", 1, "libSceUserService", 1, 1,
                 Func_D2B814603E7B4477);
    LIB_FUNCTION("wuI7c7UNk0A", "libSceUserServiceForNpToolkit", 1, "libSceUserService", 1, 1,
                 sceUserServiceRegisterEventCallback);
    LIB_FUNCTION("spW--yoLQ9o", "libSceUserServiceForNpToolkit", 1, "libSceUserService", 1, 1,
                 sceUserServiceUnregisterEventCallback);
    LIB_FUNCTION("5EiQCnL2G1Y", "libSceUserServiceRegisteredUserIdList", 1, "libSceUserService", 1,
                 1, sceUserServiceGetRegisteredUserIdList);
};

} // namespace Libraries::UserService
