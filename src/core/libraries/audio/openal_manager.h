// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <AL/al.h>
#include <AL/alc.h>

namespace Libraries::AudioOut {

class OpenALManager {
public:
    static OpenALManager& Instance() {
        static OpenALManager instance;
        return instance;
    }

    bool Initialize(int sample_rate) {
        std::lock_guard<std::mutex> lock(openal_mutex);
        if (initialized) {
            return true;
        }

        device_ = alcOpenDevice(nullptr);
        if (!device_) {
            return false;
        }

        ALCint attrs[] = {ALC_FREQUENCY, sample_rate, ALC_SYNC, ALC_FALSE, 0};
        openal_context = alcCreateContext(device_, attrs);
        if (!openal_context) {
            openal_context = alcCreateContext(device_, nullptr);
        }

        if (!openal_context || !alcMakeContextCurrent(openal_context)) {
            alcCloseDevice(device_);
            device_ = nullptr;
            openal_context = nullptr;
            return false;
        }

        initialized = true;
        return true;
    }

    void MakeContextCurrent() {
        std::lock_guard<std::mutex> lock(openal_mutex);
        if (initialized) {
            alcMakeContextCurrent(openal_context);
        }
    }

    ALCdevice* GetDevice() const {
        return device_;
    }
    ALCcontext* GetContext() const {
        return openal_context;
    }
    bool IsInitialized() const {
        return initialized;
    }

    // For extensions checking
    bool HasExtension(const char* extension) const {
        return alcIsExtensionPresent(device_, extension) == AL_TRUE;
    }

private:
    OpenALManager() = default;
    ~OpenALManager() {
        std::lock_guard<std::mutex> lock(openal_mutex);
        if (initialized) {
            alcMakeContextCurrent(nullptr);
            if (openal_context) {
                alcDestroyContext(openal_context);
            }
            if (device_) {
                alcCloseDevice(device_);
            }
        }
    }

    OpenALManager(const OpenALManager&) = delete;
    OpenALManager& operator=(const OpenALManager&) = delete;
    OpenALManager(OpenALManager&&) = delete;
    OpenALManager& operator=(OpenALManager&&) = delete;

    ALCdevice* device_ = nullptr;
    ALCcontext* openal_context = nullptr;
    bool initialized = false;
    mutable std::mutex openal_mutex;
};

} // namespace Libraries::AudioOut