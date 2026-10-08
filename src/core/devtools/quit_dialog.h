// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <set>
#include <utility>
#include <SDL3/SDL_events.h>

namespace Core::Devtools {

// Event handling runs on the SDL thread. The renderer and guest pad reads only query atomics.
class QuitDialog {
public:
    enum class Action { None, Opened, Cancelled, Confirmed };
    struct Result {
        bool consumed = false;
        Action action = Action::None;
    };

    bool IsVisible() const {
        return visible.load(std::memory_order_relaxed);
    }

    bool CapturesGamepad() const {
        return capture.load(std::memory_order_relaxed);
    }

    Action Toggle() {
        return SetVisible(!IsVisible());
    }

    Result ProcessEvent(const SDL_Event& event) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            const auto key = std::pair{event.gbutton.which, event.gbutton.button};
            if (buttons.contains(key)) {
                return {true};
            }
            const auto button = event.gbutton.button;
            if (button == SDL_GAMEPAD_BUTTON_GUIDE) {
                buttons.insert(key);
                return {true, Toggle()};
            }
            if (IsVisible() &&
                (button == SDL_GAMEPAD_BUTTON_SOUTH || button == SDL_GAMEPAD_BUTTON_EAST)) {
                buttons.insert(key);
                return {true,
                        button == SDL_GAMEPAD_BUTTON_EAST ? SetVisible(false) : Action::Confirmed};
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (buttons.erase({event.gbutton.which, event.gbutton.button})) {
                UpdateCapture();
                return {true};
            }
            if (event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
                return {true};
            }
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            std::erase_if(buttons,
                          [&](const auto& key) { return key.first == event.gdevice.which; });
            UpdateCapture();
            break;
        case SDL_EVENT_KEY_DOWN:
            if (keys.contains(event.key.scancode)) {
                return {true};
            }
            if (IsVisible() && (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN ||
                                event.key.key == SDLK_KP_ENTER)) {
                // A key held before opening must be released and pressed again to confirm.
                if (event.key.repeat) {
                    return {true};
                }
                keys.insert(event.key.scancode);
                return {true, event.key.key == SDLK_ESCAPE ? SetVisible(false) : Action::Confirmed};
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (keys.erase(event.key.scancode)) {
                UpdateCapture();
                return {true};
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            keys.clear();
            UpdateCapture();
            break;
        default:
            break;
        }
        return {};
    }

private:
    Action SetVisible(bool value) {
        if (value) {
            capture.store(true, std::memory_order_relaxed);
        }
        visible.store(value, std::memory_order_relaxed);
        UpdateCapture();
        return value ? Action::Opened : Action::Cancelled;
    }

    void UpdateCapture() {
        // Keep the cancel/guide button intercepted until its release, too.
        capture.store(IsVisible() || !buttons.empty() || !keys.empty(), std::memory_order_relaxed);
    }

    std::atomic<bool> visible{false};
    std::atomic<bool> capture{false};
    std::set<std::pair<SDL_JoystickID, Uint8>> buttons;
    std::set<SDL_Scancode> keys;
};

} // namespace Core::Devtools
