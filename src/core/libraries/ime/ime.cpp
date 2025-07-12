// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <queue>
#include "common/logging/log.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_error.h"
#include "core/libraries/ime/ime_ui.h"
#include "core/libraries/libs.h"
#include "core/tls.h"

namespace Libraries::Ime {

static std::queue<OrbisImeEvent> g_ime_events;
static ImeState g_ime_state{};
static ImeUi g_ime_ui;

class ImeHandler {
public:
    ImeHandler(const OrbisImeKeyboardParam* param) {
        Init(param, false);
    }
    ImeHandler(const OrbisImeParam* param) {
        Init(param, true);
    }
    ~ImeHandler() = default;

    void Init(const void* param, bool ime_mode) {
        if (ime_mode) {
            m_param.ime = *(OrbisImeParam*)param;
        } else {
            m_param.key = *(OrbisImeKeyboardParam*)param;
        }
        m_ime_mode = ime_mode;

        // Open an event to let the game know the IME has started
        OrbisImeEvent openEvent{};
        openEvent.id = (ime_mode ? OrbisImeEventId::Open : OrbisImeEventId::KeyboardOpen);

        if (ime_mode) {
            sceImeGetPanelSize(&m_param.ime, &openEvent.param.rect.width,
                               &openEvent.param.rect.height);
            openEvent.param.rect.x = m_param.ime.posx;
            openEvent.param.rect.y = m_param.ime.posy;
        } else {
            openEvent.param.resource_id_array.user_id = 1;
            openEvent.param.resource_id_array.resource_id[0] = 1;
        }

        // Are we supposed to call the event handler on init with
        // ADD_OSK?
        /* if (!ime_mode && False(m_param.key.option & OrbisImeKeyboardOption::AddOsk)) {
            Execute(nullptr, &openEvent, true);
        }*/

        if (ime_mode) {
            g_ime_state = ImeState(&m_param.ime);
            g_ime_ui = ImeUi(&g_ime_state, &m_param.ime);
        }
    }

    Error Update(OrbisImeEventHandler handler) {
        if (!m_ime_mode) {
            /* We don't handle any events for ImeKeyboard */
            return Error::OK;
        }

        std::unique_lock lock{g_ime_state.queue_mutex};

        while (!g_ime_state.event_queue.empty()) {
            OrbisImeEvent event = g_ime_state.event_queue.front();
            g_ime_state.event_queue.pop();
            Execute(handler, &event, false);
        }

        return Error::OK;
    }

    void Execute(OrbisImeEventHandler handler, OrbisImeEvent* event, bool use_param_handler) {
        if (m_ime_mode) {
            OrbisImeParam param = m_param.ime;
            if (use_param_handler) {
                Core::ExecuteGuest(param.handler, param.arg, event);
            } else {
                Core::ExecuteGuest(handler, param.arg, event);
            }
        } else {
            OrbisImeKeyboardParam param = m_param.key;
            if (use_param_handler) {
                Core::ExecuteGuest(param.handler, param.arg, event);
            } else {
                Core::ExecuteGuest(handler, param.arg, event);
            }
        }
    }

    Error SetText(const char16_t* text, u32 length) {
        g_ime_state.SetText(text, length);
        return Error::OK;
    }

    Error SetCaret(const OrbisImeCaret* caret) {
        g_ime_state.SetCaret(caret->index);
        return Error::OK;
    }

    bool IsIme() {
        return m_ime_mode;
    }

private:
    union ImeParam {
        OrbisImeKeyboardParam key;
        OrbisImeParam ime;
    } m_param{};
    bool m_ime_mode = false;
};

static std::unique_ptr<ImeHandler> g_ime_handler;
static std::unique_ptr<ImeHandler> g_keyboard_handler;

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

s32 PS4_SYSV_ABI sceImeClose() {
    LOG_INFO(Lib_Ime, "(STUBBED) called");

    if (!g_ime_handler) {
        return ORBIS_IME_ERROR_NOT_OPENED;
    }

    g_ime_handler.release();
    g_ime_ui = ImeUi();
    g_ime_state = ImeState();
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

Error PS4_SYSV_ABI sceImeGetPanelSize(const OrbisImeParam* param, u32* width, u32* height) {
    LOG_INFO(Lib_Ime, "called");

    if (!width || !height) {
        return Error::INVALID_ADDRESS;
    }

    switch (param->type) {
    case OrbisImeType::Default:
    case OrbisImeType::BasicLatin:
    case OrbisImeType::Url:
    case OrbisImeType::Mail:
        // We set our custom sizes, commented sizes are the original ones
        *width = 500;  // 793
        *height = 100; // 408
        break;
    case OrbisImeType::Number:
        *width = 370;
        *height = 402;
        break;
    }

    return Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardClose(s32 userId) {
    LOG_INFO(Lib_Ime, "(STUBBED) called");

    if (!g_keyboard_handler) {
        return Error::NOT_OPENED;
    }

    g_keyboard_handler.release();
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetInfo() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetResourceId() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeKeyboardOpen(s32 userId, const OrbisImeKeyboardParam* param) {
    LOG_INFO(Lib_Ime, "called");

    if (!param) {
        return Error::INVALID_ADDRESS;
    }
    if (!param->arg) {
        return Error::INVALID_ARG;
    }
    if (!param->handler) {
        return Error::INVALID_HANDLER;
    }

    if (g_keyboard_handler) {
        return Error::BUSY;
    }

    g_keyboard_handler = std::make_unique<ImeHandler>(param);
    return Error::OK;
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

Error PS4_SYSV_ABI sceImeOpen(const OrbisImeParam* param, const OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_Ime, "called");

    if (!param) {
        return Error::INVALID_ADDRESS;
    }
    if (g_ime_handler) {
        return Error::BUSY;
    }

    g_ime_handler = std::make_unique<ImeHandler>(param);
    return Error::OK;
}

int PS4_SYSV_ABI sceImeOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceImeParamInit(OrbisImeParam* param) {
    LOG_INFO(Lib_Ime, "called");

    if (!param) {
        return;
    }

    memset(param, 0, sizeof(OrbisImeParam));
    param->user_id = -1;
}

int PS4_SYSV_ABI sceImeSetCandidateIndex() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeSetCaret(const OrbisImeCaret* caret) {
    LOG_TRACE(Lib_Ime, "called");

    if (!g_ime_handler) {
        return Error::NOT_OPENED;
    }
    if (!caret) {
        return Error::INVALID_ADDRESS;
    }

    return g_ime_handler->SetCaret(caret);
}

Error PS4_SYSV_ABI sceImeSetText(const char16_t* text, u32 length) {
    LOG_TRACE(Lib_Ime, "called");

    if (!g_ime_handler) {
        return Error::NOT_OPENED;
    }
    if (!text) {
        return Error::INVALID_ADDRESS;
    }

    return g_ime_handler->SetText(text, length);
}

int PS4_SYSV_ABI sceImeSetTextGeometry() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeUpdate(OrbisImeEventHandler handler) {
    if (g_ime_handler) {
        g_ime_handler->Update(handler);
    }

    if (g_keyboard_handler) {
        g_keyboard_handler->Update(handler);
    }

    if (!g_ime_handler || !g_keyboard_handler) {
        return Error::NOT_OPENED;
    }

    return Error::OK;
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

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
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
