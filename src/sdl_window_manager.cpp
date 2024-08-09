// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_events.h>

namespace Frontend {

void QuitAllSDLWindows() {
    // In the future, windows would have to be stored accessibly outside functions to close just a
    // specific window
    SDL_Event quitEvent;
    quitEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quitEvent);
}

} // namespace Frontend