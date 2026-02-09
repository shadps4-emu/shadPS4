// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <AL/al.h>
#include <AL/alc.h>

namespace Libraries::AudioOut {

class OpenALDevice {
public:
    static OpenALDevice& GetInstance() {
        static OpenALDevice instance;
        return instance;
    }

    bool IsInitialized() const {
        return initialized;
    }

    ALCdevice* GetDevice() const {
        return device;
    }

    ALCcontext* GetContext() const {
        return context;
    }

    bool MakeCurrent() {
        std::lock_guard<std::mutex> lock(mutex);

        if (!initialized) {
            return false;
        }

        return alcMakeContextCurrent(context);
    }

    void ReleaseContext() {
        std::lock_guard<std::mutex> lock(mutex);
        alcMakeContextCurrent(nullptr);
    }

private:
    OpenALDevice() {
        Initialize();
    }

    ~OpenALDevice() {
        Cleanup();
    }

    OpenALDevice(const OpenALDevice&) = delete;
    OpenALDevice& operator=(const OpenALDevice&) = delete;

    void Initialize() {
        std::lock_guard<std::mutex> lock(mutex);

        if (initialized) {
            return;
        }

        // Open default device
        device = alcOpenDevice(nullptr);
        if (!device) {
            return;
        }

        // Create context
        context = alcCreateContext(device, nullptr);
        if (!context) {
            alcCloseDevice(device);
            device = nullptr;
            return;
        }

        initialized = true;
    }

    void Cleanup() {
        std::lock_guard<std::mutex> lock(mutex);

        if (!initialized) {
            return;
        }

        ReleaseContext();

        if (context) {
            alcDestroyContext(context);
            context = nullptr;
        }

        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }

        initialized = false;
    }

    ALCdevice* device{nullptr};
    ALCcontext* context{nullptr};
    bool initialized{false};
    mutable std::mutex mutex;
};

} // namespace Libraries::AudioOut