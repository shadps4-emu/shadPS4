// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

namespace Libraries::AudioOut {

struct DeviceContext {
    ALCdevice* device{nullptr};
    ALCcontext* context{nullptr};
    std::string device_name;
    int port_count{0};

    bool IsValid() const {
        return device != nullptr && context != nullptr;
    }

    void Cleanup() {
        if (context) {
            alcDestroyContext(context);
            context = nullptr;
        }
        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }
        port_count = 0;
    }
};

class OpenALDevice {
public:
    static OpenALDevice& GetInstance() {
        static OpenALDevice instance;
        return instance;
    }

    // Register a port that uses this device
    bool RegisterPort(const std::string& device_name) {
        std::lock_guard<std::mutex> lock(mutex);

        // Handle "Default Device" alias
        std::string actual_device_name = device_name;
        if (actual_device_name.empty() || actual_device_name == "Default Device") {
            actual_device_name = GetDefaultDeviceName();
        }

        // Find or create device context for this device name
        auto it = devices.find(actual_device_name);
        if (it != devices.end()) {
            // Device exists, increment count
            it->second.port_count++;
            LOG_INFO(Lib_AudioOut, "Reusing OpenAL device '{}', port count: {}", actual_device_name,
                     it->second.port_count);
            return true;
        }

        // Create new device
        DeviceContext ctx;
        if (!InitializeDevice(ctx, actual_device_name)) {
            LOG_ERROR(Lib_AudioOut, "Failed to initialize OpenAL device '{}'", actual_device_name);
            return false;
        }

        ctx.port_count = 1;
        devices[actual_device_name] = ctx;

        LOG_INFO(Lib_AudioOut, "Created new OpenAL device '{}'", actual_device_name);
        return true;
    }

    // Unregister a port
    void UnregisterPort(const std::string& device_name) {
        std::lock_guard<std::mutex> lock(mutex);

        std::string actual_device_name = device_name;
        if (actual_device_name.empty() || actual_device_name == "Default Device") {
            actual_device_name = GetDefaultDeviceName();
        }

        auto it = devices.find(actual_device_name);
        if (it != devices.end()) {
            it->second.port_count--;
            LOG_INFO(Lib_AudioOut, "Port unregistered from '{}', remaining ports: {}",
                     actual_device_name, it->second.port_count);

            if (it->second.port_count <= 0) {
                LOG_INFO(Lib_AudioOut, "Cleaning up OpenAL device '{}'", actual_device_name);
                it->second.Cleanup();
                devices.erase(it);
            }
        }
    }

    bool MakeCurrent(const std::string& device_name) {
        std::lock_guard<std::mutex> lock(mutex);

        std::string actual_device_name = device_name;
        if (actual_device_name.empty() || actual_device_name == "Default Device") {
            actual_device_name = GetDefaultDeviceName();
        }

        auto it = devices.find(actual_device_name);
        if (it == devices.end() || !it->second.IsValid()) {
            return false;
        }

        // Store current device for this thread (simplified - in practice you might want
        // thread-local storage)
        current_context = it->second.context;
        return alcMakeContextCurrent(it->second.context);
    }

    void ReleaseContext() {
        std::lock_guard<std::mutex> lock(mutex);
        alcMakeContextCurrent(nullptr);
        current_context = nullptr;
    }

    // Get the default device name
    static std::string GetDefaultDeviceName() {
        const ALCchar* default_device = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
        return default_device ? default_device : "Default Device";
    }

    // Check if device enumeration is supported
    static bool IsDeviceEnumerationSupported() {
        return alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") ||
               alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
    }

    // Get list of available devices
    static std::vector<std::string> GetAvailableDevices() {
        std::vector<std::string> devices_list;

        if (!alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
            return devices_list;

        const ALCchar* devices = nullptr;
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")) {
            devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        } else {
            devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        }

        if (!devices)
            return devices_list;

        const ALCchar* ptr = devices;
        while (*ptr != '\0') {
            devices_list.emplace_back(ptr);
            ptr += std::strlen(ptr) + 1;
        }

        return devices_list;
    }

private:
    OpenALDevice() = default;
    ~OpenALDevice() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [name, ctx] : devices) {
            ctx.Cleanup();
        }
        devices.clear();
    }

    OpenALDevice(const OpenALDevice&) = delete;
    OpenALDevice& operator=(const OpenALDevice&) = delete;

    bool InitializeDevice(DeviceContext& ctx, const std::string& device_name) {
        // Handle disabled audio
        if (device_name == "None") {
            return false;
        }

        // Open the requested device
        if (device_name.empty() || device_name == "Default Device") {
            ctx.device = alcOpenDevice(nullptr);
        } else {
            ctx.device = alcOpenDevice(device_name.c_str());
            if (!ctx.device) {
                LOG_WARNING(Lib_AudioOut, "Device '{}' not found, falling back to default",
                            device_name);
                ctx.device = alcOpenDevice(nullptr);
            }
        }

        if (!ctx.device) {
            LOG_ERROR(Lib_AudioOut, "Failed to open OpenAL device");
            return false;
        }

        // Create context
        ctx.context = alcCreateContext(ctx.device, nullptr);
        if (!ctx.context) {
            LOG_ERROR(Lib_AudioOut, "Failed to create OpenAL context");
            alcCloseDevice(ctx.device);
            ctx.device = nullptr;
            return false;
        }

        // Get actual device name
        const ALCchar* actual_name = nullptr;
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")) {
            actual_name = alcGetString(ctx.device, ALC_ALL_DEVICES_SPECIFIER);
        } else {
            actual_name = alcGetString(ctx.device, ALC_DEVICE_SPECIFIER);
        }
        ctx.device_name = actual_name ? actual_name : "Unknown";

        LOG_INFO(Lib_AudioOut, "OpenAL device initialized: '{}'", ctx.device_name);
        return true;
    }

    std::unordered_map<std::string, DeviceContext> devices;
    mutable std::mutex mutex;
    ALCcontext* current_context{nullptr}; // For thread-local tracking
};

} // namespace Libraries::AudioOut