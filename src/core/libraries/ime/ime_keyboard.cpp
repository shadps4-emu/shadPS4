// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
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
    std::weak_ptr<KeyboardClient> client;
};

struct KeyboardClient {
    s32 user_id;
    OrbisImeKeyboardParam param;
    u32 mode = 0;
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
u64 ime_generation = 0;
bool dispatching = false;
bool discard_filtered_text = false;

OrbisImeKeyboardType KeyboardType() {
    switch (EmulatorSettings.GetConsoleLanguage()) {
    case 16:
        return OrbisImeKeyboardType::POLISH;
    case 18:
        return OrbisImeKeyboardType::ENGLISH_GB;
    default:
        return OrbisImeKeyboardType::ENGLISH_US;
    }
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
    ime_user_id = user_id;
    ime_extended = extended ? *extended : OrbisImeParamExtended{};
    filtered_input.clear();
    discard_filtered_text = false;
}

bool ProcessImeKeyboardEvent(SDL_Event& event) {
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
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            event.key.key = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, true);
            if (input.synthesize_text && event.type == SDL_EVENT_KEY_DOWN &&
                (event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI)) == 0) {
                const auto character =
                    SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, false);
                if (character >= 0x20 && character <= 0xffff) {
                    char text[5]{};
                    ImTextCharToUtf8(text, character);
                    KeyboardInput text_input{};
                    text_input.event.type = SDL_EVENT_TEXT_INPUT;
                    text_input.event.text.windowID = event.key.windowID;
                    text_input.text = text;
                    text_input.generation = input.generation;
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
        ime_active && (ime_extended.ext_keyboard_filter ||
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
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        auto input = std::move(inputs[i]);
        const auto type = input.event.type;
        const bool receive = client && input.client.lock() == client;
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
            if (route && extended.ext_keyboard_filter && !discard_text &&
                False(extended.disable_device & OrbisImeDisableDevice::EXT_KEYBOARD)) {
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
            if ((client->mode & 3U) == 1U && event.param.keycode.character > 0x7f) {
                const auto scancode = input.event.key.scancode;
                char16_t alphabet = 0;
                if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
                    const bool uppercase = ((input.event.key.mod & SDL_KMOD_SHIFT) != 0) !=
                                           ((input.event.key.mod & SDL_KMOD_CAPS) != 0);
                    alphabet = (uppercase ? u'A' : u'a') + scancode - SDL_SCANCODE_A;
                }
                event.param.keycode.character = alphabet;
                if (!alphabet) {
                    event.param.keycode.status &=
                        ~static_cast<u32>(OrbisImeKeycodeState::CHARACTER_VALID);
                }
            }
            client->QueueEvent(event);
        }
        if (!route || !extended.ext_keyboard_filter ||
            True(extended.disable_device & OrbisImeDisableDevice::EXT_KEYBOARD)) {
            continue;
        }
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
        input.synthesize_text = type == SDL_EVENT_KEY_DOWN && changed;
        if (input.synthesize_text) {
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
    if (keyboard_client->user_id != user_id) {
        return static_cast<int>(Error::INVALID_USER_ID);
    }
    if ((mode & ~0x7fU) != 0) {
        return static_cast<int>(Error::INVALID_MODE);
    }
    keyboard_client->mode = mode;
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
