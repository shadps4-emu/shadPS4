// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_timer.h>
#include <imgui_internal.h>
#include "common/scope_exit.h"
#include "core/emulator_settings.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_composition.h"
#include "core/libraries/ime/ime_kb_layout.h"
#include "core/libraries/ime/ime_keyboard.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/rtc/rtc.h"
#include "core/user_settings.h"
#include "input/input_handler.h"

namespace Libraries::Ime {
namespace {

struct KeyboardClient;

struct KeyboardInput {
    SDL_Event event{};
    std::string text;
    u64 generation = 0;
    SDL_Keycode character = 0;
    bool synthesize_text = false;
    bool normalize_text = false;
    bool method_key = false;
    ImeKbVariant variant = ImeKbVariant::Automatic;
    std::optional<u32> external_mode;
    std::optional<OrbisImeKeycode> osk_code;
    std::weak_ptr<KeyboardClient> client;
};

struct KeyboardClient {
    s32 user_id;
    OrbisImeKeyboardParam param;
    u32 mode = 0;
    bool shift_pending = false;
    char16_t previous_character = 0;
    std::deque<OrbisImeEvent> events;
    std::atomic<bool> event_overflow = false;

    void QueueEvent(const OrbisImeEvent& event) {
        if (events.size() >= 4096) {
            event_overflow = true;
            return;
        }
        events.push_back(event);
    }
};

std::shared_ptr<KeyboardClient> keyboard_client;
std::mutex input_mutex;
std::deque<KeyboardInput> keyboard_input;
std::deque<KeyboardInput> filtered_input;
std::vector<u32> keyboard_resources;
std::atomic<bool> ime_active = false;
s32 ime_user_id = -1;
OrbisImeParamExtended ime_extended{};
ImeKbVariant ime_variant = ImeKbVariant::Automatic;
char16_t ime_dead_key = 0;
std::array<SDL_Scancode, 11> ime_keypad_keys{};
u32 ime_method_state = 0;
bool ime_shift_pending = false;
u64 ime_generation = 0;
bool dispatching = false;
bool discard_filtered_text = false;
std::map<u32, ImeKeyboardContext> keyboard_event_contexts;
thread_local const SDL_Event* replayed_event = nullptr;
thread_local std::optional<ImeKeyboardContext> replayed_context;

ImeKbVariant KeyboardVariant() {
    const auto profiles = GetImeKeyboardLanguageProfiles();
    const auto language = EmulatorSettings.GetConsoleLanguage();
    const auto profile =
        std::ranges::find(profiles, language, &ImeKbLanguageProfile::console_language);
    return profile == profiles.end() ? ImeKbVariant::English_US : profile->variant;
}

OrbisImeKeyboardType KeyboardType(ImeKbVariant variant) {
    constexpr std::array types{
        OrbisImeKeyboardType::ENGLISH_US,
        OrbisImeKeyboardType::ENGLISH_US,
        OrbisImeKeyboardType::ENGLISH_GB,
        OrbisImeKeyboardType::POLISH,
        OrbisImeKeyboardType::SPANISH,
        OrbisImeKeyboardType::GERMAN,
        OrbisImeKeyboardType::SPANISH_LA,
        OrbisImeKeyboardType::DUTCH,
        OrbisImeKeyboardType::DANISH,
        OrbisImeKeyboardType::RUSSIAN,
        OrbisImeKeyboardType::SWEDISH,
        OrbisImeKeyboardType::FINNISH,
        OrbisImeKeyboardType::FRENCH_CA,
        OrbisImeKeyboardType::FRENCH,
        OrbisImeKeyboardType::ITALIAN,
        OrbisImeKeyboardType::NORWEGIAN,
        OrbisImeKeyboardType::PORTUGUESE_PT,
        OrbisImeKeyboardType::PORTUGUESE_BR,
        OrbisImeKeyboardType::TURKISH,
        OrbisImeKeyboardType::JAPANESE_ROMAN,
        OrbisImeKeyboardType::JAPANESE_KANA,
        OrbisImeKeyboardType::KOREAN,
        OrbisImeKeyboardType::SM_CHINESE,
        OrbisImeKeyboardType::TR_CHINESE_PY_HK,
        OrbisImeKeyboardType::TR_CHINESE_PY_TW,
        OrbisImeKeyboardType::TR_CHINESE_ZY,
        OrbisImeKeyboardType::TR_CHINESE_CG,
        OrbisImeKeyboardType::ARABIC_AR,
        OrbisImeKeyboardType::THAI,
        OrbisImeKeyboardType::CZECH,
        OrbisImeKeyboardType::GREEK,
        OrbisImeKeyboardType::HUNGARIAN,
        OrbisImeKeyboardType::INDONESIAN,
        OrbisImeKeyboardType::ROMANIAN,
        OrbisImeKeyboardType::VIETNAMESE,
        OrbisImeKeyboardType::UKRAINIAN,
    };
    static_assert(types.size() == static_cast<std::size_t>(ImeKbVariant::Ukrainian) + 1);
    const auto index = static_cast<std::size_t>(variant);
    return index < types.size() ? types[index] : OrbisImeKeyboardType::ENGLISH_US;
}

OrbisImeKeyboardType KeyboardType() {
    return KeyboardType(KeyboardVariant());
}

bool ValidUser(s32 user_id) {
    s32 sdk = 0;
    Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk);
    if (user_id == 0xfe || (sdk < 0x1500000 && (user_id == -1 || user_id == 0xff))) {
        return true;
    }
    return IsRegisteredImeUser(user_id);
}

std::vector<u32> KeyboardResources() {
    std::scoped_lock lock(input_mutex);
    return keyboard_resources;
}

u32 KeyStatus(SDL_Keymod modifiers) {
    u32 status = static_cast<u32>(OrbisImeKeycodeState::KEYCODE_VALID);
    constexpr std::array<SDL_Keymod, 11> masks = {SDL_KMOD_LCTRL, SDL_KMOD_LSHIFT, SDL_KMOD_LALT,
                                                  SDL_KMOD_LGUI,  SDL_KMOD_RCTRL,  SDL_KMOD_RSHIFT,
                                                  SDL_KMOD_RALT,  SDL_KMOD_RGUI,   SDL_KMOD_NUM,
                                                  SDL_KMOD_CAPS,  SDL_KMOD_SCROLL};
    for (u32 i = 0; i < masks.size(); ++i) {
        if ((modifiers & masks[i]) != 0) {
            status |= 1U << (8 + i);
        }
    }
    return status;
}

SDL_Keymod KeyModifiers(u32 status) {
    constexpr std::array<SDL_Keymod, 11> masks = {SDL_KMOD_LCTRL, SDL_KMOD_LSHIFT, SDL_KMOD_LALT,
                                                  SDL_KMOD_LGUI,  SDL_KMOD_RCTRL,  SDL_KMOD_RSHIFT,
                                                  SDL_KMOD_RALT,  SDL_KMOD_RGUI,   SDL_KMOD_NUM,
                                                  SDL_KMOD_CAPS,  SDL_KMOD_SCROLL};
    SDL_Keymod modifiers = SDL_KMOD_NONE;
    for (u32 i = 0; i < masks.size(); ++i) {
        if ((status & (1U << (8 + i))) != 0) {
            modifiers |= masks[i];
        }
    }
    return modifiers;
}

bool UpdateKeyboardMethod(ImeKbVariant variant, const SDL_KeyboardEvent& event, u32& mode,
                          bool& shift_pending) {
    if (event.repeat || ResolveImeExternalKeyboardMode(variant, ~0U) == 0) {
        return false;
    }
    const auto key = event.scancode;
    const bool shift_key = key == SDL_SCANCODE_LSHIFT || key == SDL_SCANCODE_RSHIFT;
    const bool alt_key = key == SDL_SCANCODE_LALT || key == SDL_SCANCODE_RALT;
    const bool chinese_shift =
        variant == ImeKbVariant::Chinese_TW_Zhuyin || variant == ImeKbVariant::Chinese_TW_Cangjie;
    if (event.type == SDL_EVENT_KEY_UP) {
        if (shift_key && std::exchange(shift_pending, false) && chinese_shift) {
            mode ^= 0x04000000U;
            return true;
        }
        return false;
    }
    if (shift_key) {
        shift_pending = true;
    } else {
        shift_pending = false;
    }
    const bool chord =
        (shift_key && (event.mod & SDL_KMOD_ALT)) || (alt_key && (event.mod & SDL_KMOD_SHIFT));
    const bool arabic_chord = (key == SDL_SCANCODE_LSHIFT && (event.mod & SDL_KMOD_LALT)) ||
                              (key == SDL_SCANCODE_LALT && (event.mod & SDL_KMOD_LSHIFT));
    if ((variant == ImeKbVariant::Arabic ? arabic_chord : chord) ||
        (variant == ImeKbVariant::Korean &&
         (key == SDL_SCANCODE_RALT || key == SDL_SCANCODE_LANG1)) ||
        ((variant == ImeKbVariant::Japanese || variant == ImeKbVariant::Japanese_Gojuon) &&
         (key == SDL_SCANCODE_LANG5 || key == SDL_SCANCODE_GRAVE))) {
        mode ^= 0x04000000U;
        shift_pending = false;
        return true;
    }
    if (variant == ImeKbVariant::Japanese || variant == ImeKbVariant::Japanese_Gojuon) {
        if (key == SDL_SCANCODE_INTERNATIONAL2 || key == SDL_SCANCODE_LANG3 ||
            key == SDL_SCANCODE_LANG4) {
            const bool katakana =
                key == SDL_SCANCODE_LANG3 ||
                (key == SDL_SCANCODE_INTERNATIONAL2 && (event.mod & SDL_KMOD_SHIFT));
            mode = (mode & ~0x0c000000U) | 0x04000000U | (katakana ? 0x08000000U : 0U);
            return true;
        }
        if ((mode & 0x03000000U) && key >= SDL_SCANCODE_F6 && key <= SDL_SCANCODE_F10) {
            constexpr std::array<u32, 5> modes{0x14000000U, 0x1c000000U, 0x0c000000U, 0x18000000U,
                                               0x08000000U};
            mode = (mode & ~0x1c000000U) | modes[key - SDL_SCANCODE_F6];
            return true;
        }
    }
    return false;
}

SDL_Scancode ImeKeypadNavigationKey(SDL_Scancode key, SDL_Keymod modifiers) {
    if (modifiers & SDL_KMOD_NUM) {
        return key;
    }
    if (key >= SDL_SCANCODE_KP_1 && key <= SDL_SCANCODE_KP_PERIOD) {
        constexpr std::array navigation{
            SDL_SCANCODE_END,    SDL_SCANCODE_DOWN,   SDL_SCANCODE_PAGEDOWN, SDL_SCANCODE_LEFT,
            SDL_SCANCODE_CLEAR,  SDL_SCANCODE_RIGHT,  SDL_SCANCODE_HOME,     SDL_SCANCODE_UP,
            SDL_SCANCODE_PAGEUP, SDL_SCANCODE_INSERT, SDL_SCANCODE_DELETE};
        return navigation[key - SDL_SCANCODE_KP_1];
    }
    return key;
}

void NormalizeImeKeypadEvent(SDL_KeyboardEvent& event) {
    const auto physical_key = event.scancode;
    if (physical_key < SDL_SCANCODE_KP_1 || physical_key > SDL_SCANCODE_KP_PERIOD) {
        return;
    }
    auto& held_key = ime_keypad_keys[physical_key - SDL_SCANCODE_KP_1];
    const auto key = held_key != SDL_SCANCODE_UNKNOWN
                         ? held_key
                         : ImeKeypadNavigationKey(physical_key, event.mod);
    held_key = event.type == SDL_EVENT_KEY_UP ? SDL_SCANCODE_UNKNOWN : key;
    event.scancode = key;
    event.key = SDL_GetKeyFromScancode(key, event.mod, true);
}

char16_t ImeAlphabetKey(SDL_Scancode key, SDL_Keymod modifiers,
                        const OrbisImeParamExtended& extended, ImeKbVariant variant) {
    if (variant == ImeKbVariant::Arabic && (extended.ext_keyboard_mode & 2U) &&
        (modifiers & SDL_KMOD_RCTRL) && (modifiers & SDL_KMOD_RSHIFT) && key >= SDL_SCANCODE_1 &&
        key <= SDL_SCANCODE_4) {
        return std::u16string_view(u"\u200d\u200c\u200e\u200f")[key - SDL_SCANCODE_1];
    }
    if (modifiers & (SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI)) {
        return 0;
    }
    if (ImeKeypadNavigationKey(key, modifiers) != key) {
        return 0;
    }
    const bool shift = (modifiers & SDL_KMOD_SHIFT) != 0;
    if (key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
        const bool uppercase = shift != ((modifiers & SDL_KMOD_CAPS) != 0);
        return (uppercase ? u'A' : u'a') + key - SDL_SCANCODE_A;
    }
    if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_0) {
        return (shift ? std::u16string_view(u"!@#$%^&*()")
                      : std::u16string_view(u"1234567890"))[key - SDL_SCANCODE_1];
    }
    if (key >= SDL_SCANCODE_KP_1 && key <= SDL_SCANCODE_KP_9) {
        return u'1' + key - SDL_SCANCODE_KP_1;
    }
    if (key == SDL_SCANCODE_KP_0) {
        return u'0';
    }
    constexpr std::array keypad_symbols{SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
                                        SDL_SCANCODE_KP_MINUS,  SDL_SCANCODE_KP_PLUS,
                                        SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_KP_EQUALS,
                                        SDL_SCANCODE_KP_COMMA};
    if (const auto found = std::ranges::find(keypad_symbols, key); found != keypad_symbols.end()) {
        return std::u16string_view(u"/*-+.=,")[found - keypad_symbols.begin()];
    }
    constexpr std::array keys{
        SDL_SCANCODE_SPACE,       SDL_SCANCODE_MINUS,        SDL_SCANCODE_EQUALS,
        SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_BACKSLASH,
        SDL_SCANCODE_SEMICOLON,   SDL_SCANCODE_APOSTROPHE,   SDL_SCANCODE_GRAVE,
        SDL_SCANCODE_COMMA,       SDL_SCANCODE_PERIOD,       SDL_SCANCODE_SLASH};
    const auto found = std::ranges::find(keys, key);
    if (found == keys.end()) {
        return 0;
    }
    return (shift ? std::u16string_view(u" _+{}|:\"~<>?")
                  : std::u16string_view(u" -=[]\\;'`,./"))[found - keys.begin()];
}

std::string ImeExternalKeyText(char16_t character, SDL_Scancode key,
                               const OrbisImeParamExtended& extended, ImeKbVariant variant,
                               char16_t& dead_key) {
    char text[5]{};
    ImTextCharToUtf8(text, character);
    auto mode = extended.ext_keyboard_mode & ~0x10000000U;
    if ((key >= SDL_SCANCODE_KP_DIVIDE && key <= SDL_SCANCODE_KP_PERIOD) ||
        key == SDL_SCANCODE_KP_EQUALS || key == SDL_SCANCODE_KP_COMMA) {
        return ComposeImeExternalText(text, variant, mode, dead_key);
    }
    return ComposeImeExternalText(TransformImeExternalText(text, variant, mode), variant, mode,
                                  dead_key);
}

void QueueKeyboardOutput(KeyboardClient& client, OrbisImeEvent event, const SDL_KeyboardEvent& key,
                         ImeKbVariant variant) {
    auto& code = event.param.keycode;
    bool method_key = false;
    if (!(client.mode & 1U)) {
        u32 method =
            ((client.mode & 2U) ? 0x04000000U : 0U) | ((client.mode & 8U) ? 0x08000000U : 0U);
        if (UpdateKeyboardMethod(variant, key, method, client.shift_pending)) {
            method_key = true;
            client.mode = (client.mode & ~2U) | ((method & 0x04000000U) ? 2U : 0U);
            if (variant == ImeKbVariant::Japanese_Gojuon) {
                client.mode = (client.mode & ~8U) | ((method & 0x08000000U) ? 8U : 0U);
            }
            client.previous_character = 0;
        }
    }
    if (event.id == OrbisImeEventId::KeyboardKeycodeUp || method_key) {
        code.character = 0;
        code.status &= ~static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
        client.QueueEvent(event);
        return;
    }
    const bool changeable =
        ResolveImeExternalKeyboardMode(variant, ~0U) != 0 && variant != ImeKbVariant::Japanese &&
        variant != ImeKbVariant::Chinese_CN && variant != ImeKbVariant::Chinese_TW &&
        variant != ImeKbVariant::Chinese_HK;
    std::u16string characters;
    if (changeable) {
        OrbisImeParamExtended settings{};
        settings.ext_keyboard_mode = ((client.mode & 2U) ? 0x04000000U : 0U) |
                                     ((client.mode & 32U) ? 0U : 1U) |
                                     ((client.mode & 64U) ? 0U : 2U);
        const auto alphabet = ImeAlphabetKey(key.scancode, key.mod, settings, variant);
        if (alphabet) {
            char text[5]{};
            ImTextCharToUtf8(text, alphabet);
            const bool keypad = (key.scancode >= SDL_SCANCODE_KP_DIVIDE &&
                                 key.scancode <= SDL_SCANCODE_KP_PERIOD) ||
                                key.scancode == SDL_SCANCODE_KP_EQUALS ||
                                key.scancode == SDL_SCANCODE_KP_COMMA;
            auto mapped =
                TransformImeExternalText(text, variant, settings.ext_keyboard_mode, !keypad);
            for (const char* p = mapped.c_str(); *p;) {
                unsigned int character = 0;
                p += ImTextCharFromUtf8(&character, p, nullptr);
                if (character <= 0xffff) {
                    characters += static_cast<char16_t>(character);
                }
            }
            if (variant == ImeKbVariant::Japanese_Gojuon) {
                characters = ImeKeyboardKanaText(characters, (client.mode & 8U) != 0,
                                                 (client.mode & 16U) != 0);
            }
        }
    } else if (code.status & static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID)) {
        characters += code.character;
    }
    code.character = 0;
    code.status &= ~static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
    if (characters.empty()) {
        if (key.scancode < SDL_SCANCODE_LCTRL || key.scancode > SDL_SCANCODE_RGUI) {
            client.previous_character = 0;
        }
        client.QueueEvent(event);
        return;
    }
    bool replace = false;
    if (!(client.mode & 4U) && client.previous_character && characters.size() == 1) {
        const auto original = characters.front();
        if (variant == ImeKbVariant::Greek && original == u' ' &&
            std::u16string_view(u"΄¨΅").contains(client.previous_character)) {
            client.previous_character = 0;
            client.QueueEvent(event);
            return;
        }
        auto combined = CombineImeKeyboardCharacter(client.previous_character, original, variant);
        if (combined.size() > 1 && combined.front() == client.previous_character) {
            combined.erase(combined.begin());
        } else {
            replace = combined != characters ||
                      (variant == ImeKbVariant::Greek &&
                       UpdateImeGreekDeadKey(client.previous_character, original) != 0 &&
                       std::u16string_view(u"΄¨΅").contains(client.previous_character));
        }
        characters = std::move(combined);
    }
    for (std::size_t i = 0; i < characters.size(); ++i) {
        auto output = event;
        output.param.keycode.character = characters[i];
        output.param.keycode.status |= static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
        if (replace && i == 0) {
            output.param.keycode.status |=
                static_cast<u32>(OrbisImeKeycodeState::REPLACE_CHARACTER);
        }
        if (i + 1 < characters.size()) {
            output.param.keycode.status |= static_cast<u32>(OrbisImeKeycodeState::CONTINUOUS_EVENT);
        }
        client.QueueEvent(output);
    }
    client.previous_character = characters.back();
}

void ReplayInput(KeyboardInput input) {
    std::scoped_lock lock(input_mutex);
    if (input.generation != ime_generation || !ime_active) {
        return;
    }
    filtered_input.push_back(std::move(input));
    SDL_Event wake{};
    wake.type = SDL_EVENT_IME_KEYBOARD;
    if (!SDL_PushEvent(&wake)) {
        filtered_input.pop_back();
    }
}

} // namespace

void DiscardImeKeyboardContexts() {
    std::scoped_lock lock(input_mutex);
    for (const auto& event : ImGui::GetCurrentContext()->InputEventsTrail) {
        keyboard_event_contexts.erase(event.EventId);
    }
}

void TrackImeKeyboardEvents(const SDL_Event* event, u32 first, u32 last) {
    if (event != replayed_event || !replayed_context) {
        return;
    }
    std::scoped_lock lock(input_mutex);
    for (auto id = first; id != last; ++id) {
        if (keyboard_event_contexts.size() >= 4096) {
            if (keyboard_client) {
                keyboard_client->event_overflow = true;
            }
            break;
        }
        keyboard_event_contexts.insert_or_assign(id, *replayed_context);
    }
}

std::optional<ImeKeyboardContext> TakeImeKeyboardContext(u32 event_id) {
    std::scoped_lock lock(input_mutex);
    const auto found = keyboard_event_contexts.find(event_id);
    if (found == keyboard_event_contexts.end()) {
        return std::nullopt;
    }
    const auto context = found->second;
    keyboard_event_contexts.erase(found);
    return context;
}

std::optional<ImeKeyboardContext> GetImeExternalKeyboardContext(bool settled_only) {
    std::scoped_lock lock(input_mutex);
    if (!ime_active || (False(ime_extended.option & OrbisImeExtOption::INIT_EXT_KEYBOARD_MODE) &&
                        !ime_extended.ext_keyboard_filter)) {
        return std::nullopt;
    }
    if (settled_only &&
        (!keyboard_input.empty() || !filtered_input.empty() || !keyboard_event_contexts.empty())) {
        return std::nullopt;
    }
    return ImeKeyboardContext{
        ime_variant, ResolveImeExternalKeyboardMode(ime_variant, ime_method_state), ime_generation};
}

void SetImeKeyboardCompositionState(bool preedit, bool selected) {
    std::scoped_lock lock(input_mutex);
    ime_method_state = (ime_method_state & ~0x03000000U) | (preedit ? 0x01000000U : 0U) |
                       (selected ? 0x02000000U : 0U);
}

void QueueImeOskKey(ImeKbKeyAction action, const char* text, ImeKbVariant variant, bool uppercase,
                    bool repeat, bool shortcut) {
    std::scoped_lock lock(input_mutex);
    const auto client = keyboard_client;
    if (!ime_active || !client || (client->user_id != ime_user_id && client->user_id != 0xfe) ||
        False(client->param.option & OrbisImeKeyboardOption::AddOsk) ||
        False(client->param.option & OrbisImeKeyboardOption::EffectiveWithIme) ||
        (repeat && False(client->param.option & OrbisImeKeyboardOption::Repeat))) {
        return;
    }
    SDL_Scancode key = SDL_SCANCODE_UNKNOWN;
    std::string_view characters;
    switch (action) {
    case ImeKbKeyAction::Character:
        characters = text ? text : "";
        break;
    case ImeKbKeyAction::Space:
        key = SDL_SCANCODE_SPACE;
        characters = " ";
        break;
    case ImeKbKeyAction::Tab:
        key = SDL_SCANCODE_TAB;
        characters = "\t";
        break;
    case ImeKbKeyAction::NewLine:
    case ImeKbKeyAction::Done:
        key = SDL_SCANCODE_RETURN;
        characters = "\r";
        break;
    case ImeKbKeyAction::Backspace:
        key = SDL_SCANCODE_BACKSPACE;
        break;
    case ImeKbKeyAction::ArrowLeft:
        key = SDL_SCANCODE_LEFT;
        break;
    case ImeKbKeyAction::ArrowRight:
        key = SDL_SCANCODE_RIGHT;
        break;
    case ImeKbKeyAction::ArrowUp:
        key = SDL_SCANCODE_UP;
        break;
    case ImeKbKeyAction::ArrowDown:
        key = SDL_SCANCODE_DOWN;
        break;
    case ImeKbKeyAction::Shift:
        key = SDL_SCANCODE_LSHIFT;
        break;
    default:
        return;
    }
    OrbisImeKeycode code{};
    code.type = KeyboardType(variant);
    code.user_id = ime_user_id;
    code.status = static_cast<u32>(OrbisImeKeycodeState::FROM_OSK) |
                  (shortcut ? static_cast<u32>(OrbisImeKeycodeState::FROM_OSK_SHORTCUT) : 0U) |
                  (uppercase ? static_cast<u32>(OrbisImeKeycodeState::MODIFIER_L_SHIFT) : 0U);
    if (!characters.empty()) {
        unsigned int character = 0;
        ImTextCharFromUtf8(&character, characters.data(), characters.data() + characters.size());
        if (character <= 0xffff && (character < 0xd800 || character > 0xdfff)) {
            code.character = static_cast<char16_t>(character);
            code.status |= static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
        }
        if (key == SDL_SCANCODE_UNKNOWN) {
            for (int scan = SDL_SCANCODE_A; scan <= SDL_SCANCODE_SLASH; ++scan) {
                for (auto modifiers : {SDL_KMOD_NONE, SDL_KMOD_SHIFT}) {
                    OrbisImeParamExtended settings{};
                    settings.ext_keyboard_mode = 0x04000001U;
                    const auto alphabet = ImeAlphabetKey(static_cast<SDL_Scancode>(scan), modifiers,
                                                         settings, variant);
                    char encoded[5]{};
                    ImTextCharToUtf8(encoded, alphabet);
                    if (alphabet &&
                        (characters == encoded ||
                         characters == TransformImeExternalText(encoded, variant,
                                                                settings.ext_keyboard_mode))) {
                        key = static_cast<SDL_Scancode>(scan);
                        break;
                    }
                }
                if (key != SDL_SCANCODE_UNKNOWN) {
                    break;
                }
            }
        }
    }
    code.keycode = static_cast<u16>(key);
    if (key != SDL_SCANCODE_UNKNOWN) {
        code.status |= static_cast<u32>(OrbisImeKeycodeState::KEYCODE_VALID);
    }
    std::u16string output;
    for (const char* p = characters.data(); p && p < characters.data() + characters.size();) {
        unsigned int character = 0;
        p += ImTextCharFromUtf8(&character, p, characters.data() + characters.size());
        if (character <= 0xffff && (character < 0xd800 || character > 0xdfff)) {
            output += static_cast<char16_t>(character);
        }
    }
    const auto count = std::max<std::size_t>(1, output.size());
    if (keyboard_input.size() + count + 1 > 4096) {
        client->event_overflow = true;
        return;
    }
    KeyboardInput input{};
    input.client = client;
    input.generation = ime_generation;
    input.event.type = SDL_EVENT_KEY_DOWN;
    input.event.key.repeat = repeat;
    for (std::size_t i = 0; i < count; ++i) {
        input.osk_code = code;
        if (!output.empty()) {
            input.osk_code->character = output[i];
        }
        if (i + 1 < count) {
            input.osk_code->status |= static_cast<u32>(OrbisImeKeycodeState::CONTINUOUS_EVENT);
        }
        keyboard_input.push_back(input);
    }
    input.osk_code = code;
    input.event.type = SDL_EVENT_KEY_UP;
    input.osk_code->character = 0;
    input.osk_code->status &= ~static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
    keyboard_input.push_back(std::move(input));
}

void InitializeImeKeyboard() {
    int count = 0;
    auto* keyboards = SDL_GetKeyboards(&count);
    std::vector<u32> resources;
    if (keyboards) {
        resources.assign(keyboards, keyboards + std::min(count, 5));
        SDL_free(keyboards);
    }
    if (resources.empty() && SDL_HasKeyboard()) {
        resources.push_back(1);
    }
    std::scoped_lock lock(input_mutex);
    keyboard_resources = std::move(resources);
}

bool IsImeKeyboardActive() {
    return ime_active;
}

void ConfigureImeKeyboard(bool active, s32 user_id, const OrbisImeParamExtended* extended) {
    std::scoped_lock api_lock(GetImeMutex());
    std::scoped_lock lock(input_mutex);
    ++ime_generation;
    ime_active = active;
    ime_variant = active ? KeyboardVariant() : ImeKbVariant::Automatic;
    ime_dead_key = 0;
    ime_keypad_keys.fill(SDL_SCANCODE_UNKNOWN);
    ime_user_id = user_id;
    ime_extended = extended ? *extended : OrbisImeParamExtended{};
    if (False(ime_extended.option & OrbisImeExtOption::INIT_EXT_KEYBOARD_MODE)) {
        const auto mode = keyboard_client ? keyboard_client->mode : 0U;
        ime_extended.ext_keyboard_mode =
            0x04000000U | ((mode & 32U) ? 0U : 1U) | ((mode & 64U) ? 0U : 2U);
    }
    ime_method_state = ResolveImeExternalKeyboardMode(ime_variant, ime_extended.ext_keyboard_mode);
    ime_shift_pending = false;
    filtered_input.clear();
    discard_filtered_text = false;
}

bool ProcessImeKeyboardEvent(SDL_Event& event) {
    replayed_event = nullptr;
    replayed_context.reset();
    if (event.type == SDL_EVENT_KEYBOARD_ADDED || event.type == SDL_EVENT_KEYBOARD_REMOVED) {
        InitializeImeKeyboard();
    }
    std::scoped_lock lock(input_mutex);
    if (event.type == SDL_EVENT_IME_KEYBOARD) {
        if (filtered_input.empty()) {
            return true;
        }
        auto input = std::move(filtered_input.front());
        filtered_input.pop_front();
        if (input.generation != ime_generation || !ime_active) {
            return true;
        }
        event = input.event;
        const auto variant = input.variant == ImeKbVariant::Automatic ? ime_variant : input.variant;
        auto replay_extended = ime_extended;
        if (input.external_mode) {
            replay_extended.ext_keyboard_mode = *input.external_mode;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && input.normalize_text && ime_dead_key) {
            if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                ime_dead_key = 0;
                return true;
            }
            if (event.key.scancode == SDL_SCANCODE_ESCAPE ||
                event.key.scancode == SDL_SCANCODE_RETURN) {
                ime_dead_key = 0;
            }
        }
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            NormalizeImeKeypadEvent(event.key);
            event.key.key = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, true);
            if (input.synthesize_text && event.type == SDL_EVENT_KEY_DOWN) {
                const auto character =
                    input.normalize_text ? ImeAlphabetKey(event.key.scancode, event.key.mod,
                                                          replay_extended, variant)
                    : (event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI)) == 0
                        ? SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, false)
                        : 0;
                if (character >= 0x20 && character <= 0xffff) {
                    char text[5]{};
                    ImTextCharToUtf8(text, character);
                    KeyboardInput text_input{};
                    text_input.event.type = SDL_EVENT_TEXT_INPUT;
                    text_input.event.text.windowID = event.key.windowID;
                    text_input.text = text;
                    if (input.normalize_text) {
                        text_input.text = ImeExternalKeyText(
                            character, event.key.scancode, replay_extended, variant, ime_dead_key);
                        if (text_input.text.empty()) {
                            return false;
                        }
                    }
                    text_input.generation = input.generation;
                    text_input.external_mode = input.external_mode;
                    text_input.variant = variant;
                    filtered_input.push_front(std::move(text_input));
                    SDL_Event wake{};
                    wake.type = SDL_EVENT_IME_KEYBOARD;
                    if (!SDL_PushEvent(&wake)) {
                        filtered_input.pop_front();
                    }
                }
            }
        }
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            static thread_local std::string text;
            text = std::move(input.text);
            event.text.text = text.c_str();
        }
        if (input.external_mode) {
            replayed_event = &event;
            replayed_context = ImeKeyboardContext{variant, *input.external_mode, input.generation};
        }
        return false;
    }
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP &&
        event.type != SDL_EVENT_TEXT_INPUT && event.type != SDL_EVENT_KEYBOARD_ADDED &&
        event.type != SDL_EVENT_KEYBOARD_REMOVED) {
        return false;
    }
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        if (keyboard_resources.empty()) {
            keyboard_resources.push_back(1);
        }
    }
    const bool route =
        ime_active && (True(ime_extended.option & OrbisImeExtOption::INIT_EXT_KEYBOARD_MODE) ||
                       ime_extended.ext_keyboard_filter ||
                       True(ime_extended.disable_device & OrbisImeDisableDevice::EXT_KEYBOARD));
    if (keyboard_client || route) {
        KeyboardInput input{event, {}, ime_generation};
        input.client = keyboard_client;
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            input.character = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, false);
        }
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            input.text = event.text.text;
        }
        if (keyboard_input.size() < 4096) {
            keyboard_input.push_back(std::move(input));
        } else if (keyboard_client) {
            keyboard_client->event_overflow = true;
        }
    }
    if (ime_active && !route &&
        (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)) {
        NormalizeImeKeypadEvent(event.key);
    }
    return route && (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP ||
                     event.type == SDL_EVENT_TEXT_INPUT);
}

bool HasImeKeyboardHandler(OrbisImeEventHandler handler) {
    std::scoped_lock api_lock(GetImeMutex());
    return keyboard_client && keyboard_client->param.handler == handler;
}

Error DispatchImeKeyboard(OrbisImeEventHandler handler) {
    std::scoped_lock api_lock(GetImeMutex());
    if (dispatching) {
        return Error::OK;
    }
    dispatching = true;
    SCOPE_EXIT {
        dispatching = false;
    };
    std::deque<KeyboardInput> inputs;
    std::shared_ptr<KeyboardClient> client;
    OrbisImeParamExtended extended;
    ImeKbVariant variant;
    s32 user_id;
    u64 generation;
    bool active;
    bool overflow = false;
    std::vector<u32> resources;
    {
        std::scoped_lock lock(input_mutex);
        client = keyboard_client;
        inputs.swap(keyboard_input);
        extended = ime_extended;
        variant = ime_variant;
        user_id = ime_user_id;
        generation = ime_generation;
        active = ime_active;
        if (!inputs.empty()) {
            resources = keyboard_resources;
        }
        if (client && client->param.handler == handler) {
            overflow = client->event_overflow.exchange(false);
        }
    }
    const auto keyboard_type = inputs.empty() ? OrbisImeKeyboardType::NONE : KeyboardType();
    bool discard_text = discard_filtered_text;
    const bool managed_mode = True(extended.option & OrbisImeExtOption::INIT_EXT_KEYBOARD_MODE) ||
                              extended.ext_keyboard_filter;
    const bool normalize_text = managed_mode && ResolveImeExternalKeyboardMode(variant, ~0U) != 0;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        auto input = std::move(inputs[i]);
        const auto type = input.event.type;
        const bool receive = client && input.client.lock() == client;
        if (input.osk_code) {
            if (receive) {
                OrbisImeEvent event{};
                event.id = type == SDL_EVENT_KEY_UP ? OrbisImeEventId::KeyboardKeycodeUp
                           : input.event.key.repeat ? OrbisImeEventId::KeyboardKeycodeRepeat
                                                    : OrbisImeEventId::KeyboardKeycodeDown;
                event.param.keycode = *input.osk_code;
                Libraries::Rtc::sceRtcGetCurrentTick(&event.param.keycode.timestamp);
                client->QueueEvent(event);
            }
            continue;
        }
        if (type == SDL_EVENT_KEYBOARD_ADDED || type == SDL_EVENT_KEYBOARD_REMOVED) {
            if (receive) {
                OrbisImeEvent event{};
                event.id = type == SDL_EVENT_KEYBOARD_ADDED
                               ? OrbisImeEventId::KeyboardConnection
                               : OrbisImeEventId::KeyboardDisconnection;
                event.param.resource_id_array.user_id = client->user_id;
                event.param.resource_id_array.resource_id[0] = input.event.kdevice.which;
                client->QueueEvent(event);
            }
            continue;
        }
        const bool route = active && input.generation == generation;
        if (type == SDL_EVENT_TEXT_INPUT) {
            if (route && managed_mode && !discard_text &&
                False(extended.disable_device & OrbisImeDisableDevice::EXT_KEYBOARD)) {
                input.variant = variant;
                input.external_mode = ResolveImeExternalKeyboardMode(variant, ime_method_state);
                ReplayInput(std::move(input));
            }
            continue;
        }
        OrbisImeKeycode code{};
        code.keycode = static_cast<u16>(input.event.key.scancode);
        code.status = KeyStatus(input.event.key.mod);
        code.type = keyboard_type;
        code.user_id = client ? client->user_id : user_id;
        if (code.user_id == 0xfe) {
            code.user_id = UserManagement.GetDefaultUser().user_id;
        }
        code.resource_id = input.event.key.which;
        if (code.resource_id == 0) {
            if (!resources.empty()) {
                code.resource_id = resources.front();
            }
        }
        Libraries::Rtc::sceRtcGetCurrentTick(&code.timestamp);
        if (type == SDL_EVENT_KEY_DOWN && input.event.key.key >= 0x20 &&
            input.event.key.key <= 0xffff &&
            (input.event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) == 0) {
            auto character = input.character;
            if (character < 0x20 || character > 0xffff) {
                character = input.event.key.key;
            }
            code.character = static_cast<char16_t>(character);
            code.status |= static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
        }
        if (type == SDL_EVENT_KEY_DOWN && i + 1 < inputs.size() &&
            inputs[i + 1].event.type == SDL_EVENT_TEXT_INPUT) {
            unsigned int character = 0;
            ImTextCharFromUtf8(&character, inputs[i + 1].text.c_str(), nullptr);
            if (character > 0 && character <= 0xffff &&
                (character < 0xd800 || character > 0xdfff)) {
                code.character = static_cast<char16_t>(character);
                code.status |= static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
            }
        }
        if (receive &&
            (!active || True(client->param.option & OrbisImeKeyboardOption::EffectiveWithIme)) &&
            (!input.event.key.repeat ||
             True(client->param.option & OrbisImeKeyboardOption::Repeat))) {
            OrbisImeEvent event{};
            event.id = type == SDL_EVENT_KEY_UP ? OrbisImeEventId::KeyboardKeycodeUp
                       : input.event.key.repeat ? OrbisImeEventId::KeyboardKeycodeRepeat
                                                : OrbisImeEventId::KeyboardKeycodeDown;
            event.param.keycode = code;
            QueueKeyboardOutput(*client, event, input.event.key, KeyboardVariant());
        }
        if (!route || True(extended.disable_device & OrbisImeDisableDevice::EXT_KEYBOARD)) {
            continue;
        }
        {
            std::scoped_lock lock(input_mutex);
            if (generation != ime_generation) {
                break;
            }
            if (managed_mode) {
                input.method_key = UpdateKeyboardMethod(variant, input.event.key, ime_method_state,
                                                        ime_shift_pending);
                input.variant = variant;
                extended.ext_keyboard_mode =
                    ResolveImeExternalKeyboardMode(variant, ime_method_state);
                input.external_mode = extended.ext_keyboard_mode;
            }
            code.status |= ime_method_state & 0x1f000000U;
        }
        if (!extended.ext_keyboard_filter) {
            if (managed_mode) {
                input.normalize_text = normalize_text;
                input.synthesize_text = normalize_text && !input.method_key &&
                                        type == SDL_EVENT_KEY_DOWN &&
                                        ImeAlphabetKey(input.event.key.scancode,
                                                       input.event.key.mod, extended, variant) != 0;
                if (type == SDL_EVENT_KEY_DOWN) {
                    discard_text = input.method_key || input.synthesize_text;
                }
                ReplayInput(std::move(input));
            }
            continue;
        }
        code.user_id = user_id;
        const u16 original = code.keycode;
        u16 key = original;
        code.character = 0;
        code.status &= ~static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
        u32 status = code.status;
        const int filtered = extended.ext_keyboard_filter(&code, &key, &status, nullptr);
        {
            std::scoped_lock lock(input_mutex);
            if (generation != ime_generation) {
                break;
            }
        }
        if (filtered != 0) {
            key = original;
            status = code.status;
        }
        {
            std::scoped_lock lock(input_mutex);
            ime_method_state = (ime_method_state & ~0x1c000000U) |
                               ResolveImeExternalKeyboardMode(variant, status & 0x1c000000U);
            extended.ext_keyboard_mode = ResolveImeExternalKeyboardMode(variant, ime_method_state);
            input.external_mode = extended.ext_keyboard_mode;
        }
        const bool rejected = key == 0 || (status & 1U) == 0;
        if (type == SDL_EVENT_KEY_DOWN) {
            discard_text = rejected;
        }
        if (rejected || key >= SDL_SCANCODE_COUNT) {
            continue;
        }
        input.event.key.scancode = static_cast<SDL_Scancode>(key);
        input.event.key.mod = KeyModifiers(status);
        const bool changed = key != original || status != code.status;
        input.normalize_text = normalize_text;
        input.synthesize_text =
            type == SDL_EVENT_KEY_DOWN && (!input.method_key || key != original) &&
            (changed ||
             (normalize_text && ImeAlphabetKey(input.event.key.scancode, input.event.key.mod,
                                               extended, variant) != 0));
        if (input.synthesize_text || input.method_key) {
            discard_text = true;
        }
        ReplayInput(std::move(input));
    }
    {
        std::scoped_lock lock(input_mutex);
        if (generation == ime_generation) {
            discard_filtered_text = discard_text;
        }
        if (client && client->param.handler == handler) {
            overflow |= client->event_overflow.exchange(false);
        }
    }
    while (client && keyboard_client == client && client->param.handler == handler &&
           !client->events.empty()) {
        const auto event = client->events.front();
        client->events.pop_front();
        client->param.handler(client->param.arg, &event);
    }
    return overflow ? Error::EVENT_OVERFLOW : Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardOpen(s32 user_id, const OrbisImeKeyboardParam* param) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!param) {
        return Error::INVALID_ADDRESS;
    }
    if (!param->handler) {
        return Error::INVALID_HANDLER;
    }
    if (keyboard_client) {
        return Error::BUSY;
    }
    if (!ValidUser(user_id)) {
        return Error::INVALID_USER_ID;
    }
    s32 sdk = 0;
    Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk);
    const u32 allowed = 0xfU | (sdk > 0x24fffff ? 0x10U : 0U) | (sdk > 0x34fffff ? 0x20U : 0U);
    if ((static_cast<u32>(param->option) & ~allowed) != 0) {
        return Error::INVALID_OPTION;
    }
    if (std::ranges::any_of(param->reserved1, [](auto value) { return value != 0; }) ||
        std::ranges::any_of(param->reserved2, [](auto value) { return value != 0; })) {
        return Error::INVALID_RESERVED;
    }
    auto client = std::make_shared<KeyboardClient>();
    client->user_id = user_id;
    client->param = *param;
    OrbisImeEvent event{};
    event.id = OrbisImeEventId::KeyboardOpen;
    event.param.resource_id_array.user_id = user_id;
    const auto resources = KeyboardResources();
    std::copy(resources.begin(), resources.end(), event.param.resource_id_array.resource_id);
    client->QueueEvent(event);
    std::scoped_lock lock(input_mutex);
    keyboard_client = std::move(client);
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeKeyboardClose(s32 user_id) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!keyboard_client) {
        return Error::NOT_OPENED;
    }
    if (keyboard_client->user_id != user_id) {
        return Error::INVALID_USER_ID;
    }
    std::scoped_lock lock(input_mutex);
    keyboard_client.reset();
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardGetInfo(u32 resource_id, OrbisImeKeyboardInfo* info) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!keyboard_client) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    if (!info) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }
    const auto resources = KeyboardResources();
    if (std::ranges::find(resources, resource_id) == resources.end()) {
        return static_cast<int>(Error::NO_RESOURCE_ID);
    }
    *info = {};
    info->user_id = keyboard_client->user_id;
    info->device = OrbisImeKeyboardDeviceType::Keyboard;
    info->type = KeyboardType();
    info->repeat_delay = 500;
    info->repeat_rate = 30;
    info->status = OrbisImeKeyboardStatus::Connected;
    return static_cast<int>(Error::OK);
}

Error PS4_SYSV_ABI sceImeKeyboardGetResourceId(s32 user_id,
                                               OrbisImeKeyboardResourceIdArray* resources) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!keyboard_client) {
        return Error::NOT_OPENED;
    }
    if (!resources) {
        return Error::INVALID_ADDRESS;
    }
    if (!ValidUser(user_id) || (keyboard_client->user_id != user_id &&
                                keyboard_client->user_id != 0xfe && user_id != 0xfe)) {
        return Error::INVALID_USER_ID;
    }
    *resources = {};
    resources->user_id = user_id;
    const auto ids = KeyboardResources();
    std::copy(ids.begin(), ids.end(), resources->resource_id);
    return Error::OK;
}

int PS4_SYSV_ABI sceImeKeyboardSetMode(s32 user_id, u32 mode) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!keyboard_client) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    s32 sdk = 0;
    Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk);
    const u32 allowed_mode = sdk < 0x02000000 ? 0x1fU : 0x7fU;
    if ((mode & ~allowed_mode) != 0) {
        return static_cast<int>(Error::INVALID_MODE);
    }
    if (keyboard_client->user_id != user_id) {
        return static_cast<int>(Error::INVALID_USER_ID);
    }
    keyboard_client->mode = mode;
    keyboard_client->previous_character = 0;
    keyboard_client->shift_pending = false;
    return static_cast<int>(Error::OK);
}

int PS4_SYSV_ABI sceImeKeyboardUpdate(OrbisImeEventHandler handler) {
    std::scoped_lock api_lock(GetImeMutex());
    if (!HasImeKeyboardHandler(handler)) {
        return static_cast<int>(Error::NOT_OPENED);
    }
    return static_cast<int>(DispatchImeKeyboard(handler));
}

} // namespace Libraries::Ime
