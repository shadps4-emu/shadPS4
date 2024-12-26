// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Libraries::AudioOut {

struct PortOut;

class AudioOutBackend {
public:
    AudioOutBackend() = default;
    virtual ~AudioOutBackend() = default;

    virtual void* Open(PortOut& port) = 0;
    virtual void Close(void* impl) = 0;
    virtual void Output(void* impl, void* ptr, size_t size) = 0;
    virtual void SetVolume(void* impl, const std::array<int, 8>& ch_volumes) = 0;
};

} // namespace Libraries::AudioOut
