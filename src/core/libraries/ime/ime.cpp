// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ime.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Ime {

int PS4_SYSV_ABI FinalizeImeModule() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI InitializeImeModule() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckFilterText() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckRemoteEventParam() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckUpdateTextInfo() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeClose() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeConfigGet() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeConfigSet() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeConfirmCandidate() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicAddWord() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicDeleteLearnDics() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicDeleteUserDics() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicDeleteWord() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicGetWords() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDicReplaceWord() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDisableController() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeFilterText() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeForTestFunction() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeGetPanelPositionAndForm() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeGetPanelSize() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardClose() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetInfo() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetResourceId() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardOpen() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardSetMode() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardUpdate() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeOpen() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeParamInit() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeSetCandidateIndex() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeSetCaret() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeSetText() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeSetTextGeometry() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeUpdate() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshClearPreedit() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshClose() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshConfirmPreedit() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshDisableController() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshGetPanelPositionAndForm() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshInformConfirmdString() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshInformConfirmdString2() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshOpen() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSendTextInfo() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetCaretGeometry() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetCaretIndexInPreedit() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetPanelPosition() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetParam() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetPreeditGeometry() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetSelectGeometry() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshSetSelectionText() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshUpdate() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshUpdateContext() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeVshUpdateContext2() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceIme(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("mN+ZoSN-8hQ", "libSceIme", 1, "libSceIme", 1, 1, FinalizeImeModule);
    LIB_FUNCTION("uTW+63goeJs", "libSceIme", 1, "libSceIme", 1, 1, InitializeImeModule);
    LIB_FUNCTION("Lf3DeGWC6xg", "libSceIme", 1, "libSceIme", 1, 1, sceImeCheckFilterText);
    LIB_FUNCTION("zHuMUGb-AQI", "libSceIme", 1, "libSceIme", 1, 1, sceImeCheckRemoteEventParam);
    LIB_FUNCTION("OTb0Mg+1i1k", "libSceIme", 1, "libSceIme", 1, 1, sceImeCheckUpdateTextInfo);
    LIB_FUNCTION("TmVP8LzcFcY", "libSceIme", 1, "libSceIme", 1, 1, sceImeClose);
    LIB_FUNCTION("Ho5NVQzpKHo", "libSceIme", 1, "libSceIme", 1, 1, sceImeConfigGet);
    LIB_FUNCTION("P5dPeiLwm-M", "libSceIme", 1, "libSceIme", 1, 1, sceImeConfigSet);
    LIB_FUNCTION("tKLmVIUkpyM", "libSceIme", 1, "libSceIme", 1, 1, sceImeConfirmCandidate);
    LIB_FUNCTION("NYDsL9a0oEo", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicAddWord);
    LIB_FUNCTION("l01GKoyiQrY", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicDeleteLearnDics);
    LIB_FUNCTION("E2OcGgi-FPY", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicDeleteUserDics);
    LIB_FUNCTION("JAiMBkOTYKI", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicDeleteWord);
    LIB_FUNCTION("JoPdCUXOzMU", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicGetWords);
    LIB_FUNCTION("FuEl46uHDyo", "libSceIme", 1, "libSceIme", 1, 1, sceImeDicReplaceWord);
    LIB_FUNCTION("E+f1n8e8DAw", "libSceIme", 1, "libSceIme", 1, 1, sceImeDisableController);
    LIB_FUNCTION("evjOsE18yuI", "libSceIme", 1, "libSceIme", 1, 1, sceImeFilterText);
    LIB_FUNCTION("wVkehxutK-U", "libSceIme", 1, "libSceIme", 1, 1, sceImeForTestFunction);
    LIB_FUNCTION("T6FYjZXG93o", "libSceIme", 1, "libSceIme", 1, 1, sceImeGetPanelPositionAndForm);
    LIB_FUNCTION("ziPDcIjO0Vk", "libSceIme", 1, "libSceIme", 1, 1, sceImeGetPanelSize);
    LIB_FUNCTION("PMVehSlfZ94", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardClose);
    LIB_FUNCTION("VkqLPArfFdc", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardGetInfo);
    LIB_FUNCTION("dKadqZFgKKQ", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardGetResourceId);
    LIB_FUNCTION("eaFXjfJv3xs", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardOpen);
    LIB_FUNCTION("oYkJlMK51SA", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardOpenInternal);
    LIB_FUNCTION("ua+13Hk9kKs", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardSetMode);
    LIB_FUNCTION("3Hx2Uw9xnv8", "libSceIme", 1, "libSceIme", 1, 1, sceImeKeyboardUpdate);
    LIB_FUNCTION("RPydv-Jr1bc", "libSceIme", 1, "libSceIme", 1, 1, sceImeOpen);
    LIB_FUNCTION("16UI54cWRQk", "libSceIme", 1, "libSceIme", 1, 1, sceImeOpenInternal);
    LIB_FUNCTION("WmYDzdC4EHI", "libSceIme", 1, "libSceIme", 1, 1, sceImeParamInit);
    LIB_FUNCTION("TQaogSaqkEk", "libSceIme", 1, "libSceIme", 1, 1, sceImeSetCandidateIndex);
    LIB_FUNCTION("WLxUN2WMim8", "libSceIme", 1, "libSceIme", 1, 1, sceImeSetCaret);
    LIB_FUNCTION("ieCNrVrzKd4", "libSceIme", 1, "libSceIme", 1, 1, sceImeSetText);
    LIB_FUNCTION("TXYHFRuL8UY", "libSceIme", 1, "libSceIme", 1, 1, sceImeSetTextGeometry);
    LIB_FUNCTION("-4GCfYdNF1s", "libSceIme", 1, "libSceIme", 1, 1, sceImeUpdate);
    LIB_FUNCTION("oOwl47ouxoM", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshClearPreedit);
    LIB_FUNCTION("gtoTsGM9vEY", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshClose);
    LIB_FUNCTION("wTKF4mUlSew", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshConfirmPreedit);
    LIB_FUNCTION("rM-1hkuOhh0", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshDisableController);
    LIB_FUNCTION("42xMaQ+GLeQ", "libSceIme", 1, "libSceIme", 1, 1,
                 sceImeVshGetPanelPositionAndForm);
    LIB_FUNCTION("ZmmV6iukhyo", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshInformConfirmdString);
    LIB_FUNCTION("EQBusz6Uhp8", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshInformConfirmdString2);
    LIB_FUNCTION("LBicRa-hj3A", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshOpen);
    LIB_FUNCTION("-IAOwd2nO7g", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSendTextInfo);
    LIB_FUNCTION("qDagOjvJdNk", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetCaretGeometry);
    LIB_FUNCTION("tNOlmxee-Nk", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetCaretIndexInPreedit);
    LIB_FUNCTION("rASXozKkQ9g", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetPanelPosition);
    LIB_FUNCTION("idvMaIu5H+k", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetParam);
    LIB_FUNCTION("ga5GOgThbjo", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetPreeditGeometry);
    LIB_FUNCTION("RuSca8rS6yA", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetSelectGeometry);
    LIB_FUNCTION("J7COZrgSFRA", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshSetSelectionText);
    LIB_FUNCTION("WqAayyok5p0", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshUpdate);
    LIB_FUNCTION("O7Fdd+Oc-qQ", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshUpdateContext);
    LIB_FUNCTION("fwcPR7+7Rks", "libSceIme", 1, "libSceIme", 1, 1, sceImeVshUpdateContext2);
};

} // namespace Libraries::Ime