// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL.h>
#include "sdl_audio.h"

int SDLAudio::AudioInit() {
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}

int SDLAudio::AudioOutOpen(int type, u32 samples_num, u32 freq, u32 format) {
    std::scoped_lock{m_mutex};
    for (int id = 0; id < portsOut.size(); id++) {
        auto& port = portsOut[id];
        if (!port.isOpen) {
            port.isOpen = true;
            port.type = type;
            port.samples_num = samples_num;
            port.freq = freq;
            port.format = format;
        }
        return id + 1;
    }

    return -1; // all ports are used
}
