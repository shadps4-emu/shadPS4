// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/audio/audioout_backend.h"

namespace Libraries::AudioOut {

class SDLAudioOut final : public AudioOutBackend {
public:
    void* Open(bool is_float, int num_channels, u32 sample_rate) override;
    void Close(void* impl) override;
    void Output(void* impl, const void* ptr, size_t size) override;
    void SetVolume(void* impl, std::array<int, 8> ch_volumes) override;
};

} // namespace Libraries::AudioOut
