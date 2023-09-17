#include "emulator.h"

#include "Core/PS4/HLE/Graphics/video_out.h"
#include <vulkan_util.h>

namespace Emulator {

static WindowCtx* g_window_ctx = nullptr;
bool m_emu_needs_exit = false;

void emuInit(u32 width, u32 height) {
    g_window_ctx = new WindowCtx;

    g_window_ctx->m_graphic_ctx.screen_width = width;
    g_window_ctx->m_graphic_ctx.screen_height = height;
}

void checkAndWaitForGraphicsInit() {
    Lib::LockMutexGuard lock(g_window_ctx->m_mutex);

    while (!g_window_ctx->m_is_graphic_initialized) {
        g_window_ctx->m_graphic_initialized_cond.WaitCondVar(&g_window_ctx->m_mutex);
    }
}
static void CreateSdlWindow(WindowCtx* ctx) {
    int width = static_cast<int>(ctx->m_graphic_ctx.screen_width);
    int height = static_cast<int>(ctx->m_graphic_ctx.screen_height);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("%s\n", SDL_GetError());
        std::exit(0);
    }

    ctx->m_window = SDL_CreateWindowWithPosition("shadps4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                                                 (static_cast<uint32_t>(SDL_WINDOW_HIDDEN) | static_cast<uint32_t>(SDL_WINDOW_VULKAN)));

    ctx->is_window_hidden = true;  // hide window until we need to show something (should draw something in buffers)

    if (ctx->m_window == nullptr) {
        printf("%s\n", SDL_GetError());
        std::exit(0);
    }

    SDL_SetWindowResizable(ctx->m_window, SDL_FALSE);  // we don't support resizable atm
}
void emuRun() {
    g_window_ctx->m_mutex.LockMutex();
    {
        // init window and wait until init finishes
        CreateSdlWindow(g_window_ctx);
        Graphics::Vulkan::vulkanCreate(g_window_ctx);
        g_window_ctx->m_is_graphic_initialized = true;
        g_window_ctx->m_graphic_initialized_cond.SignalCondVar();
    }
    g_window_ctx->m_mutex.UnlockMutex();

    bool exit_loop = false;

    for (;;) {
        if (exit_loop) {
            break;
        }

        SDL_Event event;
        if (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_EVENT_QUIT: m_emu_needs_exit = true; break;

                case SDL_EVENT_TERMINATING: m_emu_needs_exit = true; break;

                case SDL_EVENT_WILL_ENTER_BACKGROUND: break;

                case SDL_EVENT_DID_ENTER_BACKGROUND: break;

                case SDL_EVENT_WILL_ENTER_FOREGROUND: break;

                case SDL_EVENT_DID_ENTER_FOREGROUND: break;

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP: break;
            }
            continue;
        }
        exit_loop = m_emu_needs_exit;

        if (!exit_loop) {
            HLE::Libs::Graphics::VideoOut::videoOutFlip(100000);  // flip every 0.1 sec
        }
    }
    std::exit(0);
}
}  // namespace Emulator