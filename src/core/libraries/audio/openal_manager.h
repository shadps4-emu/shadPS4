// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

#include "common/logging/log.h"
#include "core/emulator_settings.h"

#ifndef ALC_SOFT_HRTF
#define ALC_HRTF_SOFT 0x1992
#define ALC_DONT_CARE_SOFT 0x0002
#define ALC_HRTF_STATUS_SOFT 0x1993
#define ALC_HRTF_DISABLED_SOFT 0x0000
#define ALC_HRTF_ENABLED_SOFT 0x0001
#define ALC_HRTF_DENIED_SOFT 0x0002
#define ALC_HRTF_REQUIRED_SOFT 0x0003
#define ALC_HRTF_HEADPHONES_DETECTED_SOFT 0x0004
#define ALC_HRTF_UNSUPPORTED_FORMAT_SOFT 0x0005
#endif

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

        // Create context, requesting HRTF per the user setting when the device
        // supports ALC_SOFT_HRTF. In Auto mode OpenAL Soft decides on its own
        const ALCint* attr_ptr = nullptr;
        std::array<ALCint, 3> attrs{};
        const bool has_hrtf_ext = alcIsExtensionPresent(ctx.device, "ALC_SOFT_HRTF");
        if (has_hrtf_ext) {
            const u32 hrtf_mode = EmulatorSettings.GetOpenALHrtf();
            const ALCint hrtf_value = hrtf_mode == OpenALHrtfMode::HrtfOn    ? ALC_TRUE
                                      : hrtf_mode == OpenALHrtfMode::HrtfOff ? ALC_FALSE
                                                                             : ALC_DONT_CARE_SOFT;
            attrs = {ALC_HRTF_SOFT, hrtf_value, 0};
            attr_ptr = attrs.data();
        }

        ctx.context = alcCreateContext(ctx.device, attr_ptr);
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

        if (has_hrtf_ext) {
            ALCint status = ALC_HRTF_DISABLED_SOFT;
            alcGetIntegerv(ctx.device, ALC_HRTF_STATUS_SOFT, 1, &status);
            LOG_INFO(Lib_AudioOut, "OpenAL HRTF status for '{}': {}", ctx.device_name,
                     HrtfStatusString(status));
        } else {
            LOG_INFO(Lib_AudioOut, "OpenAL device '{}' does not support ALC_SOFT_HRTF",
                     ctx.device_name);
        }

        LOG_INFO(Lib_AudioOut, "OpenAL device initialized: '{}'", ctx.device_name);
        return true;
    }

    static const char* HrtfStatusString(const ALCint status) {
        switch (status) {
        case ALC_HRTF_DISABLED_SOFT:
            return "disabled";
        case ALC_HRTF_ENABLED_SOFT:
            return "enabled";
        case ALC_HRTF_DENIED_SOFT:
            return "denied by configuration";
        case ALC_HRTF_REQUIRED_SOFT:
            return "required by configuration";
        case ALC_HRTF_HEADPHONES_DETECTED_SOFT:
            return "enabled (headphones detected)";
        case ALC_HRTF_UNSUPPORTED_FORMAT_SOFT:
            return "unsupported output format";
        default:
            return "unknown";
        }
    }

    std::unordered_map<std::string, DeviceContext> devices;
    mutable std::mutex mutex;
    ALCcontext* current_context{nullptr}; // For thread-local tracking
};

} // namespace Libraries::AudioOut