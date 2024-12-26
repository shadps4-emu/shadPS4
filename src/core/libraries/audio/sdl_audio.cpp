// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include "common/assert.h"
#include "core/libraries/audio/sdl_audio.h"

namespace Libraries::AudioOut {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

void* SDLAudioOut::Open(bool is_float, int num_channels, u32 sample_rate) {
    SDL_AudioSpec fmt;
    SDL_zero(fmt);
    fmt.format = is_float ? SDL_AUDIO_F32 : SDL_AUDIO_S16;
    fmt.channels = num_channels;
    fmt.freq = sample_rate;

    auto* stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, nullptr, nullptr);
    SDL_ResumeAudioStreamDevice(stream);
    return stream;
}

void SDLAudioOut::Close(void* impl) {
    SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(impl));
}

void SDLAudioOut::Output(void* impl, const void* ptr, size_t size) {
    auto* stream = static_cast<SDL_AudioStream*>(impl);
    SDL_PutAudioStreamData(stream, ptr, size);
    while (SDL_GetAudioStreamAvailable(stream) > AUDIO_STREAM_BUFFER_THRESHOLD) {
        SDL_Delay(0);
    }
}

void SDLAudioOut::SetVolume(void* impl, std::array<int, 8> ch_volumes) {
    // Not yet implemented
}

} // namespace Libraries::AudioOut
