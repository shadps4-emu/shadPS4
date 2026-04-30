// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <queue>
#include "common/logging/log.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_error.h"
#include "core/libraries/ime/ime_ui.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/tls.h"

namespace Libraries::Ime {

static std::queue<OrbisImeEvent> g_ime_events;
static std::unique_ptr<ImeState> g_ime_state;
static std::unique_ptr<ImeUi> g_ime_ui;

namespace {

u32 GetCompiledSdkVersion() {
    s32 ver = 0;
    if (Libraries::Kernel::sceKernelGetCompiledSdkVersion(&ver) != ORBIS_OK) {
        return 0;
    }
    return static_cast<u32>(ver);
}

u32 GetImeOptionMask() {
    const u32 sdk = GetCompiledSdkVersion();
    u32 mask = static_cast<u32>(sdk > 0x14fffff) << 8;
    u32 tmp = mask | 0x70ffU;
    if (sdk > 0x174ffff) {
        tmp = mask | 0x78ffU;
    }
    mask = tmp & 0x69ffU;
    if (sdk > 0x2ffffff) {
        mask = tmp;
    }
    tmp = mask & 0x59ffU;
    if (sdk > 0x34fffff) {
        tmp = mask;
    }
    mask = tmp & 0x39ffU;
    if (sdk > 0x3ffffff) {
        mask = tmp;
    }
    return mask;
}

bool HasInvalidImeOption(u32 option) {
    return (((GetImeOptionMask() ^ 0xfffffdffU) & option) != 0);
}

u64 GetImeLanguageMask() {
    const u32 sdk = GetCompiledSdkVersion();
    u64 mask = (sdk > 0x1ffffff ? 0x1000000ULL : 0ULL) + 0x3fe1fffffULL;
    u64 tmp = mask & 0x3fd1fffffULL;
    if (sdk > 0x24fffff) {
        tmp = mask;
    }
    mask = tmp & 0x2031fffffULL;
    if (sdk > 0x4ffffff) {
        mask = tmp;
    }
    tmp = mask & 0x1ff1fffffULL;
    if (sdk > 0xfffffff) {
        tmp = mask;
    }
    return tmp;
}

bool IsValidImeExtOption(u32 option) {
    const u32 sdk = GetCompiledSdkVersion();
    u32 allow = (sdk < 0x1560000) ? 0x41dfU : 0x4fdfU;
    if ((option & 0x4080U) == 0x4000U) {
        return false;
    }
    if (sdk <= 0x5ffffff) {
        allow &= 0x0fdfU;
    }
    return ((~allow & option) == 0);
}

bool IsValidImeUserId(Libraries::UserService::OrbisUserServiceUserId user_id) {
    const u32 sdk = GetCompiledSdkVersion();
    const u32 uid_u = static_cast<u32>(user_id);

    if (sdk < 0x1500000) {
        if ((uid_u + 1U) < 2U || (uid_u - 0xfeU) < 2U) {
            return true;
        }
        return user_id >= 0;
    }

    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return false;
    }
    if (user_id == 0xfe) {
        return true;
    }
    return user_id >= 0;
}

u32 CountUtf16Text(const char16_t* text, u32 max_len) {
    if (!text) {
        return 0;
    }
    u32 len = 0;
    while (len < max_len && text[len] != u'\0') {
        ++len;
    }
    return len;
}

bool IsValidImeText(const char16_t* text, u32 length, bool multiline, u32 sdk) {
    if (!text) {
        return false;
    }
    const bool allow_new_line = (sdk >= 0x1560000) && multiline;

    for (u32 i = 0; i < length; ++i) {
        const char16_t ch = text[i];
        if (!allow_new_line && (ch == u'\n' || ch == u'\r')) {
            return false;
        }
        if ((static_cast<u16>(ch) & 0xf800U) == 0xd800U) {
            return false;
        }
        if (ch == u'\0') {
            break;
        }
    }
    return true;
}

bool HasInvalidWorkOverlap(const OrbisImeParam* param) {
    if (!param || !param->inputTextBuffer || !param->work) {
        return false;
    }

    const auto input_addr = reinterpret_cast<uintptr_t>(param->inputTextBuffer);
    const auto work_addr = reinterpret_cast<uintptr_t>(param->work);
    const bool expanded = True(param->option & OrbisImeOption::EXPANDED_PREEDIT_BUFFER);
    const u32 scratch_chars = param->maxTextLength + (expanded ? 0x79U : 0x1fU);

    if (input_addr <= work_addr) {
        const uintptr_t input_end =
            input_addr + static_cast<uintptr_t>(scratch_chars) * sizeof(char16_t);
        if (work_addr < input_end) {
            return true;
        }
    }

    if (work_addr <= input_addr && input_addr <= (work_addr + 0x4fffU)) {
        return true;
    }

    return false;
}

} // namespace

class ImeHandler {
public:
    ImeHandler(const OrbisImeKeyboardParam* param) {
        Init(param, nullptr, false);
    }
    ImeHandler(const OrbisImeParam* param, const OrbisImeParamExtended* extended = nullptr) {

        LOG_DEBUG(Lib_Ime, "param->work: 0x{:X}",
                  static_cast<u64>(reinterpret_cast<uintptr_t>(param->work)));
        LOG_DEBUG(Lib_Ime, "param->inputTextBuffer: 0x{:X}",
                  static_cast<u64>(reinterpret_cast<uintptr_t>(param->arg)));
        Init(param, extended, true);
    }
    ~ImeHandler() = default;

    void Init(const void* param, const OrbisImeParamExtended* extended, bool ime_mode) {
        if (ime_mode) {
            m_param.ime = *(OrbisImeParam*)param;
            LOG_DEBUG(Lib_Ime, "m_param.ime.work: 0x{:X}",
                      static_cast<u64>(reinterpret_cast<uintptr_t>(m_param.ime.work)));
            LOG_DEBUG(Lib_Ime, "m_param.ime.inputTextBuffer: 0x{:X}",
                      static_cast<u64>(reinterpret_cast<uintptr_t>(m_param.ime.inputTextBuffer)));
            if (extended)
                m_param.ime_ext = *extended;
            else
                m_param.ime_ext = {};
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
            g_ime_state = std::make_unique<ImeState>(&m_param.ime, &m_param.ime_ext);
            g_ime_ui = std::make_unique<ImeUi>(g_ime_state.get(), &m_param.ime, &m_param.ime_ext);

            // Queue the Open event so it is delivered on next sceImeUpdate
            LOG_DEBUG(Lib_Ime, "IME Event queued: Open rect x={}, y={}, w={}, h={}",
                      openEvent.param.rect.x, openEvent.param.rect.y, openEvent.param.rect.width,
                      openEvent.param.rect.height);
            g_ime_state->SendEvent(&openEvent);
        }
    }

    Error Update(OrbisImeEventHandler handler) {
        if (!m_ime_mode) {
            /* We don't handle any events for ImeKeyboard */
            return Error::OK;
        }
        if (!g_ime_state) {
            LOG_ERROR(Lib_Ime, "ImeHandler::Update called without active IME state");
            return Error::INTERNAL;
        }

        std::unique_lock<std::mutex> lock{g_ime_state->queue_mutex};

        while (!g_ime_state->event_queue.empty()) {
            OrbisImeEvent event = g_ime_state->event_queue.front();
            g_ime_state->event_queue.pop();
            Execute(handler, &event, false);
        }

        return Error::OK;
    }

    void Execute(OrbisImeEventHandler handler, OrbisImeEvent* event, bool use_param_handler) {
        if (m_ime_mode) {
            OrbisImeParam param = m_param.ime;
            const OrbisImeEventHandler callback = use_param_handler ? param.handler : handler;
            if (!callback) {
                LOG_ERROR(Lib_Ime, "ImeHandler::Execute called with null IME callback");
                return;
            }
            callback(param.arg, event);
        } else {
            OrbisImeKeyboardParam param = m_param.key;
            const OrbisImeEventHandler callback = use_param_handler ? param.handler : handler;
            if (!callback) {
                LOG_ERROR(Lib_Ime, "ImeHandler::Execute called with null keyboard callback");
                return;
            }
            callback(param.arg, event);
        }
    }

    Error SetText(const char16_t* text, u32 length) {
        if (!text) {
            LOG_WARNING(Lib_Ime, "ImeHandler::SetText received null text pointer");
            return Error::INVALID_ADDRESS;
        }
        if (!g_ime_state) {
            LOG_ERROR(Lib_Ime, "ImeHandler::SetText called without active IME state");
            return Error::INTERNAL;
        }
        g_ime_state->SetText(text, length);
        return Error::OK;
    }

    Error SetCaret(const OrbisImeCaret* caret) {
        if (!caret) {
            LOG_WARNING(Lib_Ime, "ImeHandler::SetCaret received null caret pointer");
            return Error::INVALID_ADDRESS;
        }
        if (!g_ime_state) {
            LOG_ERROR(Lib_Ime, "ImeHandler::SetCaret called without active IME state");
            return Error::INTERNAL;
        }
        g_ime_state->SetCaret(caret->index);
        return Error::OK;
    }

    bool IsIme() {
        return m_ime_mode;
    }

    OrbisImeEventHandler GetHandler() const {
        return m_ime_mode ? m_param.ime.handler : m_param.key.handler;
    }

    u32 GetImeOptionBits() const {
        return m_ime_mode ? static_cast<u32>(m_param.ime.option) : 0U;
    }

    u32 GetImeMaxTextLength() const {
        return m_ime_mode ? m_param.ime.maxTextLength : 0U;
    }

    u32 GetImeCurrentTextLength() const {
        if (!m_ime_mode) {
            return 0U;
        }
        return CountUtf16Text(m_param.ime.inputTextBuffer, m_param.ime.maxTextLength);
    }

private:
    struct ImeParam {
        OrbisImeKeyboardParam key;
        OrbisImeParam ime;
        OrbisImeParamExtended ime_ext;
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

Error PS4_SYSV_ABI sceImeClose() {
    LOG_DEBUG(Lib_Ime, "called");

    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "No IME handler is open");
        return Error::NOT_OPENED;
    }

    g_ime_handler.reset();
    if (g_ime_handler) {
        LOG_ERROR(Lib_Ime, "Failed to close IME handler, it is still open");
        return Error::INTERNAL;
    }
    g_ime_ui = std::make_unique<ImeUi>();
    g_ime_state = std::make_unique<ImeState>();

    LOG_DEBUG(Lib_Ime, "IME closed successfully");
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

int PS4_SYSV_ABI sceImeConfirmCandidate(s32 index) {
    (void)index;
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

int PS4_SYSV_ABI sceImeGetPanelPositionAndForm(OrbisImePositionAndForm* posForm) {
    if (!posForm) {
        return ORBIS_OK;
    }
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeGetPanelSize(const OrbisImeParam* param, u32* width, u32* height) {
    if (!param || !width || !height) {
        return Error::INVALID_ADDRESS;
    }
    if (static_cast<u32>(param->type) > static_cast<u32>(OrbisImeType::Number)) {
        return Error::INVALID_TYPE;
    }

    const u32 option = static_cast<u32>(param->option);
    if (HasInvalidImeOption(option)) {
        return Error::INVALID_OPTION;
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (param->type == OrbisImeType::Number) {
        *width = 0x172U;
        *height = (sdk > 0x16fffffU) ? 0x192U : 0x170U;
    } else if (param->type == OrbisImeType::BasicLatin || (option & 0xc0000004U) == 4U) {
        *width = 0x319U;
        *height = (sdk > 0x16fffffU) ? 0x198U : 0x170U;
    } else {
        *width = 0x319U;
        *height = 0x198U;
    }

    if ((option & static_cast<u32>(OrbisImeOption::USE_OVER_2K_COORDINATES)) != 0) {
        *width <<= 1;
        *height <<= 1;
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardClose(Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_Ime, "called");

    if (!g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "No keyboard handler is open");
        return Error::NOT_OPENED;
    }
    // TODO: Check for valid user IDs.
    if (userId == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        // Maybe g_keyboard_handler should hold a user ID and I must compare it here?
        LOG_ERROR(Lib_Ime, "Invalid userId: {}", userId);
        return Error::INVALID_USER_ID;
    }

    g_keyboard_handler.reset();
    if (g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "failed to close keyboard handler, it is still open");
        return Error::INTERNAL;
    }

    LOG_DEBUG(Lib_Ime, "Keyboard handler closed successfully for user ID: {}", userId);
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetInfo(u32 resourceId, OrbisImeKeyboardInfo* info) {
    (void)resourceId;
    if (info) {
        *info = {};
    }
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

    // TODO: Check for valid user IDs.
    if (userId == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        LOG_ERROR(Lib_Ime, "Invalid userId: {}", userId);
        /*
        resourceIdArray->user_id = userId;
        for (u32& id : resourceIdArray->resource_id) {
            id = 0;
        }
        */
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
    LOG_DEBUG(Lib_Ime, "No USB keyboard connected (simulated)");
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

    LOG_DEBUG(Lib_Ime, "  userId: {}", static_cast<u32>(userId));
    LOG_DEBUG(Lib_Ime, "  param->option: {:032b}", static_cast<u32>(param->option));
    LOG_DEBUG(Lib_Ime, "  param->arg: {}", param->arg);
    LOG_DEBUG(Lib_Ime, "  param->handler: 0x{:X}",
              static_cast<u64>(reinterpret_cast<uintptr_t>(param->handler)));

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

    // TODO: Check for valid user IDs.
    if (userId == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
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
    LOG_DEBUG(Lib_Ime, "Keyboard handler created successfully for user ID: {}", userId);
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardSetMode(Libraries::UserService::OrbisUserServiceUserId userId,
                                       u32 mode) {
    (void)userId;
    (void)mode;
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeKeyboardUpdate() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeOpen(const OrbisImeParam* param, const OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_Ime, "called");

    if (g_ime_handler) {
        LOG_ERROR(Lib_Ime, "IME handler is already open");
        return Error::BUSY;
    }

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
        LOG_DEBUG(Lib_Ime, "  param->handler: 0x{:X}",
                  static_cast<u64>(reinterpret_cast<uintptr_t>(param->handler)));
    }

    if (!extended) {
        LOG_DEBUG(Lib_Ime, "Not used extended: NULL");
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

    if (!IsValidImeUserId(param->user_id)) {
        LOG_ERROR(Lib_Ime, "Invalid user_id: {}", static_cast<u32>(param->user_id));
        return Error::INVALID_USER_ID;
    }

    if (static_cast<u32>(param->type) > static_cast<u32>(OrbisImeType::Number)) {
        LOG_ERROR(Lib_Ime, "Invalid type: {}", static_cast<u32>(param->type));
        return Error::INVALID_TYPE;
    }

    const u64 lang_mask = GetImeLanguageMask();
    if ((~lang_mask & static_cast<u64>(param->supported_languages)) != 0) {
        LOG_ERROR(Lib_Ime,
                  "Invalid supported_languages\n"
                  "supported_languages: {:064b}\n"
                  "valid_mask:          {:064b}",
                  static_cast<u64>(param->supported_languages), lang_mask);
        return Error::INVALID_SUPPORTED_LANGUAGES;
    }

    if (static_cast<u32>(param->enter_label) > static_cast<u32>(OrbisImeEnterLabel::Go)) {
        LOG_ERROR(Lib_Ime, "Invalid enter_label: {}", static_cast<u32>(param->enter_label));
        return Error::INVALID_ENTER_LABEL;
    }

    if (param->input_method != OrbisImeInputMethod::Default) {
        LOG_ERROR(Lib_Ime, "Invalid input_method: {}", static_cast<u32>(param->input_method));
        return Error::INVALID_INPUT_METHOD;
    }

    const u32 option = static_cast<u32>(param->option);
    if (HasInvalidImeOption(option)) {
        LOG_ERROR(Lib_Ime, "option has invalid bits set (0x{:X}), mask=(0x{:X})", option,
                  GetImeOptionMask());
        return Error::INVALID_OPTION;
    }

    const bool multiline = True(param->option & OrbisImeOption::MULTILINE);
    const bool password = True(param->option & OrbisImeOption::PASSWORD);
    if (multiline && password) {
        LOG_ERROR(Lib_Ime, "Invalid option combination: MULTILINE + PASSWORD");
        return Error::INVALID_PARAM;
    }
    if (multiline && param->type != OrbisImeType::Default &&
        param->type != OrbisImeType::BasicLatin) {
        LOG_ERROR(Lib_Ime, "MULTILINE requires type Default or BasicLatin, got {}",
                  static_cast<u32>(param->type));
        return Error::INVALID_PARAM;
    }
    if (password && param->type != OrbisImeType::BasicLatin &&
        param->type != OrbisImeType::Number) {
        LOG_ERROR(Lib_Ime, "PASSWORD requires type BasicLatin or Number, got {}",
                  static_cast<u32>(param->type));
        return Error::INVALID_PARAM;
    }

    if (param->maxTextLength == 0 || param->maxTextLength > ORBIS_IME_DIALOG_MAX_TEXT_LENGTH) {
        LOG_ERROR(Lib_Ime, "Invalid maxTextLength: {}", param->maxTextLength);
        return Error::INVALID_MAX_TEXT_LENGTH;
    }

    if (!param->inputTextBuffer) {
        LOG_ERROR(Lib_Ime, "Invalid inputTextBuffer: NULL");
        return Error::INVALID_INPUT_TEXT_BUFFER;
    }

    const u32 sdk = GetCompiledSdkVersion();
    float maxWidth = 1920.0f;
    float maxHeight = 1080.0f;
    if (sdk >= 0x1500000) {
        const bool use_high_res = True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES);
        maxWidth = use_high_res ? 3840.0f : 1920.0f;
        maxHeight = use_high_res ? 2160.0f : 1080.0f;
    }

    if (param->posx < 0.0f || param->posx >= maxWidth) {
        LOG_ERROR(Lib_Ime, "Invalid posx: {}, range: 0.0 - {}", param->posx, maxWidth);
        return Error::INVALID_POSX;
    }
    if (param->posy < 0.0f || param->posy >= maxHeight) {
        LOG_ERROR(Lib_Ime, "Invalid posy: {}, range: 0.0 - {}", param->posy, maxHeight);
        return Error::INVALID_POSY;
    }

    if (static_cast<u32>(param->horizontal_alignment) > 2U) {
        LOG_ERROR(Lib_Ime, "Invalid horizontal_alignment: {}",
                  static_cast<u32>(param->horizontal_alignment));
        return Error::INVALID_HORIZONTALIGNMENT;
    }
    if (static_cast<u32>(param->vertical_alignment) > 2U) {
        LOG_ERROR(Lib_Ime, "Invalid vertical_alignment: {}",
                  static_cast<u32>(param->vertical_alignment));
        return Error::INVALID_VERTICALALIGNMENT;
    }

    if (extended) {
        if (static_cast<u32>(extended->priority) >
            static_cast<u32>(OrbisImePanelPriority::Accent)) {
            LOG_ERROR(Lib_Ime, "Invalid extended->priority: {}",
                      static_cast<u32>(extended->priority));
            return Error::INVALID_EXTENDED;
        }

        const u32 ext_option_value = static_cast<u32>(extended->option);
        if (!IsValidImeExtOption(ext_option_value)) {
            LOG_ERROR(Lib_Ime,
                      "Invalid extended->option\n"
                      "option: {:032b}\n"
                      "sdk: 0x{:X}",
                      ext_option_value, sdk);
            return Error::INVALID_EXTENDED;
        }

        if ((extended->ext_keyboard_mode & 0xe3fffffcU) != 0) {
            LOG_ERROR(Lib_Ime, "Invalid extended->ext_keyboard_mode: 0x{:X}",
                      extended->ext_keyboard_mode);
            return Error::INVALID_EXTENDED;
        }

        for (size_t i = 0; i < sizeof(extended->reserved); ++i) {
            if (extended->reserved[i] != 0) {
                LOG_ERROR(Lib_Ime, "Invalid extended->reserved: not zeroed");
                return Error::INVALID_EXTENDED;
            }
        }

        const u32 disable_device = static_cast<u32>(extended->disable_device);
        if (sdk < 0x1560000) {
            if (extended->ext_keyboard_filter != nullptr || disable_device != 0 ||
                extended->ext_keyboard_mode != 0) {
                LOG_ERROR(Lib_Ime, "Invalid extended fields for SDK < 0x1560000");
                return Error::INVALID_EXTENDED;
            }
        } else if (disable_device > 7U) {
            LOG_ERROR(Lib_Ime, "Invalid extended->disable_device: {}", disable_device);
            return Error::INVALID_EXTENDED;
        }
    }

    if (!param->work) {
        LOG_ERROR(Lib_Ime, "Invalid work: NULL");
        return Error::INVALID_WORK;
    }
    if ((reinterpret_cast<uintptr_t>(param->work) & 0x3U) != 0) {
        LOG_ERROR(Lib_Ime, "Invalid work alignment: {:p}", param->work);
        return Error::INVALID_WORK;
    }

    if (!param->handler) {
        LOG_ERROR(Lib_Ime, "Invalid handler: NULL");
        return Error::INVALID_HANDLER;
    }

    for (size_t i = 0; i < sizeof(param->reserved); ++i) {
        if (param->reserved[i] != 0) {
            LOG_ERROR(Lib_Ime, "Invalid reserved: not zeroed");
            return Error::INVALID_RESERVED;
        }
    }

    if (HasInvalidWorkOverlap(param)) {
        LOG_ERROR(Lib_Ime, "Invalid overlap between inputTextBuffer and work");
        return Error::INVALID_PARAM;
    }

    if (!IsValidImeText(param->inputTextBuffer, param->maxTextLength, multiline, sdk)) {
        LOG_ERROR(Lib_Ime, "Invalid initial inputTextBuffer content");
        return Error::INVALID_TEXT;
    }

    std::memset(param->work, 0, 0x5000);

    if (extended) {
        g_ime_handler = std::make_unique<ImeHandler>(param, extended);
    } else {
        g_ime_handler = std::make_unique<ImeHandler>(param);
    }
    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "Failed to create IME handler");
        return Error::NO_MEMORY; // or Error::INTERNAL
    }

    LOG_DEBUG(Lib_Ime, "IME handler created successfully");
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
    param->user_id = Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
}

int PS4_SYSV_ABI sceImeSetCandidateIndex(s32 index) {
    (void)index;
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeSetCaret(const OrbisImeCaret* caret) {
    LOG_TRACE(Lib_Ime, "called");

    if (!g_ime_handler) {
        return Error::NOT_OPENED;
    }
    const u32 sdk = GetCompiledSdkVersion();
    if (!caret) {
        return (sdk < 0x1500000U) ? Error::INVALID_PARAM : Error::INVALID_ADDRESS;
    }
    if (caret->index > g_ime_handler->GetImeCurrentTextLength()) {
        return Error::INVALID_PARAM;
    }

    return g_ime_handler->SetCaret(caret);
}

Error PS4_SYSV_ABI sceImeSetText(const char16_t* text, u32 length) {
    LOG_TRACE(Lib_Ime, "called");

    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "IME handler not opened");
        return Error::NOT_OPENED;
    }
    if (!text) {
        LOG_ERROR(Lib_Ime, "Invalid text pointer: NULL");
        return Error::INVALID_ADDRESS;
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (sdk < 0x1560000U && length == 0) {
        return Error::INVALID_PARAM;
    }
    if (length != 0) {
        const bool multiline =
            (g_ime_handler->GetImeOptionBits() & static_cast<u32>(OrbisImeOption::MULTILINE)) != 0;
        if (!IsValidImeText(text, length, multiline, sdk)) {
            return Error::INVALID_TEXT;
        }
    }

    return g_ime_handler->SetText(text, length);
}

int PS4_SYSV_ABI sceImeSetTextGeometry(OrbisImeTextAreaMode mode,
                                       const OrbisImeTextGeometry* geometry) {
    if (!g_ime_handler) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    if (!geometry) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }

    const float x = geometry->x;
    const float y = geometry->y;
    if (x < 0.0f || x >= 1920.0f || y < 0.0f || y >= 1080.0f) {
        return static_cast<int>(Error::INVALID_PARAM);
    }

    if (mode != OrbisImeTextAreaMode::Select && mode != OrbisImeTextAreaMode::Preedit) {
        return static_cast<int>(Error::INVALID_PARAM);
    }

    return static_cast<int>(Error::OK);
}

Error PS4_SYSV_ABI sceImeUpdate(OrbisImeEventHandler handler) {
    if (!g_ime_handler && !g_keyboard_handler) {
        LOG_ERROR(Lib_Ime, "sceImeUpdate called with no active handler");
        return Error::NOT_OPENED;
    }

    const u32 sdk = GetCompiledSdkVersion();
    bool ime_dispatched = false;

    if (g_ime_handler) {
        if (handler == g_ime_handler->GetHandler()) {
            g_ime_handler->Update(handler);
            ime_dispatched = true;
        } else if (sdk < 0x1500000U) {
            ime_dispatched = true;
        } else if (!g_keyboard_handler) {
            return Error::NOT_OPENED;
        }
    }

    if (g_keyboard_handler) {
        g_keyboard_handler->Update(handler);
    }

    if (!ime_dispatched && !g_keyboard_handler) {
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
    LIB_FUNCTION("mN+ZoSN-8hQ", "libSceIme", 1, "libSceIme", FinalizeImeModule);
    LIB_FUNCTION("uTW+63goeJs", "libSceIme", 1, "libSceIme", InitializeImeModule);
    LIB_FUNCTION("Lf3DeGWC6xg", "libSceIme", 1, "libSceIme", sceImeCheckFilterText);
    LIB_FUNCTION("zHuMUGb-AQI", "libSceIme", 1, "libSceIme", sceImeCheckRemoteEventParam);
    LIB_FUNCTION("OTb0Mg+1i1k", "libSceIme", 1, "libSceIme", sceImeCheckUpdateTextInfo);
    LIB_FUNCTION("TmVP8LzcFcY", "libSceIme", 1, "libSceIme", sceImeClose);
    LIB_FUNCTION("Ho5NVQzpKHo", "libSceIme", 1, "libSceIme", sceImeConfigGet);
    LIB_FUNCTION("P5dPeiLwm-M", "libSceIme", 1, "libSceIme", sceImeConfigSet);
    LIB_FUNCTION("tKLmVIUkpyM", "libSceIme", 1, "libSceIme", sceImeConfirmCandidate);
    LIB_FUNCTION("NYDsL9a0oEo", "libSceIme", 1, "libSceIme", sceImeDicAddWord);
    LIB_FUNCTION("l01GKoyiQrY", "libSceIme", 1, "libSceIme", sceImeDicDeleteLearnDics);
    LIB_FUNCTION("E2OcGgi-FPY", "libSceIme", 1, "libSceIme", sceImeDicDeleteUserDics);
    LIB_FUNCTION("JAiMBkOTYKI", "libSceIme", 1, "libSceIme", sceImeDicDeleteWord);
    LIB_FUNCTION("JoPdCUXOzMU", "libSceIme", 1, "libSceIme", sceImeDicGetWords);
    LIB_FUNCTION("FuEl46uHDyo", "libSceIme", 1, "libSceIme", sceImeDicReplaceWord);
    LIB_FUNCTION("E+f1n8e8DAw", "libSceIme", 1, "libSceIme", sceImeDisableController);
    LIB_FUNCTION("evjOsE18yuI", "libSceIme", 1, "libSceIme", sceImeFilterText);
    LIB_FUNCTION("wVkehxutK-U", "libSceIme", 1, "libSceIme", sceImeForTestFunction);
    LIB_FUNCTION("T6FYjZXG93o", "libSceIme", 1, "libSceIme", sceImeGetPanelPositionAndForm);
    LIB_FUNCTION("ziPDcIjO0Vk", "libSceIme", 1, "libSceIme", sceImeGetPanelSize);
    LIB_FUNCTION("PMVehSlfZ94", "libSceIme", 1, "libSceIme", sceImeKeyboardClose);
    LIB_FUNCTION("VkqLPArfFdc", "libSceIme", 1, "libSceIme", sceImeKeyboardGetInfo);
    LIB_FUNCTION("dKadqZFgKKQ", "libSceIme", 1, "libSceIme", sceImeKeyboardGetResourceId);
    LIB_FUNCTION("eaFXjfJv3xs", "libSceIme", 1, "libSceIme", sceImeKeyboardOpen);
    LIB_FUNCTION("oYkJlMK51SA", "libSceIme", 1, "libSceIme", sceImeKeyboardOpenInternal);
    LIB_FUNCTION("ua+13Hk9kKs", "libSceIme", 1, "libSceIme", sceImeKeyboardSetMode);
    LIB_FUNCTION("3Hx2Uw9xnv8", "libSceIme", 1, "libSceIme", sceImeKeyboardUpdate);
    LIB_FUNCTION("RPydv-Jr1bc", "libSceIme", 1, "libSceIme", sceImeOpen);
    LIB_FUNCTION("16UI54cWRQk", "libSceIme", 1, "libSceIme", sceImeOpenInternal);
    LIB_FUNCTION("WmYDzdC4EHI", "libSceIme", 1, "libSceIme", sceImeParamInit);
    LIB_FUNCTION("TQaogSaqkEk", "libSceIme", 1, "libSceIme", sceImeSetCandidateIndex);
    LIB_FUNCTION("WLxUN2WMim8", "libSceIme", 1, "libSceIme", sceImeSetCaret);
    LIB_FUNCTION("ieCNrVrzKd4", "libSceIme", 1, "libSceIme", sceImeSetText);
    LIB_FUNCTION("TXYHFRuL8UY", "libSceIme", 1, "libSceIme", sceImeSetTextGeometry);
    LIB_FUNCTION("-4GCfYdNF1s", "libSceIme", 1, "libSceIme", sceImeUpdate);
    LIB_FUNCTION("oOwl47ouxoM", "libSceIme", 1, "libSceIme", sceImeVshClearPreedit);
    LIB_FUNCTION("gtoTsGM9vEY", "libSceIme", 1, "libSceIme", sceImeVshClose);
    LIB_FUNCTION("wTKF4mUlSew", "libSceIme", 1, "libSceIme", sceImeVshConfirmPreedit);
    LIB_FUNCTION("rM-1hkuOhh0", "libSceIme", 1, "libSceIme", sceImeVshDisableController);
    LIB_FUNCTION("42xMaQ+GLeQ", "libSceIme", 1, "libSceIme", sceImeVshGetPanelPositionAndForm);
    LIB_FUNCTION("ZmmV6iukhyo", "libSceIme", 1, "libSceIme", sceImeVshInformConfirmdString);
    LIB_FUNCTION("EQBusz6Uhp8", "libSceIme", 1, "libSceIme", sceImeVshInformConfirmdString2);
    LIB_FUNCTION("LBicRa-hj3A", "libSceIme", 1, "libSceIme", sceImeVshOpen);
    LIB_FUNCTION("-IAOwd2nO7g", "libSceIme", 1, "libSceIme", sceImeVshSendTextInfo);
    LIB_FUNCTION("qDagOjvJdNk", "libSceIme", 1, "libSceIme", sceImeVshSetCaretGeometry);
    LIB_FUNCTION("tNOlmxee-Nk", "libSceIme", 1, "libSceIme", sceImeVshSetCaretIndexInPreedit);
    LIB_FUNCTION("rASXozKkQ9g", "libSceIme", 1, "libSceIme", sceImeVshSetPanelPosition);
    LIB_FUNCTION("idvMaIu5H+k", "libSceIme", 1, "libSceIme", sceImeVshSetParam);
    LIB_FUNCTION("ga5GOgThbjo", "libSceIme", 1, "libSceIme", sceImeVshSetPreeditGeometry);
    LIB_FUNCTION("RuSca8rS6yA", "libSceIme", 1, "libSceIme", sceImeVshSetSelectGeometry);
    LIB_FUNCTION("J7COZrgSFRA", "libSceIme", 1, "libSceIme", sceImeVshSetSelectionText);
    LIB_FUNCTION("WqAayyok5p0", "libSceIme", 1, "libSceIme", sceImeVshUpdate);
    LIB_FUNCTION("O7Fdd+Oc-qQ", "libSceIme", 1, "libSceIme", sceImeVshUpdateContext);
    LIB_FUNCTION("fwcPR7+7Rks", "libSceIme", 1, "libSceIme", sceImeVshUpdateContext2);
};

} // namespace Libraries::Ime
