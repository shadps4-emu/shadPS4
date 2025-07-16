// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/process.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SysModule {

enum class OrbisSysModule : u16 {
    ORBIS_SYSMODULE_INVALID = 0x0000,
    ORBIS_SYSMODULE_FIBER = 0x0006,            // libSceFiber.sprx
    ORBIS_SYSMODULE_ULT = 0x0007,              // libSceUlt.sprx
    ORBIS_SYSMODULE_NGS2 = 0x000B,             // libSceNgs2.sprx
    ORBIS_SYSMODULE_XML = 0x0017,              // libSceXml.sprx
    ORBIS_SYSMODULE_NP_UTILITY = 0x0019,       // libSceNpUtility.sprx
    ORBIS_SYSMODULE_VOICE = 0x001A,            // libSceVoice.sprx
    ORBIS_SYSMODULE_VOICEQOS = 0x001B,         // libSceVoiceQos.sprx
    ORBIS_SYSMODULE_NP_MATCHING2 = 0x001C,     // libSceNpMatching2.sprx
    ORBIS_SYSMODULE_NP_SCORE_RANKING = 0x001E, // libSceNpScoreRanking.sprx
    ORBIS_SYSMODULE_RUDP = 0x0021,             // libSceRudp.sprx
    ORBIS_SYSMODULE_NP_TUS = 0x002C,           // libSceNpTus.sprx
    ORBIS_SYSMODULE_FACE = 0x0038,
    ORBIS_SYSMODULE_SMART = 0x0039,
    ORBIS_SYSMODULE_JSON = 0x0080,
    ORBIS_SYSMODULE_GAME_LIVE_STREAMING = 0x0081, // libSceGameLiveStreaming.sprx
    ORBIS_SYSMODULE_COMPANION_UTIL = 0x0082,      // libSceCompanionUtil.sprx
    ORBIS_SYSMODULE_PLAYGO = 0x0083,              // libScePlayGo.sprx
    ORBIS_SYSMODULE_FONT = 0x0084,                // libSceFont.sprx
    ORBIS_SYSMODULE_VIDEO_RECORDING = 0x0085,     // libSceVideoRecording.sprx
    ORBIS_SYSMODULE_S3DCONVERSION = 0x0086,
    ORBIS_SYSMODULE_AUDIODEC = 0x0088,    // libSceAudiodec.sprx
    ORBIS_SYSMODULE_JPEG_DEC = 0x008A,    // libSceJpegDec.sprx
    ORBIS_SYSMODULE_JPEG_ENC = 0x008B,    // libSceJpegEnc.sprx
    ORBIS_SYSMODULE_PNG_DEC = 0x008C,     // libScePngDec.sprx
    ORBIS_SYSMODULE_PNG_ENC = 0x008D,     // libScePngEnc.sprx
    ORBIS_SYSMODULE_VIDEODEC = 0x008E,    // libSceVideodec.sprx
    ORBIS_SYSMODULE_MOVE = 0x008F,        // libSceMove.sprx
    ORBIS_SYSMODULE_PAD_TRACKER = 0x0091, // libScePadTracker.sprx
    ORBIS_SYSMODULE_DEPTH = 0x0092,       // libSceDepth.sprx
    ORBIS_SYSMODULE_HAND = 0x0093,
    ORBIS_SYSMODULE_LIBIME = 0x0095,          // libSceIme.sprx
    ORBIS_SYSMODULE_IME_DIALOG = 0x0096,      // libSceImeDialog.sprx
    ORBIS_SYSMODULE_NP_PARTY = 0x0097,        // libSceNpParty.sprx
    ORBIS_SYSMODULE_FONT_FT = 0x0098,         // libSceFontFt.sprx
    ORBIS_SYSMODULE_FREETYPE_OT = 0x0099,     // libSceFreeTypeOt.sprx
    ORBIS_SYSMODULE_FREETYPE_OL = 0x009A,     // libSceFreeTypeOl.sprx
    ORBIS_SYSMODULE_FREETYPE_OPT_OL = 0x009B, // libSceFreeTypeOptOl.sprx
    ORBIS_SYSMODULE_SCREEN_SHOT = 0x009C,     // libSceScreenShot.sprx
    ORBIS_SYSMODULE_NP_AUTH = 0x009D,         // libSceNpAuth.sprx
    ORBIS_SYSMODULE_SULPHA = 0x009F,
    ORBIS_SYSMODULE_SAVE_DATA_DIALOG = 0x00A0,  // libSceSaveDataDialog.sprx
    ORBIS_SYSMODULE_INVITATION_DIALOG = 0x00A2, // libSceInvitationDialog.sprx
    ORBIS_SYSMODULE_DEBUG_KEYBOARD = 0x00A3,
    ORBIS_SYSMODULE_MESSAGE_DIALOG = 0x00A4,          // libSceMsgDialog.sprx
    ORBIS_SYSMODULE_AV_PLAYER = 0x00A5,               // libSceAvPlayer.sprx
    ORBIS_SYSMODULE_CONTENT_EXPORT = 0x00A6,          // libSceContentExport.sprx
    ORBIS_SYSMODULE_AUDIO_3D = 0x00A7,                // libSceAudio3d.sprx
    ORBIS_SYSMODULE_NP_COMMERCE = 0x00A8,             // libSceNpCommerce.sprx
    ORBIS_SYSMODULE_MOUSE = 0x00A9,                   // libSceMouse.sprx
    ORBIS_SYSMODULE_COMPANION_HTTPD = 0x00AA,         // libSceCompanionHttpd.sprx
    ORBIS_SYSMODULE_WEB_BROWSER_DIALOG = 0x00AB,      // libSceWebBrowserDialog.sprx
    ORBIS_SYSMODULE_ERROR_DIALOG = 0x00AC,            // libSceErrorDialog.sprx
    ORBIS_SYSMODULE_NP_TROPHY = 0x00AD,               // libSceNpTrophy.sprx
    ORBIS_SYSMODULE_VIDEO_CORE_IF = 0x00AE,           // libSceVideoCoreInterface.sprx
    ORBIS_SYSMODULE_VIDEO_CORE_SERVER_IF = 0x00AF,    // libSceVideoCoreServerInterface.sprx
    ORBIS_SYSMODULE_NP_SNS_FACEBOOK = 0x00B0,         // libSceNpSnsFacebookDialog.sprx
    ORBIS_SYSMODULE_MOVE_TRACKER = 0x00B1,            // libSceMoveTracker.sprx
    ORBIS_SYSMODULE_NP_PROFILE_DIALOG = 0x00B2,       // libSceNpProfileDialog.sprx
    ORBIS_SYSMODULE_NP_FRIEND_LIST_DIALOG = 0x00B3,   // libSceNpFriendListDialog.sprx
    ORBIS_SYSMODULE_APP_CONTENT = 0x00B4,             // libSceAppContent.sprx
    ORBIS_SYSMODULE_NP_SIGNALING = 0x00B5,            // libSceNpSignaling.sprx
    ORBIS_SYSMODULE_REMOTE_PLAY = 0x00B6,             // libSceRemoteplay.sprx
    ORBIS_SYSMODULE_USBD = 0x00B7,                    // libSceUsbd.sprx
    ORBIS_SYSMODULE_GAME_CUSTOM_DATA_DIALOG = 0x00B8, // libSceGameCustomDataDialog.sprx
    ORBIS_SYSMODULE_NP_EULA_DIALOG = 0x00B9,          // libSceNpEulaDialog.sprx
    ORBIS_SYSMODULE_RANDOM = 0x00BA,                  // libSceRandom.sprx
    ORBIS_SYSMODULE_RESERVED2 = 0x00BB,
    ORBIS_SYSMODULE_M4AAC_ENC = 0x00BC,                  // libSceM4aacEnc.sprx
    ORBIS_SYSMODULE_AUDIODEC_CPU = 0x00BD,               // libSceAudiodecCpu.sprx
    ORBIS_SYSMODULE_AUDIODEC_CPU_DDP = 0x00BE,           // libSceAudiodecCpuDdp.sprx
    ORBIS_SYSMODULE_AUDIODEC_CPU_M4AAC = 0x00C0,         // libSceAudiodecCpuM4aac.sprx
    ORBIS_SYSMODULE_BEMP2_SYS = 0x00C1,                  // libSceBemp2sys.sprx
    ORBIS_SYSMODULE_BEISOBMF = 0x00C2,                   // libSceBeisobmf.sprx
    ORBIS_SYSMODULE_PLAY_READY = 0x00C3,                 // libScePlayReady.sprx
    ORBIS_SYSMODULE_VIDEO_NATIVE_EXT_ESSENTIAL = 0x00C4, // libSceVideoNativeExtEssential.sprx
    ORBIS_SYSMODULE_ZLIB = 0x00C5,                       // libSceZlib.sprx
    ORBIS_SYSMODULE_DTCP_IP = 0x00C6,                    // libSceDtcpIp.sprx
    ORBIS_SYSMODULE_CONTENT_SEARCH = 0x00C7,             // libSceContentSearch.sprx
    ORBIS_SYSMODULE_SHARE_UTILITY = 0x00C8,              // libSceShareUtility.sprx
    ORBIS_SYSMODULE_AUDIODEC_CPU_DTS_HD_LBR = 0x00C9,    // libSceAudiodecCpuDtsHdLbr.sprx
    ORBIS_SYSMODULE_DECI4H = 0x00CA,
    ORBIS_SYSMODULE_HEAD_TRACKER = 0x00CB,
    ORBIS_SYSMODULE_GAME_UPDATE = 0x00CC,         // libSceGameUpdate.sprx
    ORBIS_SYSMODULE_AUTO_MOUNTER_CLIENT = 0x00CD, // libSceAutoMounterClient.sprx
    ORBIS_SYSMODULE_SYSTEM_GESTURE = 0x00CE,      // libSceSystemGesture.sprx
    ORBIS_SYSMODULE_VIDEODEC2 = 0x00CF,
    ORBIS_SYSMODULE_VDECWRAP = 0x00D0,           // libSceVdecwrap.sprx
    ORBIS_SYSMODULE_AT9_ENC = 0x00D1,            // libSceAt9Enc.sprx
    ORBIS_SYSMODULE_CONVERT_KEYCODE = 0x00D2,    // libSceConvertKeycode.sprx
    ORBIS_SYSMODULE_SHARE_PLAY = 0x00D3,         // libSceSharePlay.sprx
    ORBIS_SYSMODULE_HMD = 0x00D4,                // libSceHmd.sprx
    ORBIS_SYSMODULE_USB_STORAGE = 0x00D5,        // libSceUsbStorage.sprx
    ORBIS_SYSMODULE_USB_STORAGE_DIALOG = 0x00D6, // libSceUsbStorageDialog.sprx
    ORBIS_SYSMODULE_DISC_MAP = 0x00D7,           // libSceDiscMap.sprx
    ORBIS_SYSMODULE_FACE_TRACKER = 0x00D8,
    ORBIS_SYSMODULE_HAND_TRACKER = 0x00D9,
    ORBIS_SYSMODULE_NP_SNS_YOUTUBE_DIALOG = 0x00DA,     // libSceNpSnsYouTubeDialog.sprx
    ORBIS_SYSMODULE_PROFILE_CACHE_EXTERNAL = 0x00DC,    // libSceProfileCacheExternal.sprx
    ORBIS_SYSMODULE_MUSIC_PLAYER_SERVICE = 0x00DD,      // libSceMusicPlayerService.sprx
    ORBIS_SYSMODULE_SP_SYS_CALL_WRAPPER = 0x00DE,       // libSceSpSysCallWrapper.sprx
    ORBIS_SYSMODULE_PS2_EMU_MENU_DIALOG = 0x00DF,       // libScePs2EmuMenuDialog.sprx
    ORBIS_SYSMODULE_NP_SNS_DIALYMOTION_DIALOG = 0x00E0, // libSceNpSnsDailyMotionDialog.sprx
    ORBIS_SYSMODULE_AUDIODEC_CPU_HEVAG = 0x00E1,        // libSceAudiodecCpuHevag.sprx
    ORBIS_SYSMODULE_LOGIN_DIALOG = 0x00E2,              // libSceLoginDialog.sprx
    ORBIS_SYSMODULE_LOGIN_SERVICE = 0x00E3,             // libSceLoginService.sprx
    ORBIS_SYSMODULE_SIGNIN_DIALOG = 0x00E4,             // libSceSigninDialog.sprx
    ORBIS_SYSMODULE_VDECSW = 0x00E5,                    // libSceVdecsw.sprx
    ORBIS_SYSMODULE_CUSTOM_MUSIC_CORE = 0x00E6,         // libSceCustomMusicCore.sprx
    ORBIS_SYSMODULE_JSON2 = 0x00E7,                     // libSceJson2.sprx
    ORBIS_SYSMODULE_AUDIO_LATENCY_ESTIMATION = 0x00E8,
    ORBIS_SYSMODULE_WK_FONT_CONFIG = 0x00E9, // libSceWkFontConfig.sprx
    ORBIS_SYSMODULE_RESERVED27 = 0x00EA,
    ORBIS_SYSMODULE_HMD_SETUP_DIALOG = 0x00EB, // libSceHmdSetupDialog.sprx
    ORBIS_SYSMODULE_RESERVED28 = 0x00EC,
    ORBIS_SYSMODULE_VR_TRACKER = 0x00ED,        // libSceVrTracker.sprx
    ORBIS_SYSMODULE_CONTENT_DELETE = 0x00EE,    // libSceContentDelete.sprx
    ORBIS_SYSMODULE_IME_BACKEND = 0x00EF,       // libSceImeBackend.sprx
    ORBIS_SYSMODULE_NET_CTL_AP_DIALOG = 0x00F0, // libSceNetCtlApDialog.sprx
    ORBIS_SYSMODULE_PLAYGO_DIALOG = 0x00F1,     // libScePlayGoDialog.sprx
    ORBIS_SYSMODULE_SOCIAL_SCREEN = 0x00F2,     // libSceSocialScreen.sprx
    ORBIS_SYSMODULE_EDIT_MP4 = 0x00F3,          // libSceEditMp4.sprx
    ORBIS_SYSMODULE_PSM_KIT_SYSTEM = 0x00F5,    // libScePsmKitSystem.sprx
    ORBIS_SYSMODULE_TEXT_TO_SPEECH = 0x00F6,    // libSceTextToSpeech.sprx
    ORBIS_SYSMODULE_NP_TOOLKIT = 0x00F7,
    ORBIS_SYSMODULE_CUSTOM_MUSIC_SERVICE = 0x00F8,      // libSceCustomMusicService.sprx
    ORBIS_SYSMODULE_CL_SYS_CALL_WRAPPER = 0x00F9,       // libSceClSysCallWrapper.sprx
    ORBIS_SYSMODULE_SYSTEM_LOGGER = 0x00FA,             // libSceSystemLogger.sprx
    ORBIS_SYSMODULE_BLUETOOTH_HID = 0x00FB,             // libSceBluetoothHid.sprx
    ORBIS_SYSMODULE_VIDEO_DECODER_ARBITRATION = 0x00FC, // libSceVideoDecoderArbitration.sprx
    ORBIS_SYSMODULE_VR_SERVICE_DIALOG = 0x00FD,         // libSceVrServiceDialog.sprx
    ORBIS_SYSMODULE_JOB_MANAGER = 0x00FE,
    ORBIS_SYSMODULE_KEYBOARD = 0x0106,
};

enum class OrbisSysModuleInternal : u32 {
    ORBIS_SYSMODULE_INTERNAL_RAZOR_CPU = 0x80000019, // libSceRazorCpu.sprx
};

int PS4_SYSV_ABI sceSysmoduleGetModuleHandleInternal();
s32 PS4_SYSV_ABI sceSysmoduleGetModuleInfoForUnwind(VAddr addr, s32 flags,
                                                    Kernel::OrbisModuleInfoForUnwind* info);
int PS4_SYSV_ABI sceSysmoduleIsCalledFromSysModule();
int PS4_SYSV_ABI sceSysmoduleIsCameraPreloaded();
int PS4_SYSV_ABI sceSysmoduleIsLoaded(OrbisSysModule id);
int PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(OrbisSysModuleInternal id);
int PS4_SYSV_ABI sceSysmoduleLoadModule(OrbisSysModule id);
int PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal();
int PS4_SYSV_ABI sceSysmoduleLoadModuleInternal();
int PS4_SYSV_ABI sceSysmoduleLoadModuleInternalWithArg();
int PS4_SYSV_ABI sceSysmoduleMapLibcForLibkernel();
int PS4_SYSV_ABI sceSysmodulePreloadModuleForLibkernel();
int PS4_SYSV_ABI sceSysmoduleUnloadModule();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleByNameInternal();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternal();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternalWithArg();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SysModule
