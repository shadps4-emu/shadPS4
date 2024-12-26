// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/audio/audioout_backend.h"

typedef struct cubeb cubeb;

namespace Libraries::AudioOut {

class CubebAudioOut final : public AudioOutBackend {
public:
    CubebAudioOut();
    ~CubebAudioOut();

    void* Open(PortOut& port) override;
    void Close(void* impl) override;
    void Output(void* impl, void* ptr, size_t size) override;
    void SetVolume(void* impl, const std::array<int, 8>& ch_volumes) override;

private:
    static void LogCallback(const char* format, ...);

    cubeb* ctx = nullptr;
};

} // namespace Libraries::AudioOut
