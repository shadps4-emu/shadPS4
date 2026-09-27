// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/voice/voice.h"
#include "core/libraries/voice/voice_error.h"

namespace Libraries::Voice {

namespace {

struct Port {
    bool allocated = false;
    OrbisVoicePortParam param{};
    bool paused = false;
    bool muted = false;
    float volume = 1.0f;
    u32 bitrate = 48000;
    s32 connected_to = -1; // For an "in" port, the "out" port it is routed to, if any.
};

// The retail module keeps a single global instance, protected by an internal mutex, allocated
// the first time sceVoiceInit succeeds; every entry point rejects calls made before Init or
// after End. We mirror that with a static instance instead of matching its exact allocation
// strategy, since only the external contract (not the allocator) is observable from HLE.
struct VoiceManager {
    std::mutex mutex;
    bool initialized = false;
    bool started = false;
    std::array<Port, ORBIS_VOICE_MAX_PORT> ports{};
};

VoiceManager g_voice_manager;

} // namespace

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[in_port_id].allocated ||
        !g_voice_manager.ports[out_port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    g_voice_manager.ports[in_port_id].connected_to = static_cast<s32>(out_port_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceCreatePort(s32* port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id == nullptr || param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto it = std::find_if(g_voice_manager.ports.begin(), g_voice_manager.ports.end(),
                                 [](const Port& port) { return !port.allocated; });
    if (it == g_voice_manager.ports.end()) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *it = Port{
        .allocated = true,
        .param = *param,
    };
    *port_id = static_cast<s32>(std::distance(g_voice_manager.ports.begin(), it));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDeletePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.connected_to == static_cast<s32>(port_id)) {
            port.connected_to = -1;
        }
    }
    g_voice_manager.ports[port_id] = Port{};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& in_port = g_voice_manager.ports[in_port_id];
    if (in_port.connected_to == static_cast<s32>(out_port_id)) {
        in_port.connected_to = -1;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceEnd() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.initialized = false;
    g_voice_manager.started = false;
    g_voice_manager.ports = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (bitrate == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *bitrate = port.bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetMuteFlag(u32 port_id, bool* mute) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (mute == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *mute = port.muted;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortAttr(u32 port_id, s32 attr_id, void* value, u32 size) {
    LOG_TRACE(Lib_Voice, "port_id = {}, attr_id = {}, size = {}", port_id, attr_id, size);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (value == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    // The retail module validates `size` against a fixed per-attribute contract before touching
    // `value`; the table below (id -> required size) is confirmed via disassembly.
    u32 expected_size;
    switch (attr_id) {
    case ORBIS_VOICE_ATTR_1000:
    case ORBIS_VOICE_ATTR_1001:
    case ORBIS_VOICE_ATTR_2000:
    case ORBIS_VOICE_ATTR_2002:
        expected_size = 1;
        break;
    case ORBIS_VOICE_ATTR_1003:
        expected_size = 2;
        break;
    case ORBIS_VOICE_ATTR_1002:
    case ORBIS_VOICE_ATTR_2001:
        expected_size = 4;
        break;
    default:
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    if (size != expected_size) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    // The real meaning of each attribute id could not be identified (no public reference for
    // this table was found); report a stable, benign default so titles that poll this function
    // every frame (observed behavior) see consistent values instead of garbage.
    switch (expected_size) {
    case 1:
        *static_cast<u8*>(value) = 0;
        break;
    case 2:
        *static_cast<u16*>(value) = 0;
        break;
    case 4:
        *static_cast<u32*>(value) = 0;
        break;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (info == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    info->port_type = port.param.type;
    info->state = port.paused ? 1 : 0;
    info->edge = nullptr;
    info->byte_count = 0;
    info->frame_size = 1;
    info->edge_count = 0;
    info->reserved = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetResourceInfo(void* info) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (info == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetVolume(u32 port_id, float* volume) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (volume == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    const auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    *volume = port.volume;
    return ORBIS_OK;
}

static s32 InitInternal(const OrbisVoiceInitParam* param) {
    if (g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_ALREADY_INIT;
    }
    if (param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.ports = {};
    g_voice_manager.started = false;
    g_voice_manager.initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceInit(const OrbisVoiceInitParam* param, s32 container) {
    LOG_INFO(Lib_Voice, "called, container = {}", container);
    return InitInternal(param);
}

s32 PS4_SYSV_ABI sceVoiceInitHQ(const OrbisVoiceInitParam* param, s32 container) {
    LOG_INFO(Lib_Voice, "called, container = {}", container);
    return InitInternal(param);
}

s32 PS4_SYSV_ABI sceVoicePausePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.paused = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoicePausePortAll() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.paused = true;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceReadFromOPort(u32 port_id, void* data, u32* size) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (data == nullptr || size == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    // No audio backend is wired up yet: report no data available rather than fabricating audio.
    *size = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResetPort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    const auto param = port.param;
    port = Port{.allocated = true, .param = param};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.paused = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePortAll() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.paused = false;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetBitRate(u32 port_id, u32 bitrate) {
    LOG_INFO(Lib_Voice, "port_id = {}, bitrate = {}", port_id, bitrate);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.bitrate = bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlag(u32 port_id, bool mute) {
    LOG_INFO(Lib_Voice, "port_id = {}, mute = {}", port_id, mute);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.muted = mute;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll(bool mute) {
    LOG_INFO(Lib_Voice, "mute = {}", mute);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    for (auto& port : g_voice_manager.ports) {
        if (port.allocated) {
            port.muted = mute;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetThreadsParams(void* params) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (params == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetVolume(u32 port_id, float volume) {
    LOG_INFO(Lib_Voice, "port_id = {}, volume = {}", port_id, volume);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.volume = volume;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStart(const OrbisVoiceStartParam* param) {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    // Confirmed via disassembly: the retail module additionally requires the first 8 bytes of
    // this struct to be non-zero.
    if (param == nullptr || param->container == 0) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.started = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStop() {
    LOG_INFO(Lib_Voice, "called");
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    g_voice_manager.started = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceUpdatePort(u32 port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (param == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    auto& port = g_voice_manager.ports[port_id];
    if (!port.allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    port.param = *param;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADAdjustment(u32 port_id, s32 value) {
    LOG_INFO(Lib_Voice, "port_id = {}, value = {}", port_id, value);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADSetVersion(u32 version) {
    LOG_INFO(Lib_Voice, "version = {}", version);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceWriteToIPort(u32 port_id, const void* data, u32* size) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    if (!g_voice_manager.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_INVALID_PORT_ID;
    }
    if (data == nullptr || size == nullptr) {
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    std::scoped_lock lock{g_voice_manager.mutex};
    if (!g_voice_manager.ports[port_id].allocated) {
        return ORBIS_VOICE_ERROR_NOT_ACTIVE;
    }
    // No audio backend is wired up yet: report every supplied byte as consumed so callers do
    // not spin retrying, without fabricating an encode/network path.
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oV9GAdJ23Gw", "libSceVoice", 1, "libSceVoice", sceVoiceConnectIPortToOPort);
    LIB_FUNCTION("nXpje5yNpaE", "libSceVoice", 1, "libSceVoice", sceVoiceCreatePort);
    LIB_FUNCTION("b7kJI+nx2hg", "libSceVoice", 1, "libSceVoice", sceVoiceDeletePort);
    LIB_FUNCTION("ajVj3QG2um4", "libSceVoice", 1, "libSceVoice", sceVoiceDisconnectIPortFromOPort);
    LIB_FUNCTION("Oo0S5PH7FIQ", "libSceVoice", 1, "libSceVoice", sceVoiceEnd);
    LIB_FUNCTION("cJLufzou6bc", "libSceVoice", 1, "libSceVoice", sceVoiceGetBitRate);
    LIB_FUNCTION("Pc4z1QjForU", "libSceVoice", 1, "libSceVoice", sceVoiceGetMuteFlag);
    LIB_FUNCTION("elcxZTEfHZM", "libSceVoice", 1, "libSceVoice", sceVoiceGetPortAttr);
    LIB_FUNCTION("CrLqDwWLoXM", "libSceVoice", 1, "libSceVoice", sceVoiceGetPortInfo);
    LIB_FUNCTION("Z6QV6j7igvE", "libSceVoice", 1, "libSceVoice", sceVoiceGetResourceInfo);
    LIB_FUNCTION("jjkCjneOYSs", "libSceVoice", 1, "libSceVoice", sceVoiceGetVolume);
    LIB_FUNCTION("9TrhuGzberQ", "libSceVoice", 1, "libSceVoice", sceVoiceInit);
    LIB_FUNCTION("IPHvnM5+g04", "libSceVoice", 1, "libSceVoice", sceVoiceInitHQ);
    LIB_FUNCTION("x0slGBQW+wY", "libSceVoice", 1, "libSceVoice", sceVoicePausePort);
    LIB_FUNCTION("Dinob0yMRl8", "libSceVoice", 1, "libSceVoice", sceVoicePausePortAll);
    LIB_FUNCTION("cQ6DGsQEjV4", "libSceVoice", 1, "libSceVoice", sceVoiceReadFromOPort);
    LIB_FUNCTION("udAxvCePkUs", "libSceVoice", 1, "libSceVoice", sceVoiceResetPort);
    LIB_FUNCTION("gAgN+HkiEzY", "libSceVoice", 1, "libSceVoice", sceVoiceResumePort);
    LIB_FUNCTION("jbkJFmOZ9U0", "libSceVoice", 1, "libSceVoice", sceVoiceResumePortAll);
    LIB_FUNCTION("TexwmOHQsDg", "libSceVoice", 1, "libSceVoice", sceVoiceSetBitRate);
    LIB_FUNCTION("gwUynkEgNFY", "libSceVoice", 1, "libSceVoice", sceVoiceSetMuteFlag);
    LIB_FUNCTION("oUha0S-Ij9Q", "libSceVoice", 1, "libSceVoice", sceVoiceSetMuteFlagAll);
    LIB_FUNCTION("clyKUyi3RYU", "libSceVoice", 1, "libSceVoice", sceVoiceSetThreadsParams);
    LIB_FUNCTION("QBFoAIjJoXQ", "libSceVoice", 1, "libSceVoice", sceVoiceSetVolume);
    LIB_FUNCTION("54phPH2LZls", "libSceVoice", 1, "libSceVoice", sceVoiceStart);
    LIB_FUNCTION("Ao2YNSA7-Qo", "libSceVoice", 1, "libSceVoice", sceVoiceStop);
    LIB_FUNCTION("jSZNP7xJrcw", "libSceVoice", 1, "libSceVoice", sceVoiceUpdatePort);
    LIB_FUNCTION("hg9T73LlRiU", "libSceVoice", 1, "libSceVoice", sceVoiceVADAdjustment);
    LIB_FUNCTION("wFeAxEeEi-8", "libSceVoice", 1, "libSceVoice", sceVoiceVADSetVersion);
    LIB_FUNCTION("YeJl6yDlhW0", "libSceVoice", 1, "libSceVoice", sceVoiceWriteToIPort);
};

} // namespace Libraries::Voice
