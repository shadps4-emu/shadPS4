// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <memory>

namespace Libraries::AudioIn {

struct PortIn;

class PortInBackend {
public:
    virtual ~PortInBackend() = default;
    virtual int Read(void* out_buffer) = 0;
    virtual void Clear() = 0;
    virtual bool IsAvailable() = 0;
};

class AudioInBackend {
public:
    AudioInBackend() = default;
    virtual ~AudioInBackend() = default;
    virtual std::unique_ptr<PortInBackend> Open(PortIn& port) = 0;
};

class SDLAudioIn final : public AudioInBackend {
public:
    std::unique_ptr<PortInBackend> Open(PortIn& port) override;
};

} // namespace Libraries::AudioIn