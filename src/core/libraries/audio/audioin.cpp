// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <shared_mutex>
#include "audioin_backend.h"
#include "audioin_error.h"
#include "common/logging/log.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::AudioIn {

std::array<std::shared_ptr<PortIn>, ORBIS_AUDIO_IN_NUM_PORTS> port_table{};
std::shared_mutex port_table_mutex;
std::mutex port_allocation_mutex;

static std::unique_ptr<AudioInBackend> audio;

/*
 * Helper functions
 **/
static int GetPortId(s32 handle) {
    int port_id = handle & 0xFF;

    if (port_id >= ORBIS_AUDIO_IN_NUM_PORTS) {
        LOG_ERROR(Lib_AudioIn, "Invalid port");
        return ORBIS_AUDIO_IN_ERROR_PORT_FULL;
    }

    if ((handle & 0x7f000000) != 0x30000000) {
        LOG_ERROR(Lib_AudioIn, "Invalid handle format");
        return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
    }

    return port_id;
}

static s32 GetPortType(s32 handle) {
    return (handle >> 16) & 0xFF;
}

static int AllocatePort(OrbisAudioInType type) {
    // TODO implement port type ranges if needed
    for (int i = 0; i <= ORBIS_AUDIO_IN_NUM_PORTS; i++) {
        std::shared_lock read_lock{port_table_mutex};
        if (!port_table[i]) {
            return i;
        }
    }
    return -1;
}
/*
 * sceAudioIn implementation
 **/
static bool initOnce = false;
int PS4_SYSV_ABI sceAudioInOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                u32 index, u32 len, u32 freq, u32 param) {
    LOG_INFO(Lib_AudioIn, "called, userId={}, type={}, index={}, len={}, freq={}, param={}", userId,
             type, index, len, freq, param);
    if (!initOnce) {
        // sceAudioInInit doesn't seem to be called by most apps before sceAudioInOpen so we init
        // here
        audio = std::make_unique<SDLAudioIn>();
        initOnce = true;
    }

    if (len == 0 || len > 2048) {
        LOG_ERROR(Lib_AudioIn, "Invalid size");
        return ORBIS_AUDIO_IN_ERROR_INVALID_SIZE;
    }

    // Validate parameters
    OrbisAudioInType in_type = static_cast<OrbisAudioInType>(type);
    OrbisAudioInParamFormat format = static_cast<OrbisAudioInParamFormat>(param);

    if (format != OrbisAudioInParamFormat::S16Mono &&
        format != OrbisAudioInParamFormat::S16Stereo) {
        LOG_ERROR(Lib_AudioIn, "Invalid format");
        return ORBIS_AUDIO_IN_ERROR_INVALID_PARAM;
    }

    if (freq != 16000 && freq != 48000) {
        LOG_ERROR(Lib_AudioIn, "Invalid sample rate");
        return ORBIS_AUDIO_IN_ERROR_INVALID_FREQ;
    }

    std::unique_lock lock{port_allocation_mutex};

    // Allocate port
    int port_id = AllocatePort(in_type);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioIn, "No free audio input ports available");
        return ORBIS_AUDIO_IN_ERROR_PORT_FULL;
    }

    // Create port object
    std::shared_ptr<PortIn> port;
    try {
        port = std::make_shared<PortIn>();

        port->type = in_type;
        port->format = format;
        port->samples_num = len;
        port->freq = freq;

        // Determine channel count and sample size based on format
        switch (format) {
        case OrbisAudioInParamFormat::S16Mono:
            port->channels_num = 1;
            port->sample_size = 2;
            break;
        case OrbisAudioInParamFormat::S16Stereo:
            port->channels_num = 2;
            port->sample_size = 2;
            break;
        default:
            LOG_ERROR(Lib_AudioIn, "Unsupported audio format: {}", static_cast<u32>(format));
            return ORBIS_AUDIO_IN_ERROR_INVALID_PARAM;
        }

        // Open backend
        port->impl = audio->Open(*port);
        if (!port->impl) {
            throw std::runtime_error("Failed to create audio backend");
        }

    } catch (const std::bad_alloc&) {
        LOG_ERROR(Lib_AudioIn, "Failed to allocate memory for audio port");
        return ORBIS_AUDIO_IN_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_AudioIn, "Failed to open audio input port: {}", e.what());
        return ORBIS_AUDIO_IN_ERROR_NOT_OPENED;
    }

    // Store the port pointer with write lock
    {
        std::unique_lock write_lock{port_table_mutex};
        port_table[port_id] = port;
    }

    // Create handle
    s32 handle = (type << 16) | port_id | 0x30000000;

    LOG_INFO(Lib_AudioIn, "Opened audio input port {}: type={}, samples={}, freq={}, format={}",
             handle, static_cast<u32>(in_type), len, freq, static_cast<u32>(format));
    return handle;
}

int PS4_SYSV_ABI sceAudioInHqOpen(Libraries::UserService::OrbisUserServiceUserId userId, u32 type,
                                  u32 index, u32 len, u32 freq, u32 param) {
    LOG_INFO(Lib_AudioIn, "called, userId={}, type={}, index={}, len={}, freq={}, param={}", userId,
             type, index, len, freq, param);
    int result = sceAudioInOpen(userId, type, index, len, freq, param);
    if (result < 0) {
        LOG_ERROR(Lib_AudioIn, "Error returned  {:#x}", result);
    }
    return result;
}

int PS4_SYSV_ABI sceAudioInClose(s32 handle) {
    LOG_INFO(Lib_AudioIn, "called, handle={:#x}", handle);

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioIn, "Invalid port id");
        return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
    }

    std::unique_lock lock{port_allocation_mutex};
    std::shared_ptr<PortIn> port;

    // Get and clear the port pointer with write lock
    {
        std::unique_lock write_lock{port_table_mutex};
        port = std::move(port_table[port_id]);
        if (!port) {
            LOG_ERROR(Lib_AudioIn, "Port wasn't open {}", port_id);
            return ORBIS_AUDIO_IN_ERROR_NOT_OPENED;
        }
        port_table[port_id].reset();
    }

    // Free resources
    std::scoped_lock port_lock{port->mutex};
    port->impl.reset();

    LOG_INFO(Lib_AudioIn, "Closed audio input port {}", handle);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInInput(s32 handle, void* dest) {
    LOG_TRACE(Lib_AudioIn, "called, handle={:#x}, dest={}", handle, fmt::ptr(dest));

    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioIn, "Invalid port id");
        return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
    }

    if (!dest) {
        LOG_ERROR(Lib_AudioIn, "Invalid output buffer pointer");
        return ORBIS_AUDIO_IN_ERROR_INVALID_POINTER;
    }

    // Get port with read lock
    std::shared_ptr<PortIn> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        if (port_id < 0 || port_id >= static_cast<int>(port_table.size())) {
            LOG_ERROR(Lib_AudioIn, "Invalid port id: {}", port_id);
            return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
        }
        port = port_table[port_id];
    }

    if (!port || !port->impl) {
        LOG_ERROR(Lib_AudioIn, "Audio input port {} is not open", handle);
        return ORBIS_AUDIO_IN_ERROR_NOT_OPENED;
    }

    std::scoped_lock lock{port->mutex};
    return port->impl->Read(dest);
}

int PS4_SYSV_ABI sceAudioInGetSilentState(s32 handle) {
    LOG_TRACE(Lib_AudioIn, "called, handle={:#x}", handle);
    int port_id = GetPortId(handle);
    if (port_id < 0) {
        LOG_ERROR(Lib_AudioIn, "Invalid port id");
        return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
    }
    // Get port with read lock
    std::shared_ptr<PortIn> port;
    {
        std::shared_lock read_lock{port_table_mutex};
        if (port_id < 0 || port_id >= static_cast<int>(port_table.size())) {
            LOG_ERROR(Lib_AudioIn, "Invalid port id: {}", port_id);
            return ORBIS_AUDIO_IN_ERROR_INVALID_HANDLE;
        }
        port = port_table[port_id];
    }

    if (!port || !port->impl) {
        LOG_ERROR(Lib_AudioIn, "Audio input port {} is not open", handle);
        return ORBIS_AUDIO_IN_ERROR_NOT_OPENED;
    }

    u32 silent_state = 0;
    std::scoped_lock lock{port->mutex};
    if (!port->impl->IsAvailable()) { // if no mic exist or is not available
        silent_state |= ORBIS_AUDIO_IN_SILENT_STATE_DEVICE_NONE;
    }
    return silent_state;
}

/*
 * Stubbed functions
 **/
int PS4_SYSV_ABI sceAudioInChangeAppModuleState() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInCountPorts() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceHqOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceIdHqOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceIdOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInDeviceOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtClose() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtCtrl() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtInput() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtOpen() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInExtSetAecMode() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetHandleStatusInfo() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInGetRerouteCount() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInHqOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInInit() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInInputs() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInIsSharedDevice() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInOpenEx() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetAllMute() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetCompressorPreGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetConnections() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetConnectionsForUser() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetDevConnection() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetFocusForUser() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetMode() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetMode2() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetPortConnections() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetPortStatuses() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetSparkParam() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetSparkSideTone() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetUsbGain() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInSetUserMute() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicCreate() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicDestroy() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudioInVmicWrite() {
    LOG_ERROR(Lib_AudioIn, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("IQtWgnrw6v8", "libSceAudioIn", 1, "libSceAudioIn",
                 sceAudioInChangeAppModuleState);
    LIB_FUNCTION("Jh6WbHhnI68", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInClose);
    LIB_FUNCTION("8mtcsG-Qp5E", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInCountPorts);
    LIB_FUNCTION("5qRVfxOmbno", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInDeviceHqOpen);
    LIB_FUNCTION("gUNabrUkZNg", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInDeviceIdHqOpen);
    LIB_FUNCTION("X-AQLtdxQOo", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInDeviceIdOpen);
    LIB_FUNCTION("VoX9InuwwTg", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInDeviceOpen);
    LIB_FUNCTION("48-miagyJ2I", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInDeviceOpenEx);
    LIB_FUNCTION("kFKJ3MVcDuo", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInExtClose);
    LIB_FUNCTION("mhAfefP9m2g", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInExtCtrl);
    LIB_FUNCTION("KpBKoHKVKEc", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInExtInput);
    LIB_FUNCTION("YZ+3seW7CyY", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInExtOpen);
    LIB_FUNCTION("FVGWf8JaHOE", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInExtSetAecMode);
    LIB_FUNCTION("S-rDUfQk9sg", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInGetGain);
    LIB_FUNCTION("NJam1-F7lNY", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInGetHandleStatusInfo);
    LIB_FUNCTION("3shKmTrTw6c", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInGetRerouteCount);
    LIB_FUNCTION("BohEAQ7DlUE", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInGetSilentState);
    LIB_FUNCTION("nya-R5gDYhM", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInHqOpen);
    LIB_FUNCTION("CTh72m+IYbU", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInHqOpenEx);
    LIB_FUNCTION("SxQprgjttKE", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInInit);
    LIB_FUNCTION("LozEOU8+anM", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInInput);
    LIB_FUNCTION("rmgXsZ-2Tyk", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInInputs);
    LIB_FUNCTION("6QP1MzdFWhs", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInIsSharedDevice);
    LIB_FUNCTION("5NE8Sjc7VC8", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInOpen);
    LIB_FUNCTION("+DY07NwJb0s", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInOpenEx);
    LIB_FUNCTION("vYFsze1SqU8", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetAllMute);
    LIB_FUNCTION("vyh-T6sMqnw", "libSceAudioIn", 1, "libSceAudioIn",
                 sceAudioInSetCompressorPreGain);
    LIB_FUNCTION("YeBSNVAELe4", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetConnections);
    LIB_FUNCTION("thLNHvkWSeg", "libSceAudioIn", 1, "libSceAudioIn",
                 sceAudioInSetConnectionsForUser);
    LIB_FUNCTION("rcgv2ciDrtc", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetDevConnection);
    LIB_FUNCTION("iN3KqF-8R-w", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetFocusForUser);
    LIB_FUNCTION("VAzfxqDwbQ0", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetMode);
    LIB_FUNCTION("CwBFvAlOv7k", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetMode2);
    LIB_FUNCTION("tQpOPpYwv7o", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetPortConnections);
    LIB_FUNCTION("NUWqWguYcNQ", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetPortStatuses);
    LIB_FUNCTION("U0ivfdKFZbA", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetSparkParam);
    LIB_FUNCTION("hWMCAPpqzDo", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetSparkSideTone);
    LIB_FUNCTION("nqXpw3MaN50", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetUsbGain);
    LIB_FUNCTION("arJp991xk5k", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInSetUserMute);
    LIB_FUNCTION("DVTn+iMSpBM", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInVmicCreate);
    LIB_FUNCTION("3ULZGIl+Acc", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInVmicDestroy);
    LIB_FUNCTION("4kHw99LUG3A", "libSceAudioIn", 1, "libSceAudioIn", sceAudioInVmicWrite);
};

} // namespace Libraries::AudioIn
