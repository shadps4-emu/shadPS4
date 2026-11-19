// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>

#include "common/types.h"

namespace Libraries::AudioOut {

struct PortOut;

class PortBackend {
public:
    virtual ~PortBackend() = default;

    /// Guaranteed to be called in intervals of at least port buffer time,
    /// with size equal to port buffer size.
    virtual void Output(void* ptr) = 0;

    virtual void SetVolume(const std::array<int, 8>& ch_volumes) = 0;

    /// Number of channels actually delivered to the host device for a multi-channel port
    /// (2, 6 or 8), after any downmix done by the backend. Returns 0 when unknown.
    virtual u32 GetOutputChannels() const = 0;

    /// True when a PADSPK port has no dedicated (controller speaker) device and its stream
    /// plays on the same host device as the MAIN output. Always false for other port types.
    virtual bool SharesMainDevice() const = 0;
};

class AudioOutBackend {
public:
    AudioOutBackend() = default;
    virtual ~AudioOutBackend() = default;

    virtual std::unique_ptr<PortBackend> Open(PortOut& port) = 0;
};

class SDLAudioOut final : public AudioOutBackend {
public:
    std::unique_ptr<PortBackend> Open(PortOut& port) override;
};

class OpenALAudioOut final : public AudioOutBackend {
public:
    std::unique_ptr<PortBackend> Open(PortOut& port) override;
};

} // namespace Libraries::AudioOut
