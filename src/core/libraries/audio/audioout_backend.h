// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Libraries::AudioOut {

class AudioOutBackend {
public:
    AudioOutBackend() = default;
    virtual ~AudioOutBackend() = default;

    virtual void* Open(bool is_float, int num_channels, u32 sample_rate) = 0;
    virtual void Close(void* impl) = 0;
    virtual void Output(void* impl, const void* ptr, size_t size) = 0;
    virtual void SetVolume(void* impl, std::array<int, 8> ch_volumes) = 0;
};

} // namespace Libraries::AudioOut
