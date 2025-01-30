// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "common/assert.h"
#include "common/config.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/pad/pad.h"
#include "imgui/renderer/imgui_core.h"
#include "input/controller.h"
#include "sdl_window.h"
#include "video_core/renderdoc.h"

#ifdef __APPLE__
#include <SDL3/SDL_metal.h>
#endif

namespace Input {

using Libraries::Pad::OrbisPadButtonDataOffset;

static OrbisPadButtonDataOffset SDLGamepadToOrbisButton(u8 button) {
    using OPBDO = OrbisPadButtonDataOffset;

    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return OPBDO::Down;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return OPBDO::Up;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return OPBDO::Left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return OPBDO::Right;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return OPBDO::Cross;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return OPBDO::Triangle;
    case SDL_GAMEPAD_BUTTON_WEST:
        return OPBDO::Square;
    case SDL_GAMEPAD_BUTTON_EAST:
        return OPBDO::Circle;
    case SDL_GAMEPAD_BUTTON_START:
        return OPBDO::Options;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_BACK:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return OPBDO::L1;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return OPBDO::R1;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return OPBDO::L3;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return OPBDO::R3;
    default:
        return OPBDO::None;
    }
}

static SDL_GamepadAxis InputAxisToSDL(Axis axis) {
    switch (axis) {
    case Axis::LeftX:
        return SDL_GAMEPAD_AXIS_LEFTX;
    case Axis::LeftY:
        return SDL_GAMEPAD_AXIS_LEFTY;
    case Axis::RightX:
        return SDL_GAMEPAD_AXIS_RIGHTX;
    case Axis::RightY:
        return SDL_GAMEPAD_AXIS_RIGHTY;
    case Axis::TriggerLeft:
        return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    case Axis::TriggerRight:
        return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    default:
        UNREACHABLE();
    }
}

SDLInputEngine::~SDLInputEngine() {
    if (m_gamepad) {
        SDL_CloseGamepad(m_gamepad);
    }
}

void SDLInputEngine::Init() {
    if (m_gamepad) {
        SDL_CloseGamepad(m_gamepad);
        m_gamepad = nullptr;
    }
    int gamepad_count;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepad_count);
    if (!gamepads) {
        LOG_ERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
        return;
    }
    if (gamepad_count == 0) {
        LOG_INFO(Input, "No gamepad found!");
        SDL_free(gamepads);
        return;
    }
    LOG_INFO(Input, "Got {} gamepads. Opening the first one.", gamepad_count);
    if (!(m_gamepad = SDL_OpenGamepad(gamepads[0]))) {
        LOG_ERROR(Input, "Failed to open gamepad 0: {}", SDL_GetError());
        SDL_free(gamepads);
        return;
    }
    if (Config::getIsMotionControlsEnabled()) {
        if (SDL_SetGamepadSensorEnabled(m_gamepad, SDL_SENSOR_GYRO, true)) {
            m_gyro_poll_rate = SDL_GetGamepadSensorDataRate(m_gamepad, SDL_SENSOR_GYRO);
            LOG_INFO(Input, "Gyro initialized, poll rate: {}", m_gyro_poll_rate);
        } else {
            LOG_ERROR(Input, "Failed to initialize gyro controls for gamepad");
        }
        if (SDL_SetGamepadSensorEnabled(m_gamepad, SDL_SENSOR_ACCEL, true)) {
            m_accel_poll_rate = SDL_GetGamepadSensorDataRate(m_gamepad, SDL_SENSOR_ACCEL);
            LOG_INFO(Input, "Accel initialized, poll rate: {}", m_accel_poll_rate);
        } else {
            LOG_ERROR(Input, "Failed to initialize accel controls for gamepad");
        };
    }
    SDL_free(gamepads);
    SetLightBarRGB(0, 0, 255);
}

void SDLInputEngine::SetLightBarRGB(u8 r, u8 g, u8 b) {
    if (m_gamepad) {
        SDL_SetGamepadLED(m_gamepad, r, g, b);
    }
}

void SDLInputEngine::SetVibration(u8 smallMotor, u8 largeMotor) {
    if (m_gamepad) {
        const auto low_freq = (smallMotor / 255.0f) * 0xFFFF;
        const auto high_freq = (largeMotor / 255.0f) * 0xFFFF;
        SDL_RumbleGamepad(m_gamepad, low_freq, high_freq, -1);
    }
}

State SDLInputEngine::ReadState() {
    State state{};
    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    // Buttons
    for (u8 i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i) {
        auto orbisButton = SDLGamepadToOrbisButton(i);
        if (orbisButton == OrbisPadButtonDataOffset::None) {
            continue;
        }
        state.OnButton(orbisButton, SDL_GetGamepadButton(m_gamepad, (SDL_GamepadButton)i));
    }

    // Axes
    for (int i = 0; i < static_cast<int>(Axis::AxisMax); ++i) {
        const auto axis = static_cast<Axis>(i);
        const auto value = SDL_GetGamepadAxis(m_gamepad, InputAxisToSDL(axis));
        switch (axis) {
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            state.OnAxis(axis, GetAxis(0, 0x8000, value));
            break;
        default:
            state.OnAxis(axis, GetAxis(-0x8000, 0x8000, value));
            break;
        }
    }

    // Touchpad
    if (SDL_GetNumGamepadTouchpads(m_gamepad) > 0) {
        for (int finger = 0; finger < 2; ++finger) {
            bool down;
            float x, y;
            if (SDL_GetGamepadTouchpadFinger(m_gamepad, 0, finger, &down, &x, &y, NULL)) {
                state.OnTouchpad(finger, down, x, y);
            }
        }
    }

    // Gyro
    if (SDL_GamepadHasSensor(m_gamepad, SDL_SENSOR_GYRO)) {
        float gyro[3];
        if (SDL_GetGamepadSensorData(m_gamepad, SDL_SENSOR_GYRO, gyro, 3)) {
            state.OnGyro(gyro);
        }
    }

    // Accel
    if (SDL_GamepadHasSensor(m_gamepad, SDL_SENSOR_ACCEL)) {
        float accel[3];
        if (SDL_GetGamepadSensorData(m_gamepad, SDL_SENSOR_ACCEL, accel, 3)) {
            state.OnAccel(accel);
        }
    }

    return state;
}

float SDLInputEngine::GetGyroPollRate() const {
    return m_gyro_poll_rate;
}

float SDLInputEngine::GetAccelPollRate() const {
    return m_accel_poll_rate;
}

} // namespace Input

namespace Frontend {

using namespace Libraries::Pad;

static Uint32 SDLCALL PollController(void* userdata, SDL_TimerID timer_id, Uint32 interval) {
    auto* controller = reinterpret_cast<Input::GameController*>(userdata);
    return controller->Poll();
}

WindowSDL::WindowSDL(s32 width_, s32 height_, Input::GameController* controller_,
                     std::string_view window_title)
    : width{width_}, height{height_}, controller{controller_} {
    if (!SDL_SetHint(SDL_HINT_APP_NAME, "shadPS4")) {
        UNREACHABLE_MSG("Failed to set SDL window hint: {}", SDL_GetError());
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        UNREACHABLE_MSG("Failed to initialize SDL video subsystem: {}", SDL_GetError());
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          std::string(window_title).c_str());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetNumberProperty(props, "flags", SDL_WINDOW_VULKAN);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (window == nullptr) {
        UNREACHABLE_MSG("Failed to create window handle: {}", SDL_GetError());
    }

    SDL_SetWindowMinimumSize(window, 640, 360);

    bool error = false;
    const SDL_DisplayID displayIndex = SDL_GetDisplayForWindow(window);
    if (displayIndex < 0) {
        LOG_ERROR(Frontend, "Error getting display index: {}", SDL_GetError());
        error = true;
    }
    const SDL_DisplayMode* displayMode;
    if ((displayMode = SDL_GetCurrentDisplayMode(displayIndex)) == 0) {
        LOG_ERROR(Frontend, "Error getting display mode: {}", SDL_GetError());
        error = true;
    }
    if (!error) {
        SDL_SetWindowFullscreenMode(window,
                                    Config::getFullscreenMode() == "True" ? displayMode : NULL);
    }
    SDL_SetWindowFullscreen(window, Config::getIsFullscreen());

    SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    controller->SetEngine(std::make_unique<Input::SDLInputEngine>());

#if defined(SDL_PLATFORM_WIN32)
    window_info.type = WindowSystemType::Windows;
    window_info.render_surface = SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                        SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        window_info.type = WindowSystemType::X11;
        window_info.display_connection = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        window_info.render_surface = (void*)SDL_GetNumberProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        window_info.type = WindowSystemType::Wayland;
        window_info.display_connection = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        window_info.render_surface = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    }
#elif defined(SDL_PLATFORM_MACOS)
    window_info.type = WindowSystemType::Metal;
    window_info.render_surface = SDL_Metal_GetLayer(SDL_Metal_CreateView(window));
#endif
}

WindowSDL::~WindowSDL() = default;

void WindowSDL::WaitEvent() {
    // Called on main thread
    SDL_Event event;

    if (!SDL_WaitEvent(&event)) {
        return;
    }

    if (ImGui::Core::ProcessEvent(&event)) {
        return;
    }

    switch (event.type) {
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
        OnResize();
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_EXPOSED:
        is_shown = event.type == SDL_EVENT_WINDOW_EXPOSED;
        OnResize();
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        OnKeyPress(&event);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        OnGamepadEvent(&event);
        break;
    // i really would have appreciated ANY KIND OF DOCUMENTATION ON THIS
    // AND IT DOESN'T EVEN USE PROPER ENUMS
    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
        switch ((SDL_SensorType)event.gsensor.sensor) {
        case SDL_SENSOR_GYRO:
            controller->Gyro(0, event.gsensor.data);
            break;
        case SDL_SENSOR_ACCEL:
            controller->Acceleration(0, event.gsensor.data);
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_QUIT:
        is_open = false;
        break;
    default:
        break;
    }
}

void WindowSDL::InitTimers() {
    SDL_AddTimer(100, &PollController, controller);
}

void WindowSDL::RequestKeyboard() {
    if (keyboard_grab == 0) {
        SDL_RunOnMainThread(
            [](void* userdata) { SDL_StartTextInput(static_cast<SDL_Window*>(userdata)); }, window,
            true);
    }
    keyboard_grab++;
}

void WindowSDL::ReleaseKeyboard() {
    ASSERT(keyboard_grab > 0);
    keyboard_grab--;
    if (keyboard_grab == 0) {
        SDL_RunOnMainThread(
            [](void* userdata) { SDL_StopTextInput(static_cast<SDL_Window*>(userdata)); }, window,
            true);
    }
}

void WindowSDL::OnResize() {
    SDL_GetWindowSizeInPixels(window, &width, &height);
    ImGui::Core::OnResize();
}

void WindowSDL::OnKeyPress(const SDL_Event* event) {
    auto button = OrbisPadButtonDataOffset::None;
    Input::Axis axis = Input::Axis::AxisMax;
    int axisvalue = 0;
    int ax = 0;
    std::string backButtonBehavior = Config::getBackButtonBehavior();
    switch (event->key.key) {
    case SDLK_UP:
        button = OrbisPadButtonDataOffset::Up;
        break;
    case SDLK_DOWN:
        button = OrbisPadButtonDataOffset::Down;
        break;
    case SDLK_LEFT:
        button = OrbisPadButtonDataOffset::Left;
        break;
    case SDLK_RIGHT:
        button = OrbisPadButtonDataOffset::Right;
        break;
    // Provide alternatives for face buttons for users without a numpad.
    case SDLK_KP_8:
    case SDLK_C:
        button = OrbisPadButtonDataOffset::Triangle;
        break;
    case SDLK_KP_6:
    case SDLK_B:
        button = OrbisPadButtonDataOffset::Circle;
        break;
    case SDLK_KP_2:
    case SDLK_N:
        button = OrbisPadButtonDataOffset::Cross;
        break;
    case SDLK_KP_4:
    case SDLK_V:
        button = OrbisPadButtonDataOffset::Square;
        break;
    case SDLK_RETURN:
        button = OrbisPadButtonDataOffset::Options;
        break;
    case SDLK_A:
        axis = Input::Axis::LeftX;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += -127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_D:
        axis = Input::Axis::LeftX;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_W:
        axis = Input::Axis::LeftY;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += -127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_S:
        axis = Input::Axis::LeftY;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_J:
        axis = Input::Axis::RightX;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += -127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_L:
        axis = Input::Axis::RightX;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_I:
        axis = Input::Axis::RightY;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += -127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_K:
        axis = Input::Axis::RightY;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 127;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(-0x80, 0x80, axisvalue);
        break;
    case SDLK_X:
        button = OrbisPadButtonDataOffset::L3;
        break;
    case SDLK_M:
        button = OrbisPadButtonDataOffset::R3;
        break;
    case SDLK_Q:
        button = OrbisPadButtonDataOffset::L1;
        break;
    case SDLK_U:
        button = OrbisPadButtonDataOffset::R1;
        break;
    case SDLK_E:
        button = OrbisPadButtonDataOffset::L2;
        axis = Input::Axis::TriggerLeft;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 255;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(0, 0x80, axisvalue);
        break;
    case SDLK_O:
        button = OrbisPadButtonDataOffset::R2;
        axis = Input::Axis::TriggerRight;
        if (event->type == SDL_EVENT_KEY_DOWN) {
            axisvalue += 255;
        } else {
            axisvalue = 0;
        }
        ax = Input::GetAxis(0, 0x80, axisvalue);
        break;
    case SDLK_SPACE:
        if (backButtonBehavior != "none") {
            float x = backButtonBehavior == "left" ? 0.25f
                                                   : (backButtonBehavior == "right" ? 0.75f : 0.5f);
            // trigger a touchpad event so that the touchpad emulation for back button works
            controller->SetTouchpadState(0, true, x, 0.5f);
            button = OrbisPadButtonDataOffset::TouchPad;
        } else {
            button = {};
        }
        break;
    case SDLK_F11:
        if (event->type == SDL_EVENT_KEY_DOWN) {
            {
                SDL_WindowFlags flag = SDL_GetWindowFlags(window);
                bool is_fullscreen = flag & SDL_WINDOW_FULLSCREEN;
                SDL_SetWindowFullscreen(window, !is_fullscreen);
            }
        }
        break;
    case SDLK_F12:
        if (event->type == SDL_EVENT_KEY_DOWN) {
            // Trigger rdoc capture
            VideoCore::TriggerCapture();
        }
        break;
    default:
        break;
    }
    if (button != OrbisPadButtonDataOffset::None) {
        controller->CheckButton(0, button, event->type == SDL_EVENT_KEY_DOWN);
    }
    if (axis != Input::Axis::AxisMax) {
        controller->Axis(0, axis, ax);
    }
}

void WindowSDL::OnGamepadEvent(const SDL_Event* event) {
    auto button = OrbisPadButtonDataOffset::None;
    Input::Axis axis = Input::Axis::AxisMax;
    switch (event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
        controller->SetEngine(std::make_unique<Input::SDLInputEngine>());
        break;
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        controller->SetTouchpadState(event->gtouchpad.finger,
                                     event->type != SDL_EVENT_GAMEPAD_TOUCHPAD_UP,
                                     event->gtouchpad.x, event->gtouchpad.y);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
        button = Input::SDLGamepadToOrbisButton(event->gbutton.button);
        if (button == OrbisPadButtonDataOffset::None) {
            break;
        }
        if (event->gbutton.button != SDL_GAMEPAD_BUTTON_BACK) {
            controller->CheckButton(0, button, event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            break;
        }
        const auto backButtonBehavior = Config::getBackButtonBehavior();
        if (backButtonBehavior != "none") {
            float x = backButtonBehavior == "left" ? 0.25f
                                                   : (backButtonBehavior == "right" ? 0.75f : 0.5f);
            // trigger a touchpad event so that the touchpad emulation for back button works
            controller->SetTouchpadState(0, true, x, 0.5f);
            controller->CheckButton(0, button, event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        axis = event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX           ? Input::Axis::LeftX
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY         ? Input::Axis::LeftY
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTX        ? Input::Axis::RightX
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY        ? Input::Axis::RightY
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER  ? Input::Axis::TriggerLeft
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ? Input::Axis::TriggerRight
                                                                     : Input::Axis::AxisMax;
        if (axis != Input::Axis::AxisMax) {
            if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
                event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                controller->Axis(0, axis, Input::GetAxis(0, 0x8000, event->gaxis.value));

            } else {
                controller->Axis(0, axis, Input::GetAxis(-0x8000, 0x8000, event->gaxis.value));
            }
        }
        break;
    }
}

} // namespace Frontend
