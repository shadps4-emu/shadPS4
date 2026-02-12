// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <mutex>
#include <string>
#include <vector>
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
        std::lock_guard<std::mutex> lock(mutex);
        return initialized;
    }

    ALCdevice* GetDevice() const {
        std::lock_guard<std::mutex> lock(mutex);
        return device;
    }

    ALCcontext* GetContext() const {
        std::lock_guard<std::mutex> lock(mutex);
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

    // Select a specific audio device
    bool SelectDevice(const std::string& device_name) {
        std::lock_guard<std::mutex> lock(mutex);

        // If already initialized, clean up first
        if (initialized) {
            CleanupInternal();
        }

        return InitializeInternal(device_name);
    }

    // Get the name of the currently opened device
    std::string GetCurrentDeviceName() const {
        std::lock_guard<std::mutex> lock(mutex);
        return current_device_name;
    }

    // Check if device enumeration is supported
    static bool IsDeviceEnumerationSupported() {
        return alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") ||
               alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
    }

    // Get list of available devices (returns empty vector if enumeration not supported)
    static std::vector<std::string> GetAvailableDevices() {
        std::vector<std::string> devices_list;

        if (!IsDeviceEnumerationSupported()) {
            return devices_list;
        }

        // Get device list
        const ALCchar* devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        if (!devices) {
            return devices_list;
        }

        // Parse null-separated list
        const ALCchar* ptr = devices;
        while (ptr && *ptr) {
            devices_list.emplace_back(ptr);
            ptr += strlen(ptr) + 1;
        }

        return devices_list;
    }

private:
    OpenALDevice() {}

    ~OpenALDevice() {
        Cleanup();
    }

    OpenALDevice(const OpenALDevice&) = delete;
    OpenALDevice& operator=(const OpenALDevice&) = delete;

    bool InitializeInternal(const std::string& device_name) {
        // Handle disabled audio
        if (device_name == "None") {
            initialized = false;
            return false;
        }

        // Open the requested device
        if (device_name.empty() || device_name == "Default Device") {
            device = alcOpenDevice(nullptr); // Default device
        } else {
            // Try to open specific device
            device = alcOpenDevice(device_name.c_str());

            if (!device) {
                // Device not found, fall back to default
                device = alcOpenDevice(nullptr);
            }
        }

        if (!device) {
            return false;
        }

        // Create context
        context = alcCreateContext(device, nullptr);
        if (!context) {
            alcCloseDevice(device);
            device = nullptr;
            return false;
        }

        // Get actual device name
        const ALCchar* actual_name = alcGetString(device, ALC_DEVICE_SPECIFIER);
        current_device_name = actual_name ? actual_name : "Unknown";

        initialized = true;
        return true;
    }

    void Cleanup() {
        std::lock_guard<std::mutex> lock(mutex);
        CleanupInternal();
    }

    void CleanupInternal() {
        if (!initialized) {
            return;
        }

        alcMakeContextCurrent(nullptr);

        if (context) {
            alcDestroyContext(context);
            context = nullptr;
        }

        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }

        current_device_name.clear();
        initialized = false;
    }

    ALCdevice* device{nullptr};
    ALCcontext* context{nullptr};
    bool initialized{false};
    std::string current_device_name;
    mutable std::mutex mutex;
};

} // namespace Libraries::AudioOut