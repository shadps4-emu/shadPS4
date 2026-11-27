// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <queue>
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/emulator_settings.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_error.h"
#include "core/libraries/ime/ime_keyboard.h"
#include "core/libraries/ime/ime_ui.h"
#include "core/libraries/ime/ime_ui_shared.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/tls.h"
#include "core/user_settings.h"

namespace Libraries::Ime {

static std::recursive_mutex g_ime_api_mutex;
static std::shared_ptr<ImeState> g_ime_state;
static std::shared_ptr<ImeUi> g_ime_ui;
static std::atomic<ImeFrontend> g_ime_frontend{ImeFrontend::None};

std::recursive_mutex& GetImeMutex() {
    return g_ime_api_mutex;
}

bool IsRegisteredImeUser(s32 user_id) {
    return user_id >= 0 && UserManagement.GetUserByID(user_id) != nullptr;
}

bool TryAcquireImeFrontend(ImeFrontend frontend) {
    auto expected = ImeFrontend::None;
    return g_ime_frontend.compare_exchange_strong(expected, frontend);
}

void ReleaseImeFrontend(ImeFrontend frontend) {
    g_ime_frontend.compare_exchange_strong(frontend, ImeFrontend::None);
}

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
        return IsRegisteredImeUser(user_id);
    }

    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return false;
    }
    if (user_id == 0xfe) {
        return true;
    }
    return IsRegisteredImeUser(user_id);
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

static std::vector<ImeKbVariant> GetConfiguredImeKeyboards(s32 user_id) {
    const auto* user = UserManagement.GetUserByID(user_id);
    const auto configured = user && !user->ime_input_languages.empty()
                                ? user->ime_input_languages
                                : EmulatorSettings.GetImeInputLanguages();
    const bool ukrainian_available =
        (GetImeLanguageMask() & static_cast<u64>(OrbisImeLanguage::UKRAINIAN)) != 0;
    const auto compatible_variant = [ukrainian_available](ImeKbVariant variant) {
        return variant == ImeKbVariant::Ukrainian && !ukrainian_available ? ImeKbVariant::English_GB
                                                                          : variant;
    };
    std::vector<ImeKbVariant> result;
    for (const int id : configured) {
        if (const auto* profile = FindImeKeyboardSetting(id)) {
            const auto variant = compatible_variant(profile->variant);
            if (std::ranges::find(result, variant) == result.end()) {
                result.push_back(variant);
            }
        }
    }
    if (result.empty()) {
        result.push_back(ImeKbVariant::English_US);
        const auto language = ResolveImeKeyboardLanguage(static_cast<OrbisImeLanguage>(0),
                                                         EmulatorSettings.GetConsoleLanguage());
        const auto variant = compatible_variant(
            GetImeKeyboardLanguageProfile(language, EmulatorSettings.GetImeKeyboardLayout())
                .variant);
        if (variant != result.front()) {
            result.push_back(variant);
        }
    }
    return result;
}

OrbisImeLanguage GetImeInputLanguages(OrbisImeLanguage supported_languages, OrbisImeOption options,
                                      s32 user_id) {
    const auto supported =
        static_cast<u64>(supported_languages) != 0
            ? supported_languages
            : static_cast<OrbisImeLanguage>(GetImeLanguageMask() & kValidOrbisImeLanguageMask);
    if (user_id == 0xfe) {
        return supported;
    }
    auto configured = static_cast<OrbisImeLanguage>(0);
    for (const auto variant : GetConfiguredImeKeyboards(user_id)) {
        configured |= GetImeKeyboardLanguageProfile(variant).language;
    }
    return ResolveImeInputLanguages(supported, configured, options);
}

std::vector<ImeKbVariant> GetImeInputKeyboards(OrbisImeLanguage languages, OrbisImeOption options,
                                               s32 user_id) {
    auto result = GetOrderedImeKeyboardLanguages(languages, EmulatorSettings.GetConsoleLanguage());
    if (user_id == 0xfe) {
        return result;
    }
    const auto configured = GetConfiguredImeKeyboards(user_id);
    const bool forced = True(options & OrbisImeOption::LANGUAGES_FORCED);
    const bool overlap = std::ranges::any_of(configured, [&](const auto variant) {
        return True(GetImeKeyboardLanguageProfile(variant).language & languages);
    });
    std::erase_if(result, [&](const auto variant) {
        const auto language = GetImeKeyboardLanguageProfile(variant).language;
        const bool language_enabled = std::ranges::any_of(configured, [&](const auto enabled) {
            return GetImeKeyboardLanguageProfile(enabled).language == language;
        });
        if (language_enabled) {
            return std::ranges::find(configured, variant) == configured.end();
        }
        if (!forced && overlap) {
            return true;
        }
        return language == OrbisImeLanguage::JAPANESE &&
               variant != (forced ? ImeKbVariant::Japanese : ImeKbVariant::Japanese_Gojuon);
    });
    return result;
}

std::string GetLastImeKeyboard(s32 user_id) {
    const auto* user = UserManagement.GetUserByID(user_id);
    return user ? user->ime_last_keyboard : std::string{};
}

void RememberImeKeyboard(s32 user_id, const ImeKbLayoutSelection& selection) {
    if (selection.input_type == OrbisImeType::Number) {
        return;
    }
    if (auto* user = UserManagement.GetUserByID(user_id)) {
        const auto* name = GetImeKeyboardLanguageProfile(selection.variant).name;
        if (user->ime_last_keyboard != name) {
            user->ime_last_keyboard = name;
            UserManagement.Save();
        }
    }
}

class ImeHandler {
public:
    ImeHandler(const OrbisImeParam* param, const OrbisImeParamExtended* extended = nullptr) {
        m_param.ime = *param;
        m_param.ime_ext = extended ? *extended : OrbisImeParamExtended{};
        g_ime_state = std::make_shared<ImeState>(&m_param.ime);
        g_ime_ui = std::make_shared<ImeUi>(g_ime_state.get(), &m_param.ime, &m_param.ime_ext);
        OrbisImeEvent event{};
        event.id = OrbisImeEventId::Open;
        sceImeGetPanelSize(param, &event.param.rect.width, &event.param.rect.height);
        event.param.rect.x = param->posx;
        event.param.rect.y = param->posy;
        g_ime_state->SendEvent(&event);
    }

    Error Update(OrbisImeEventHandler handler) {
        if (!g_ime_state) {
            LOG_ERROR(Lib_Ime, "ImeHandler::Update called without active IME state");
            return Error::INTERNAL;
        }

        auto state = g_ime_state;
        auto ui = g_ime_ui;
        const auto callback = handler;
        const auto filter = m_param.ime.filter;
        const u32 max_length = m_param.ime.maxTextLength;
        void* const arg = m_param.ime.arg;
        while (g_ime_state == state && g_ime_ui == ui) {
            std::scoped_lock draw_lock(ui->draw_mutex);
            ImeState::QueuedEvent queued;
            {
                std::unique_lock queue_lock(state->queue_mutex);
                if (state->event_queue.empty()) {
                    break;
                }
                queued = std::move(state->event_queue.front());
                state->event_queue.pop();
            }
            auto& event = queued.event;
            const bool candidate_event = event.id >= OrbisImeEventId::CandidateListStart &&
                                         event.id <= OrbisImeEventId::CandidateCancel;
            if (candidate_event && queued.candidate_revision != state->candidate_revision) {
                continue;
            }
            const bool text_event = event.id == OrbisImeEventId::UpdateText ||
                                    event.id == OrbisImeEventId::UpdateCaret || candidate_event;
            if (text_event && queued.revision != state->text_revision) {
                continue;
            }
            if (event.id == OrbisImeEventId::UpdateText && !queued.text_edit &&
                queued.caret_revision != state->caret_revision) {
                continue;
            }
            if (event.id == OrbisImeEventId::PressEnter ||
                event.id == OrbisImeEventId::PressClose) {
                queued.text = state->reported_text;
                event.param.text.caret_index = state->reported_caret_index;
            }
            if (event.id == OrbisImeEventId::UpdateText && !queued.text_edit) {
                queued.text = state->reported_text;
                event.param.text.caret_index =
                    std::min(event.param.text.caret_index, static_cast<u32>(queued.text.size()));
            } else if (event.id == OrbisImeEventId::UpdateText && filter && !queued.preedit) {
                const auto original = queued.text;
                auto caret = event.param.text.caret_index;
                const bool filtered = FilterImeText(filter, queued.text, max_length, caret);
                if (g_ime_state != state || g_ime_ui != ui ||
                    queued.revision != state->text_revision) {
                    break;
                }
                if (filtered) {
                    event.param.text.caret_index = caret;
                    u32 prefix = 0;
                    while (prefix < state->reported_text.size() && prefix < queued.text.size() &&
                           state->reported_text[prefix] == queued.text[prefix]) {
                        ++prefix;
                    }
                    auto& edit = event.param.text.text_area[0];
                    edit.index = std::min(edit.index, prefix);
                    edit.length = static_cast<s32>(queued.text.size()) -
                                  static_cast<s32>(state->reported_text.size());

                    std::array<char16_t, ORBIS_IME_MAX_TEXT_LENGTH + 1> editor_text{};
                    state->ConvertUTF8ToOrbis(state->current_text.begin(),
                                              state->current_text.size(), editor_text.data(),
                                              editor_text.size());
                    if (original == editor_text.data()) {
                        state->ConvertOrbisToUTF8(queued.text.data(), queued.text.size(),
                                                  state->current_text.begin(),
                                                  state->current_text.capacity());
                        state->caret_index = static_cast<int>(caret);
                        state->caret_byte_index = Utf8ByteIndexFromUtf16Index(
                            state->current_text.begin(), state->caret_index);
                        state->caret_dirty = true;
                    }
                }
            }
            const bool preserve_caret = event.id == OrbisImeEventId::UpdateText &&
                                        queued.caret_revision != state->caret_revision;
            if (preserve_caret) {
                event.param.text.caret_index =
                    std::min(state->reported_caret_index, static_cast<u32>(queued.text.size()));
            }
            if (event.id == OrbisImeEventId::UpdateText) {
                for (u32 area = 1; area < std::min(event.param.text.area_num, 4U); ++area) {
                    auto& range = event.param.text.text_area[area];
                    range.index = std::min(range.index, static_cast<u32>(queued.text.size()));
                    range.length = std::clamp(range.length, 0,
                                              static_cast<s32>(queued.text.size() - range.index));
                }
            }
            if (event.id == OrbisImeEventId::UpdateCaret) {
                ui->text_select_mode = queued.selection_anchor >= 0;
                ui->text_select_anchor_utf16 = queued.selection_anchor;
            }
            if (event.id == OrbisImeEventId::UpdateText ||
                event.id == OrbisImeEventId::PressEnter ||
                event.id == OrbisImeEventId::PressClose) {
                auto* text = reinterpret_cast<char16_t*>(state->work_buffer);
                std::copy(queued.text.begin(), queued.text.end(), text);
                text[queued.text.size()] = 0;
                event.param.text.str = text;
                const auto& committed = queued.preedit ? queued.committed_text : queued.text;
                std::copy(committed.begin(), committed.end(), state->text_buffer);
                state->text_buffer[committed.size()] = 0;
                state->reported_text = queued.text;
                state->reported_caret_index = event.param.text.caret_index;
                if (preserve_caret) {
                    ui->SetCaret(event.param.text.caret_index);
                }
            }
            if (event.id == OrbisImeEventId::CandidateListStart) {
                state->candidate_count = 0;
            } else if (event.id == OrbisImeEventId::CandidateWord) {
                ++state->candidate_count;
                event.param.candidate_word = queued.text.data();
            } else if (event.id == OrbisImeEventId::CandidateDone ||
                       event.id == OrbisImeEventId::CandidateCancel) {
                state->candidate_count = -1;
            }
            const bool was_dispatching_caret = ui->dispatching_caret_move;
            ui->dispatching_caret_move = event.id == OrbisImeEventId::UpdateCaret;
            SCOPE_EXIT {
                ui->dispatching_caret_move = was_dispatching_caret;
            };
            callback(arg, &event);
        }

        return Error::OK;
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
        auto ui = g_ime_ui;
        std::scoped_lock lock(ui->draw_mutex);
        g_ime_state->SetText(text, length);
        ui->published_candidates.clear();
        ui->candidate_events_active = false;
        ui->text_select_mode = false;
        ui->text_select_anchor_utf16 = ui->text_select_focus_utf16 = g_ime_state->caret_index;
        ui->last_input_text.clear();
        ui->last_input_caret = -1;
        return Error::OK;
    }

    Error SelectCandidate(s32 index, bool confirm) {
        auto state = g_ime_state;
        auto ui = g_ime_ui;
        std::scoped_lock lock(ui->draw_mutex);
        if (state->candidate_count < 0) {
            return Error::INVALID_TIMING;
        }
        if (index < 0 || index >= state->candidate_count) {
            return Error::INVALID_PARAM;
        }
        return ui->SelectCandidate(index, confirm) ? Error::OK : Error::INVALID_PARAM;
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
        auto state = g_ime_state;
        auto ui = g_ime_ui;
        std::scoped_lock lock(ui->draw_mutex);
        const auto old_selection_index = state->selection_index;
        const auto old_selection_length = state->selection_length;
        const bool moved = state->reported_caret_index != caret->index;
        state->caret_geometry = *caret;
        state->geometry_dirty = true;
        if (moved || ui->dispatching_caret_move) {
            ui->SetCaret(caret->index);
        } else {
            state->SetCaret(caret->index);
        }
        if (moved || old_selection_length != state->selection_length ||
            (state->selection_length != 0 && old_selection_index != state->selection_index)) {
            OrbisImeEvent event{};
            event.id = OrbisImeEventId::UpdateText;
            event.param.text.str = reinterpret_cast<char16_t*>(g_ime_state->work_buffer);
            event.param.text.caret_index = caret->index;
            event.param.text.area_num = 1;
            event.param.text.text_area[0].mode = OrbisImeTextAreaMode::Edit;
            if (state->selection_length != 0) {
                event.param.text.area_num = 2;
                event.param.text.text_area[1] = {OrbisImeTextAreaMode::Select,
                                                 state->selection_index,
                                                 static_cast<s32>(state->selection_length)};
            }
            std::copy(state->reported_text.begin(), state->reported_text.end(),
                      event.param.text.str);
            event.param.text.str[state->reported_text.size()] = 0;
            std::copy(state->reported_text.begin(), state->reported_text.end(), state->text_buffer);
            state->text_buffer[state->reported_text.size()] = 0;
            if (m_param.ime.handler) {
                m_param.ime.handler(m_param.ime.arg, &event);
            }
        }
        return Error::OK;
    }

    Error GetPanelPosition(OrbisImePositionAndForm* position) {
        std::scoped_lock lock(g_ime_ui->draw_mutex);
        *position = g_ime_state->panel_layout;
        return Error::OK;
    }

    Error SetTextGeometry(OrbisImeTextAreaMode mode, const OrbisImeTextGeometry* geometry) {
        std::scoped_lock lock(g_ime_ui->draw_mutex);
        g_ime_state->text_geometry[static_cast<u32>(mode)] = *geometry;
        g_ime_state->geometry_dirty = true;
        return Error::OK;
    }

    OrbisImeEventHandler GetHandler() const {
        return m_param.ime.handler;
    }

    u32 GetImeOptionBits() const {
        return static_cast<u32>(m_param.ime.option);
    }

    u32 GetImeCurrentTextLength() const {
        auto ui = g_ime_ui;
        std::scoped_lock lock(ui->draw_mutex);
        return g_ime_state ? static_cast<u32>(g_ime_state->reported_text.size()) : 0U;
    }

private:
    struct ImeParam {
        OrbisImeParam ime;
        OrbisImeParamExtended ime_ext;
    } m_param{};
};

static std::unique_ptr<ImeHandler> g_ime_handler;

int PS4_SYSV_ABI FinalizeImeModule() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI InitializeImeModule() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckFilterText(char16_t* text, bool repair) {
    if (!text) {
        return -3;
    }
    constexpr u32 filter_text_limit = 152;
    u32 length = 0;
    while (length < filter_text_limit && text[length]) {
        ++length;
    }
    if (length == filter_text_limit) {
        if (!repair) {
            return -2;
        }
        text[filter_text_limit] = 0;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckRemoteEventParam(OrbisImeEvent* event, bool repair) {
    if (!event) {
        return -3;
    }
    if (event->id == OrbisImeEventId::UpdateText) {
        return sceImeCheckUpdateTextInfo(&event->param.text, repair);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeCheckUpdateTextInfo(OrbisImeEditText* text, bool repair) {
    if (!text) {
        return -3;
    }
    u32 length = 0;
    if (text->str) {
        while (length < ORBIS_IME_MAX_TEXT_LENGTH && text->str[length]) {
            ++length;
        }
    }
    auto checked = *text;
    checked.area_num = std::min<u32>(checked.area_num, std::size(checked.text_area));
    bool changed = false;
    for (u32 i = 0; i < checked.area_num; ++i) {
        auto& area = checked.text_area[i];
        if (area.mode != OrbisImeTextAreaMode::Preedit &&
            area.mode != OrbisImeTextAreaMode::Select) {
            continue;
        }
        if (area.index > length || area.length < 0 ||
            static_cast<u32>(area.length) > length - area.index) {
            if (!repair) {
                return -3;
            }
            area.index = 0;
            area.length = 0;
            changed = true;
        }
    }
    if (checked.caret_index > length) {
        if (!repair) {
            return -3;
        }
        checked.caret_index = length;
        changed = true;
    }
    if (repair && changed) {
        *text = checked;
    }
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeClose() {
    std::scoped_lock api_lock(g_ime_api_mutex);
    LOG_DEBUG(Lib_Ime, "called");

    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "No IME handler is open");
        return Error::NOT_OPENED;
    }

    ConfigureImeKeyboard(false);
    g_ime_handler.reset();
    g_ime_ui.reset();
    g_ime_state.reset();
    ReleaseImeFrontend(ImeFrontend::Ime);

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
    std::scoped_lock api_lock(g_ime_api_mutex);
    LOG_TRACE(Lib_Ime, "index={}", index);
    if (!g_ime_handler) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    return static_cast<int>(g_ime_handler->SelectCandidate(index, true));
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
    std::scoped_lock lock(g_ime_api_mutex);
    if (!g_ime_handler) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    g_ime_ui->DisableController();
    return static_cast<int>(Error::OK);
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
    std::scoped_lock lock(g_ime_api_mutex);
    if (!g_ime_handler) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    if (!posForm) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }
    return static_cast<int>(g_ime_handler->GetPanelPosition(posForm));
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

int PS4_SYSV_ABI sceImeKeyboardOpenInternal() {
    LOG_ERROR(Lib_Ime, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeOpen(const OrbisImeParam* param, const OrbisImeParamExtended* extended) {
    std::scoped_lock api_lock(g_ime_api_mutex);
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

    if (!TryAcquireImeFrontend(ImeFrontend::Ime)) {
        return Error::NOT_ACTIVE;
    }

    std::memset(param->work, 0, 0x5000);

    if (extended) {
        g_ime_handler = std::make_unique<ImeHandler>(param, extended);
    } else {
        g_ime_handler = std::make_unique<ImeHandler>(param);
    }
    if (!g_ime_handler) {
        LOG_ERROR(Lib_Ime, "Failed to create IME handler");
        ReleaseImeFrontend(ImeFrontend::Ime);
        return Error::NO_MEMORY; // or Error::INTERNAL
    }

    ConfigureImeKeyboard(true, param->user_id, extended);
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
    std::scoped_lock api_lock(g_ime_api_mutex);
    LOG_TRACE(Lib_Ime, "index={}", index);
    if (!g_ime_handler) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    return static_cast<int>(g_ime_handler->SelectCandidate(index, false));
}

Error PS4_SYSV_ABI sceImeSetCaret(const OrbisImeCaret* caret) {
    std::scoped_lock api_lock(g_ime_api_mutex);
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
    std::scoped_lock api_lock(g_ime_api_mutex);
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
    std::scoped_lock api_lock(g_ime_api_mutex);
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

    return static_cast<int>(g_ime_handler->SetTextGeometry(mode, geometry));
}

Error PS4_SYSV_ABI sceImeUpdate(OrbisImeEventHandler handler) {
    std::scoped_lock api_lock(g_ime_api_mutex);
    const bool keyboard = HasImeKeyboardHandler(handler);
    if (!g_ime_handler && !keyboard) {
        return Error::NOT_OPENED;
    }
    if (g_ime_handler && handler == g_ime_handler->GetHandler()) {
        const auto keyboard_result = DispatchImeKeyboard(handler);
        if (keyboard_result != Error::OK) {
            return keyboard_result;
        }
        if (g_ime_handler && handler == g_ime_handler->GetHandler()) {
            return g_ime_handler->Update(handler);
        }
        return Error::OK;
    }
    if (keyboard) {
        return DispatchImeKeyboard(handler);
    }
    return GetCompiledSdkVersion() < 0x1500000U ? Error::OK : Error::NOT_OPENED;
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
