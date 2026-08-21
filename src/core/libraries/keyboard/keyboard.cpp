// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/keyboard/keyboard.h"
#include "core/libraries/libs.h"

#include "SDL3/SDL_keyboard.h"
#include "keyboard.h"
#include "core/emulator_settings.h"

namespace Libraries::Keyboard {

s32 PS4_SYSV_ABI sceKeyboardClose() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardConnectPort() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardDebugGetDeviceId() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardDeviceOpen() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardDisconnectDevice() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardDisconnectPort() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardGetConnection() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardGetDeviceInfo() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardGetKey2Char() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardInit() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardMbusInit() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardOpen() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return 1;
}

s32 PS4_SYSV_ABI sceKeyboardRead() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardReadState(s32 handle, OrbisKeyboardData* data) {
    LOG_DEBUG(Lib_Keyboard, "(DUMMY) called");
    if (handle != 1) {
        return ORBIS_KEYBOARD_ERROR_INVALID_HANDLE;
    }
    if (!EmulatorSettings.IsKeyboardUsedAsKeyboard()) {
        data->nkeys = 0;
        return ORBIS_OK;
    }
    static bool prev_state[SDL_SCANCODE_COUNT]{};
    s32 numkeys;
    auto buf = SDL_GetKeyboardState(&numkeys);
    int j = 0;
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        if (buf[i]) {
            data->keycodes[j++] = i;
        }
    }
    data->nkeys = j;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardSetProcessFocus() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKeyboardSetProcessPrivilege() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_97BEA3A6D1DF8B33() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardClose() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardGetKey2Char() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardInit() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardOpen() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardRead() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgKeyboardReadState() {
    LOG_ERROR(Lib_Keyboard, "(STUBBED) called");
    return ORBIS_OK;
}

bool PushSDLEvent(SDL_Event& e) {
    if (!EmulatorSettings.IsKeyboardUsedAsKeyboard()) {
        return false;
    }
    return e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0LWei+c7RNc", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardClose);
    LIB_FUNCTION("cKyUWdEKBME", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardConnectPort);
    LIB_FUNCTION("qkHOAYtCFxg", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardDebugGetDeviceId);
    LIB_FUNCTION("0lAFLqZonXI", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardDeviceOpen);
    LIB_FUNCTION("DhTavziiZ80", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardDisconnectDevice);
    LIB_FUNCTION("Hdm6rO0xlnw", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardDisconnectPort);
    LIB_FUNCTION("XNArvUsT18s", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardGetConnection);
    LIB_FUNCTION("84mvEJFMYK4", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardGetDeviceInfo);
    LIB_FUNCTION("yO9JwdRhtSA", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardGetKey2Char);
    LIB_FUNCTION("wadT3QBCGY0", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardInit);
    LIB_FUNCTION("6zKouhabgjM", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardMbusInit);
    LIB_FUNCTION("HJ+KnEHcaxI", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardOpen);
    LIB_FUNCTION("xybbGMCr738", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardRead);
    LIB_FUNCTION("6HpE68bzX6M", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardReadState);
    LIB_FUNCTION("jqRcNtZmLuE", "libSceKeyboard", 1, "libSceKeyboard", sceKeyboardSetProcessFocus);
    LIB_FUNCTION("K1A7IJp5xYc", "libSceKeyboard", 1, "libSceKeyboard",
                 sceKeyboardSetProcessPrivilege);
    LIB_FUNCTION("l76jptHfizM", "libSceKeyboard", 1, "libSceKeyboard", Func_97BEA3A6D1DF8B33);
    LIB_FUNCTION("FU9dBtKaM90", "libSceDbgKeyboard", 1, "libSceKeyboard", sceDbgKeyboardClose);
    LIB_FUNCTION("61ykjnH3AR4", "libSceDbgKeyboard", 1, "libSceKeyboard",
                 sceDbgKeyboardGetKey2Char);
    LIB_FUNCTION("bRuUeIPXFDM", "libSceDbgKeyboard", 1, "libSceKeyboard", sceDbgKeyboardInit);
    LIB_FUNCTION("TESmX5D-i54", "libSceDbgKeyboard", 1, "libSceKeyboard", sceDbgKeyboardOpen);
    LIB_FUNCTION("vKbf07G7KPE", "libSceDbgKeyboard", 1, "libSceKeyboard", sceDbgKeyboardRead);
    LIB_FUNCTION("HJMS5pu4dvY", "libSceDbgKeyboard", 1, "libSceKeyboard", sceDbgKeyboardReadState);
};

} // namespace Libraries::Keyboard