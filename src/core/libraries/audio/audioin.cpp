// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/sdl_in.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::AudioIn {

static std::unique_ptr<SDLAudioIn> audio = std::make_unique<SDLAudioIn>();

int PS4_SYSV_ABI sceAudioInChangeAppModuleState() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInClose(s32 handle) {
    audio->AudioInClose(handle);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInCountPorts() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceHqOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceIdHqOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceIdOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtClose() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtCtrl() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtInput() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtSetAecMode() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetHandleStatusInfo() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetRerouteCount() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetSilentState() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInHqOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                  u32 index, u32 len, u32 freq, u32 param) {
    int result = audio->AudioInOpen(type, len, freq, param);
    if (result < 0) {
        LOG_ERROR(Lib_AudioIn, "Error returned  {:#x}", result);
    }
    return result;
}

int PS4_SYSV_ABI sceAudioInHqOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInInit() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInInput(s32 handle, void* dest) {
    return audio->AudioInInput(handle, dest);
}

int PS4_SYSV_ABI sceAudioInInputs() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInIsSharedDevice() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                u32 index, u32 len, u32 freq, u32 param) {
    int result = audio->AudioInOpen(type, len, freq, param);
    if (result < 0) {
        LOG_ERROR(Lib_AudioIn, "Error returned  {:#x}", result);
    }
    return result;
}

int PS4_SYSV_ABI sceAudioInOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetAllMute() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetCompressorPreGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetConnections() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetConnectionsForUser() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetDevConnection() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetFocusForUser() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetMode() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetMode2() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetPortConnections() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetPortStatuses() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetSparkParam() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetSparkSideTone() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetUsbGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetUserMute() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicCreate() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicDestroy() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicWrite() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("IQtWgnrw6v8", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInChangeAppModuleState);
    LIB_FUNCTION("Jh6WbHhnI68", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInClose);
    LIB_FUNCTION("8mtcsG-Qp5E", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInCountPorts);
    LIB_FUNCTION("5qRVfxOmbno", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInDeviceHqOpen);
    LIB_FUNCTION("gUNabrUkZNg", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInDeviceIdHqOpen);
    LIB_FUNCTION("X-AQLtdxQOo", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInDeviceIdOpen);
    LIB_FUNCTION("VoX9InuwwTg", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInDeviceOpen);
    LIB_FUNCTION("48-miagyJ2I", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInDeviceOpenEx);
    LIB_FUNCTION("kFKJ3MVcDuo", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInExtClose);
    LIB_FUNCTION("mhAfefP9m2g", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInExtCtrl);
    LIB_FUNCTION("KpBKoHKVKEc", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInExtInput);
    LIB_FUNCTION("YZ+3seW7CyY", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInExtOpen);
    LIB_FUNCTION("FVGWf8JaHOE", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInExtSetAecMode);
    LIB_FUNCTION("S-rDUfQk9sg", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInGetGain);
    LIB_FUNCTION("NJam1-F7lNY", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInGetHandleStatusInfo);
    LIB_FUNCTION("3shKmTrTw6c", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInGetRerouteCount);
    LIB_FUNCTION("BohEAQ7DlUE", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInGetSilentState);
    LIB_FUNCTION("nya-R5gDYhM", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInHqOpen);
    LIB_FUNCTION("CTh72m+IYbU", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInHqOpenEx);
    LIB_FUNCTION("SxQprgjttKE", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInInit);
    LIB_FUNCTION("LozEOU8+anM", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInInput);
    LIB_FUNCTION("rmgXsZ-2Tyk", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInInputs);
    LIB_FUNCTION("6QP1MzdFWhs", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInIsSharedDevice);
    LIB_FUNCTION("5NE8Sjc7VC8", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInOpen);
    LIB_FUNCTION("+DY07NwJb0s", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInOpenEx);
    LIB_FUNCTION("vYFsze1SqU8", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetAllMute);
    LIB_FUNCTION("vyh-T6sMqnw", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetCompressorPreGain);
    LIB_FUNCTION("YeBSNVAELe4", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetConnections);
    LIB_FUNCTION("thLNHvkWSeg", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetConnectionsForUser);
    LIB_FUNCTION("rcgv2ciDrtc", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetDevConnection);
    LIB_FUNCTION("iN3KqF-8R-w", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetFocusForUser);
    LIB_FUNCTION("VAzfxqDwbQ0", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetMode);
    LIB_FUNCTION("CwBFvAlOv7k", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetMode2);
    LIB_FUNCTION("tQpOPpYwv7o", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetPortConnections);
    LIB_FUNCTION("NUWqWguYcNQ", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetPortStatuses);
    LIB_FUNCTION("U0ivfdKFZbA", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetSparkParam);
    LIB_FUNCTION("hWMCAPpqzDo", "libSceAudioIn", 1, "libSceAudioIn", 1, 1,
                 sceAudioInSetSparkSideTone);
    LIB_FUNCTION("nqXpw3MaN50", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetUsbGain);
    LIB_FUNCTION("arJp991xk5k", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInSetUserMute);
    LIB_FUNCTION("DVTn+iMSpBM", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInVmicCreate);
    LIB_FUNCTION("3ULZGIl+Acc", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInVmicDestroy);
    LIB_FUNCTION("4kHw99LUG3A", "libSceAudioIn", 1, "libSceAudioIn", 1, 1, sceAudioInVmicWrite);
};

} // namespace Libraries::AudioIn
