#pragma once
#include <Core/PS4/HLE/Graphics/graphics_ctx.h>
#include <Lib/Threads.h>
#include <SDL.h>

namespace Emulator {
struct WindowCtx {
    HLE::Libs::Graphics::GraphicCtx m_graphic_ctx;
    Lib::Mutex m_mutex;
    bool m_is_graphic_initialized = false;
    Lib::ConditionVariable m_graphic_initialized_cond;
    SDL_Window* m_window = nullptr;
    bool is_window_hidden = true;
};

struct EmuPrivate {
    EmuPrivate() = default;
    Lib::Mutex m_mutex;
    HLE::Libs::Graphics::GraphicCtx* m_graphic_ctx = nullptr;
    void* data1 = nullptr;
    void* data2 = nullptr;
    u32 m_screen_width = {0};
    u32 m_screen_height = {0};
};
void emuInit(u32 width, u32 height);
void emuRun();
void checkAndWaitForGraphicsInit();
}  // namespace Emulator