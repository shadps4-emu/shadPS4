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
        LOG_INFO(Lib_Ime, "Creating ImeHandler for keyboard");
        Init(param, false);
    }
    ImeHandler(const OrbisImeParam* param, const OrbisImeParamExtended* extended = nullptr) {
        LOG_INFO(Lib_Ime, "Creating ImeHandler for IME");
        if (extended) {
            m_extended = *extended;
            m_has_extended = true;
        }
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
            LOG_INFO(Lib_Ime, "calling sceImeGetPanelSize");
            Error e = sceImeGetPanelSize(&m_param.ime, &openEvent.param.rect.width,
                                         &openEvent.param.rect.height);
            if (e != Error::OK) {
                LOG_ERROR(Lib_Ime, "sceImeGetPanelSize returned 0x{:X}", static_cast<u32>(e));
            }

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
            g_ime_state = ImeState(&m_param.ime, m_has_extended ? &m_extended : nullptr);
            g_ime_ui = ImeUi(&g_ime_state, &m_param.ime, m_has_extended ? &m_extended : nullptr);
        }
    }

    Error Update(OrbisImeEventHandler handler) {
        if (!m_ime_mode) {
            /* We don't handle any events for ImeKeyboard */
            return Error::OK;
        }

        std::unique_lock<std::mutex> lock{g_ime_state.queue_mutex};

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

    OrbisImeParamExtended m_extended{};
    bool m_has_extended = false;
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

Error PS4_SYSV_ABI sceImeClose() {
    LOG_INFO(Lib_Ime, "called");

    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "No IME handler is open");
        return Error::NOT_OPENED;
    }

    g_ime_handler.release();
    if (g_ime_handler) {
        LOG_ERROR(Lib_Ime, "Failed to close IME handler, it is still open");
        return Error::INTERNAL;
    }
    g_ime_ui = ImeUi();
    g_ime_state = ImeState();

    LOG_INFO(Lib_Ime, "IME closed successfully");
    return Error::OK;
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
    LOG_INFO(Lib_Ime, "sceImeGetPanelSize called");

    if (!param) {
        LOG_ERROR(Lib_Ime, "Invalid param: NULL");
        return Error::INVALID_ADDRESS;
    }

    if (!width) {
        LOG_ERROR(Lib_Ime, "Invalid *width: NULL");
        return Error::INVALID_ADDRESS;
    }
    if (!height) {
        LOG_ERROR(Lib_Ime, "Invalid *height: NULL");
        return Error::INVALID_ADDRESS;
    }

    if (static_cast<u32>(param->option) & ~0x7BFF) { // Basic check for invalid options
        LOG_ERROR(Lib_Ime, "Invalid option: {:032b}", static_cast<u32>(param->option));
        return Error::INVALID_OPTION;
    }

    switch (param->type) {
    case OrbisImeType::Default:
        *width = 500;  // dummy value
        *height = 100; // dummy value
        LOG_DEBUG(Lib_Ime, "param->type: Default ({})", static_cast<u32>(param->type));
        break;
    case OrbisImeType::BasicLatin:
        *width = 500;  // dummy value
        *height = 100; // dummy value
        LOG_DEBUG(Lib_Ime, "param->type: BasicLatin ({})", static_cast<u32>(param->type));
        break;
    case OrbisImeType::Url:
        *width = 500;  // dummy value
        *height = 100; // dummy value
        LOG_DEBUG(Lib_Ime, "param->type: Url ({})", static_cast<u32>(param->type));
        break;
    case OrbisImeType::Mail:
        // We set our custom sizes, commented sizes are the original ones
        *width = 500;  // 793
        *height = 100; // 408
        LOG_DEBUG(Lib_Ime, "param->type: Mail ({})", static_cast<u32>(param->type));
        break;
    case OrbisImeType::Number:
        *width = 370;
        *height = 402;
        LOG_DEBUG(Lib_Ime, "param->type: Number ({})", static_cast<u32>(param->type));
        break;
    default:
        LOG_ERROR(Lib_Ime, "Invalid param->type: ({})", static_cast<u32>(param->type));
        return Error::INVALID_TYPE;
    }

    LOG_INFO(Lib_Ime, "IME panel size: width={}, height={}", *width, *height);
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardClose(Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_Ime, "called");

    if (!g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "No keyboard handler is open");
        return Error::NOT_OPENED;
    }
    // TODO: Check for valid user IDs. Disabled until user manager is ready.
    if ((userId < 0 || userId > 4) && false) {
        // Maybe g_keyboard_handler should hold a user ID and I must compare it here?
        LOG_ERROR(Lib_Ime, "Invalid userId: {}", userId);
        return Error::INVALID_USER_ID;
    }

    g_keyboard_handler.release();
    if (g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "failed to close keyboard handler, it is still open");
        return Error::INTERNAL;
    }

    LOG_INFO(Lib_Ime, "Keyboard handler closed successfully for user ID: {}", userId);
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetInfo() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}
Error PS4_SYSV_ABI
sceImeKeyboardGetResourceId(Libraries::UserService::OrbisUserServiceUserId userId,
                            OrbisImeKeyboardResourceIdArray* resourceIdArray) {
    LOG_INFO(Lib_Ime, "(partial) called");

    if (!resourceIdArray) {
        LOG_ERROR(Lib_Ime, "Invalid resourceIdArray: NULL");
        return Error::INVALID_ADDRESS;
    }

    // TODO: Check for valid user IDs. Disabled until user manager is ready.
    if ((userId < 0 || userId > 4) && false) {
        LOG_ERROR(Lib_Ime, "Invalid userId: {}", userId);
        resourceIdArray->user_id = userId;
        for (u32& id : resourceIdArray->resource_id) {
            id = 0;
        }
        return Error::INVALID_USER_ID;
    }

    if (!g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "Keyboard handler not opened");
        resourceIdArray->user_id = userId;
        for (u32& id : resourceIdArray->resource_id) {
            id = 0;
        }
        return Error::NOT_OPENED;
    }

    // Simulate "no USB keyboard connected", needed for some Unity engine games
    resourceIdArray->user_id = userId;
    for (u32& id : resourceIdArray->resource_id) {
        id = 0;
    }
    LOG_INFO(Lib_Ime, "No USB keyboard connected (simulated)");
    return Error::CONNECTION_FAILED;

    // For future reference, if we had a real keyboard handler
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardOpen(Libraries::UserService::OrbisUserServiceUserId userId,
                                      const OrbisImeKeyboardParam* param) {
    LOG_INFO(Lib_Ime, "called");
    if (!param) {
        LOG_ERROR(Lib_Ime, "Invalid param: NULL");
        return Error::INVALID_ADDRESS;
    }
    if (!param->handler) {
        LOG_ERROR(Lib_Ime, "Invalid param->handler: NULL");
        return Error::INVALID_HANDLER;
    }
    // seems like arg is optional, need to check if it is used in the handler
    // Todo: check if arg is used in the handler, temporarily disabled
    if (!param->arg && false) {
        LOG_ERROR(Lib_Ime, "Invalid param->arg: NULL");
        return Error::INVALID_ARG;
    }
    if (static_cast<u32>(param->option) & ~kValidOrbisImeKeyboardOptionMask) {
        LOG_ERROR(Lib_Ime,
                  "Invalid param->option\n"
                  "option:    {:032b}\n"
                  "validMask: {:032b}",
                  static_cast<u32>(param->option), kValidOrbisImeKeyboardOptionMask);
        return Error::INVALID_OPTION;
    }

    // TODO: Check for valid user IDs. Disabled until user manager is ready.
    if ((userId < 0 || userId > 4) && false) {
        LOG_ERROR(Lib_Ime, "Invalid userId: {}", userId);
        return Error::INVALID_USER_ID;
    }
    for (size_t i = 0; i < sizeof(param->reserved1); ++i) {
        if (param->reserved1[i] != 0) {
            LOG_ERROR(Lib_Ime, "Invalid reserved1: not zeroed");
            return Error::INVALID_RESERVED;
        }
    }
    for (size_t i = 0; i < sizeof(param->reserved2); ++i) {
        if (param->reserved2[i] != 0) {
            LOG_ERROR(Lib_Ime, "Invalid reserved2: not zeroed");
            return Error::INVALID_RESERVED;
        }
    }

    // Todo: figure out what it is, always false for now
    if (false) {
        LOG_ERROR(Lib_Ime, "USB keyboard some special kind of failure");
        return Error::CONNECTION_FAILED;
    }
    if (g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "Keyboard handler is already open");
        return Error::BUSY;
    }
    g_keyboard_handler = std::make_unique<ImeHandler>(param);
    if (!g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "Failed to create keyboard handler");
        return Error::INTERNAL; // or Error::NO_MEMORY;
    }
    LOG_INFO(Lib_Ime, "Keyboard handler created successfully for user ID: {}", userId);
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
        LOG_ERROR(Lib_Ime, "Invalid param: NULL");
        return Error::INVALID_ADDRESS;
    } else {
        // LOG_DEBUG values for debugging purposes
        LOG_DEBUG(Lib_Ime, "param->user_id: {}", param->user_id);
        LOG_DEBUG(Lib_Ime, "param->type: {}", static_cast<u32>(param->type));
        LOG_DEBUG(Lib_Ime, "param->supported_languages: {:064b}",
                  static_cast<u64>(param->supported_languages));
        LOG_DEBUG(Lib_Ime, "param->enter_label: {}", static_cast<u32>(param->enter_label));
        LOG_DEBUG(Lib_Ime, "param->input_method: {}", static_cast<u32>(param->input_method));
        LOG_DEBUG(Lib_Ime, "param->filter: {:p}", reinterpret_cast<void*>(param->filter));
        LOG_DEBUG(Lib_Ime, "param->option: {:032b}", static_cast<u32>(param->option));
        LOG_DEBUG(Lib_Ime, "param->maxTextLength: {}", param->maxTextLength);
        LOG_DEBUG(Lib_Ime, "param->inputTextBuffer: {:p}",
                  static_cast<const void*>(param->inputTextBuffer));
        LOG_DEBUG(Lib_Ime, "param->posx: {}", param->posx);
        LOG_DEBUG(Lib_Ime, "param->posy: {}", param->posy);
        LOG_DEBUG(Lib_Ime, "param->horizontal_alignment: {}",
                  static_cast<u32>(param->horizontal_alignment));
        LOG_DEBUG(Lib_Ime, "param->vertical_alignment: {}",
                  static_cast<u32>(param->vertical_alignment));
        LOG_DEBUG(Lib_Ime, "param->work: {:p}", param->work);
        LOG_DEBUG(Lib_Ime, "param->arg: {:p}", param->arg);
        LOG_DEBUG(Lib_Ime, "param->handler: {:p}", reinterpret_cast<void*>(param->handler));
    }

    if (!extended) {
        LOG_INFO(Lib_Ime, "Not used extended: NULL");
    } else {
        LOG_DEBUG(Lib_Ime, "extended->option: {:032b}", static_cast<u32>(extended->option));
        LOG_DEBUG(Lib_Ime, "extended->color_base: {{{},{},{},{}}}", extended->color_base.r,
                  extended->color_base.g, extended->color_base.b, extended->color_base.a);
        LOG_DEBUG(Lib_Ime, "extended->color_line: {{{},{},{},{}}}", extended->color_line.r,
                  extended->color_line.g, extended->color_line.b, extended->color_line.a);
        LOG_DEBUG(Lib_Ime, "extended->color_text_field: {{{},{},{},{}}}",
                  extended->color_text_field.r, extended->color_text_field.g,
                  extended->color_text_field.b, extended->color_text_field.a);
        LOG_DEBUG(Lib_Ime, "extended->color_preedit: {{{},{},{},{}}}", extended->color_preedit.r,
                  extended->color_preedit.g, extended->color_preedit.b, extended->color_preedit.a);
        LOG_DEBUG(Lib_Ime, "extended->color_button_default: {{{},{},{},{}}}",
                  extended->color_button_default.r, extended->color_button_default.g,
                  extended->color_button_default.b, extended->color_button_default.a);
        LOG_DEBUG(Lib_Ime, "extended->color_button_function: {{{},{},{},{}}}",
                  extended->color_button_function.r, extended->color_button_function.g,
                  extended->color_button_function.b, extended->color_button_function.a);
        LOG_DEBUG(Lib_Ime, "extended->color_button_symbol: {{{},{},{},{}}}",
                  extended->color_button_symbol.r, extended->color_button_symbol.g,
                  extended->color_button_symbol.b, extended->color_button_symbol.a);
        LOG_DEBUG(Lib_Ime, "extended->color_text: {{{},{},{},{}}}", extended->color_text.r,
                  extended->color_text.g, extended->color_text.b, extended->color_text.a);
        LOG_DEBUG(Lib_Ime, "extended->color_special: {{{},{},{},{}}}", extended->color_special.r,
                  extended->color_special.g, extended->color_special.b, extended->color_special.a);
        LOG_DEBUG(Lib_Ime, "extended->priority: {}", static_cast<u32>(extended->priority));
        LOG_DEBUG(Lib_Ime, "extended->additional_dictionary_path: {:p}",
                  static_cast<const void*>(extended->additional_dictionary_path));
        LOG_DEBUG(Lib_Ime, "extended->ext_keyboard_filter: {:p}",
                  reinterpret_cast<void*>(extended->ext_keyboard_filter));
        LOG_DEBUG(Lib_Ime, "extended->disable_device: {:032b}",
                  static_cast<u32>(extended->disable_device));
        LOG_DEBUG(Lib_Ime, "extended->ext_keyboard_mode: {}", extended->ext_keyboard_mode);
    }

    if (param->user_id < 1 || param->user_id > 4) { // Todo: check valid user IDs
        LOG_ERROR(Lib_Ime, "Invalid user_id: {}", static_cast<u32>(param->user_id));
        return Error::INVALID_USER_ID;
    }

    if (!magic_enum::enum_contains(param->type)) {
        LOG_ERROR(Lib_Ime, "Invalid type: {}", static_cast<u32>(param->type));
        return Error::INVALID_TYPE;
    }

    if (static_cast<u64>(param->supported_languages) & ~kValidOrbisImeLanguageMask) {
        LOG_ERROR(Lib_Ime,
                  "Invalid supported_languages\n"
                  "supported_languages: {:064b}\n"
                  "valid_mask:          {:064b}",
                  static_cast<u64>(param->supported_languages), kValidOrbisImeLanguageMask);
        return Error::INVALID_SUPPORTED_LANGUAGES;
    }

    if (!magic_enum::enum_contains(param->enter_label)) {
        LOG_ERROR(Lib_Ime, "Invalid enter_label: {}", static_cast<u32>(param->enter_label));
        return Error::INVALID_ENTER_LABEL;
    }

    if (!magic_enum::enum_contains(param->input_method)) {
        LOG_ERROR(Lib_Ime, "Invalid input_method: {}", static_cast<u32>(param->input_method));
        return Error::INVALID_INPUT_METHOD;
    }

    if (static_cast<u32>(param->option) & ~kValidImeOptionMask) {
        LOG_ERROR(Lib_Ime, "option has invalid bits set (0x{:X}), mask=(0x{:X})",
                  static_cast<u32>(param->option), kValidImeOptionMask);
        return Error::INVALID_OPTION;
    }

    if (param->maxTextLength == 0 || param->maxTextLength > ORBIS_IME_DIALOG_MAX_TEXT_LENGTH) {
        LOG_ERROR(Lib_Ime, "Invalid maxTextLength: {}", param->maxTextLength);
        return Error::INVALID_MAX_TEXT_LENGTH;
    }

    if (!param->inputTextBuffer) {
        LOG_ERROR(Lib_Ime, "Invalid inputTextBuffer: NULL");
        return Error::INVALID_INPUT_TEXT_BUFFER;
    }

    bool useHighRes = True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES);
    const float maxWidth = useHighRes ? 3840.0f : 1920.0f;
    const float maxHeight = useHighRes ? 2160.0f : 1080.0f;

    if (param->posx < 0.0f || param->posx >= maxWidth) {
        LOG_ERROR(Lib_Ime, "Invalid posx: {}, range: 0.0 - {}", param->posx, maxWidth);
        return Error::INVALID_POSX;
    }
    if (param->posy < 0.0f || param->posy >= maxHeight) {
        LOG_ERROR(Lib_Ime, "Invalid posy: {}, range: 0.0 - {}", param->posy, maxHeight);
        return Error::INVALID_POSY;
    }

    if (!magic_enum::enum_contains(param->horizontal_alignment)) {
        LOG_ERROR(Lib_Ime, "Invalid horizontal_alignment: {}",
                  static_cast<u32>(param->horizontal_alignment));
        return Error::INVALID_HORIZONTALIGNMENT;
    }
    if (!magic_enum::enum_contains(param->vertical_alignment)) {
        LOG_ERROR(Lib_Ime, "Invalid vertical_alignment: {}",
                  static_cast<u32>(param->vertical_alignment));
        return Error::INVALID_VERTICALALIGNMENT;
    }

    if (extended) {
        u32 ext_option_value = static_cast<u32>(extended->option);
        if (ext_option_value & ~kValidImeExtOptionMask) {
            LOG_ERROR(Lib_Ime,
                      "Invalid extended->option\n"
                      "option: {:032b}\n"
                      "valid_mask: {:032b}",
                      ext_option_value, kValidImeExtOptionMask);
            return Error::INVALID_EXTENDED;
        }
    }

    if (!param->work) {
        LOG_ERROR(Lib_Ime, "Invalid work: NULL");
        return Error::INVALID_WORK;
    }

    // Todo: validate arg
    if (false) {
        LOG_ERROR(Lib_Ime, "Invalid arg: NULL");
        return Error::INVALID_ARG;
    }

    // Todo: validate handler
    if (false) {
        LOG_ERROR(Lib_Ime, "Invalid handler: NULL");
        return Error::INVALID_HANDLER;
    }

    for (size_t i = 0; i < sizeof(param->reserved); ++i) {
        if (param->reserved[i] != 0) {
            LOG_ERROR(Lib_Ime, "Invalid reserved: not zeroed");
            return Error::INVALID_RESERVED;
        }
    }

    if (g_ime_handler) {
        LOG_ERROR(Lib_Ime, "IME handler is already open");
        return Error::BUSY;
    }

    g_ime_handler = std::make_unique<ImeHandler>(param, extended);
    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "Failed to create IME handler");
        return Error::NO_MEMORY; // or Error::INTERNAL
    }

    LOG_INFO(Lib_Ime, "IME handler created successfully");
    return Error::OK;
}

int PS4_SYSV_ABI sceImeOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceImeParamInit(OrbisImeParam* param) {
    LOG_INFO(Lib_Ime, "sceImeParamInit called");

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
