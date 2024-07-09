// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct SDL_Window;
union SDL_Event;

namespace Input {
class GameController;
}

namespace Frontend {

enum class WindowSystemType : u8 {
    Headless,
    Windows,
    X11,
    Wayland,
    Metal,
};

struct WindowSystemInfo {
    // Window system type. Determines which GL context or Vulkan WSI is used.
    WindowSystemType type = WindowSystemType::Headless;

    // Connection to a display server. This is used on X11 and Wayland platforms.
    void* display_connection = nullptr;

    // Render surface. This is a pointer to the native window handle, which depends
    // on the platform. e.g. HWND for Windows, Window for X11. If the surface is
    // set to nullptr, the video backend will run in headless mode.
    void* render_surface = nullptr;

    // Scale of the render surface. For hidpi systems, this will be >1.
    float render_surface_scale = 1.0f;
};

class WindowSDL {
public:
    explicit WindowSDL(s32 width, s32 height, Input::GameController* controller);
    ~WindowSDL();

    s32 getWidth() const {
        return width;
    }

    s32 getHeight() const {
        return height;
    }

    bool isOpen() const {
        return is_open;
    }

    WindowSystemInfo getWindowInfo() const {
        return window_info;
    }

    void waitEvent();

private:
    void onResize();
    void onKeyPress(const SDL_Event* event);

private:
    s32 width;
    s32 height;
    Input::GameController* controller;
    WindowSystemInfo window_info{};
    SDL_Window* window{};
    bool is_shown{};
    bool is_open{true};
};

} // namespace Frontend
