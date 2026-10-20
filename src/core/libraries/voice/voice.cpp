// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/voice/voice.h"
#include "core/libraries/voice/voice_error.h"

namespace Libraries::Voice {

namespace {

struct Port {
    bool allocated = false;
    s32 type = 0;
    bool muted = false;
    float volume = 1.0f;
    u32 bitrate = 48000;
};

struct VoiceState {
    std::mutex mutex;
    bool initialized = false;
    std::array<Port, ORBIS_VOICE_MAX_PORT> ports{};
};

VoiceState g_state;

s32 CheckPort(u32 port_id) {
    if (!g_state.initialized) {
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id >= ORBIS_VOICE_MAX_PORT) {
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    return ORBIS_OK;
}

Port* FindPort(u32 port_id) {
    Port& port = g_state.ports[port_id];
    return port.allocated ? &port : nullptr;
}

s32 InitInternal(const OrbisVoiceInitParam* param) {
    std::scoped_lock lock{g_state.mutex};
    if (g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is already initialized");
        return ORBIS_VOICE_ERROR_ALREADY_INIT;
    }
    if (param == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: param is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    g_state.ports = {};
    g_state.initialized = true;
    return ORBIS_OK;
}

} // namespace

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        LOG_ERROR(Lib_Voice, "Invalid port: in_port_id = {}, out_port_id = {}", in_port_id,
                  out_port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    if (!FindPort(in_port_id) || !FindPort(out_port_id)) {
        LOG_ERROR(Lib_Voice, "Port not allocated: in_port_id = {}, out_port_id = {}", in_port_id,
                  out_port_id);
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceCreatePort(s32* port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (port_id == nullptr || param == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: port_id is null or param is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    if (param->type < ORBIS_VOICE_PORTTYPE_IN_DEVICE ||
        param->type > ORBIS_VOICE_PORTTYPE_OUT_DEVICE) {
        LOG_ERROR(Lib_Voice, "Invalid port type: {}", param->type);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const auto it = std::ranges::find_if(g_state.ports, [](const Port& p) { return !p.allocated; });
    if (it == g_state.ports.end()) {
        LOG_ERROR(Lib_Voice, "No free port available");
        return ORBIS_VOICE_ERROR_RESOURCE_INSUFFICIENT;
    }
    *it = Port{.allocated = true, .type = param->type};
    *port_id = static_cast<s32>(std::distance(g_state.ports.begin(), it));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDeletePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (!FindPort(port_id)) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    g_state.ports[port_id] = Port{};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort(u32 in_port_id, u32 out_port_id) {
    LOG_INFO(Lib_Voice, "in_port_id = {}, out_port_id = {}", in_port_id, out_port_id);
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (in_port_id >= ORBIS_VOICE_MAX_PORT || out_port_id >= ORBIS_VOICE_MAX_PORT) {
        LOG_ERROR(Lib_Voice, "Invalid port: in_port_id = {}, out_port_id = {}", in_port_id,
                  out_port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    if (!FindPort(in_port_id) || !FindPort(out_port_id)) {
        LOG_ERROR(Lib_Voice, "Port not allocated: in_port_id = {}, out_port_id = {}", in_port_id,
                  out_port_id);
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceEnd() {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    g_state.initialized = false;
    g_state.ports = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (bitrate == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: bitrate is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    *bitrate = port->bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetMuteFlag(u32 port_id, bool* mute) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (mute == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: mute is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    *mute = port->muted;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortAttr(u32 port_id, s32 attr_id, void* value, u32 size) {
    LOG_TRACE(Lib_Voice, "port_id = {}, attr_id = {}, size = {}", port_id, attr_id, size);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (value == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: value is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    u32 expected_size;
    s32 required_type;
    switch (attr_id) {
    case ORBIS_VOICE_ATTR_1000:
    case ORBIS_VOICE_ATTR_1001:
    case ORBIS_VOICE_ATTR_2000:
    case ORBIS_VOICE_ATTR_2002:
        expected_size = 1;
        required_type = ORBIS_VOICE_PORTTYPE_IN_DEVICE;
        break;
    case ORBIS_VOICE_ATTR_1002:
    case ORBIS_VOICE_ATTR_2001:
        expected_size = 4;
        required_type = ORBIS_VOICE_PORTTYPE_IN_DEVICE;
        break;
    case ORBIS_VOICE_ATTR_1003:
        expected_size = 2;
        required_type = ORBIS_VOICE_PORTTYPE_OUT_DEVICE;
        break;
    default:
        LOG_ERROR(Lib_Voice, "Invalid attr_id: {}", attr_id);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    if (size != expected_size) {
        LOG_ERROR(Lib_Voice, "Invalid size {} for attr_id {} (expected {})", size, attr_id,
                  expected_size);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    if (port->type != required_type) {
        LOG_ERROR(Lib_Voice, "Port {} has the wrong type for attr_id {}", port_id, attr_id);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const u32 result =
        attr_id == ORBIS_VOICE_ATTR_1002 ? AudioIn::ORBIS_AUDIO_IN_SILENT_STATE_DEVICE_NONE : 0;
    switch (expected_size) {
    case 1:
        *static_cast<u8*>(value) = static_cast<u8>(result);
        break;
    case 2:
        *static_cast<u16*>(value) = static_cast<u16>(result);
        break;
    case 4:
        *static_cast<u32*>(value) = result;
        break;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (info == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: info is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
    info->port_type = port->type;
    info->state = 0;
    info->byte_count = 0;
    info->frame_size = 1;
    info->edge_count = 0;
    info->reserved = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetResourceInfo(OrbisVoiceResourceInfo* info) {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (info == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: info is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    *info = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetVolume(u32 port_id, float* volume) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (volume == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: volume is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    *volume = port->volume;
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
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (!FindPort(port_id)) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoicePausePortAll() {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceReadFromOPort(u32 port_id, void* data, u32* size) {
    LOG_TRACE(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (data == nullptr || size == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: data is null or size is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    *size = 0;
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
    switch (port->type) {
    case ORBIS_VOICE_PORTTYPE_OUT_PCMAUDIO:
    case ORBIS_VOICE_PORTTYPE_OUT_VOICE:
        return ORBIS_OK;
    case ORBIS_VOICE_PORTTYPE_OUT_DEVICE:
        LOG_ERROR(Lib_Voice, "Cannot read from a device output port: port_id = {}", port_id);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    default:
        LOG_ERROR(Lib_Voice, "Port {} is not an output port (type = {})", port_id, port->type);
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
}

s32 PS4_SYSV_ABI sceVoiceResetPort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    *port = Port{.allocated = true, .type = port->type};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePort(u32 port_id) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (!FindPort(port_id)) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePortAll() {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetBitRate(u32 port_id, u32 bitrate) {
    LOG_INFO(Lib_Voice, "port_id = {}, bitrate = {}", port_id, bitrate);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    port->bitrate = bitrate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlag(u32 port_id, bool mute) {
    LOG_INFO(Lib_Voice, "port_id = {}, mute = {}", port_id, mute);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    port->muted = mute;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll(bool mute) {
    LOG_INFO(Lib_Voice, "mute = {}", mute);
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    for (Port& port : g_state.ports) {
        port.muted = mute;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetThreadsParams() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetVolume(u32 port_id, float volume) {
    LOG_INFO(Lib_Voice, "port_id = {}, volume = {}", port_id, volume);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    port->volume = std::isfinite(volume) ? volume : 0.0f;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStart(const OrbisVoiceStartParam* param) {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    if (param == nullptr || param->memory == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: param is null or param->memory is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStop() {
    LOG_INFO(Lib_Voice, "called");
    std::scoped_lock lock{g_state.mutex};
    if (!g_state.initialized) {
        LOG_ERROR(Lib_Voice, "Voice is not initialized");
        return ORBIS_VOICE_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceUpdatePort(u32 port_id, const OrbisVoicePortParam* param) {
    LOG_INFO(Lib_Voice, "port_id = {}", port_id);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (param == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: param is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    if (!FindPort(port_id)) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        return ORBIS_VOICE_ERROR_PORT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADAdjustment(float param1, float param2) {
    LOG_INFO(Lib_Voice, "param1 = {}, param2 = {}", param1, param2);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADSetVersion(s32 version) {
    LOG_INFO(Lib_Voice, "version = {}", version);
    if (version != 0 && version != 1) {
        LOG_ERROR(Lib_Voice, "Invalid version: {}", version);
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceWriteToIPort(u32 port_id, const void* data, u32* size, s16 unk) {
    LOG_TRACE(Lib_Voice, "port_id = {}, unk = {}", port_id, unk);
    std::scoped_lock lock{g_state.mutex};
    if (const s32 ret = CheckPort(port_id); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Voice, "Port check failed: port_id = {}, error = {:#x}", port_id,
                  static_cast<u32>(ret));
        return ret;
    }
    if (data == nullptr || size == nullptr) {
        LOG_ERROR(Lib_Voice, "Invalid argument: data is null or size is null");
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
    const Port* port = FindPort(port_id);
    if (!port) {
        LOG_ERROR(Lib_Voice, "Port {} is not allocated", port_id);
        *size = 0;
        return ORBIS_VOICE_ERROR_TOPOLOGY;
    }
    if (port->type != ORBIS_VOICE_PORTTYPE_IN_PCMAUDIO &&
        port->type != ORBIS_VOICE_PORTTYPE_IN_VOICE) {
        LOG_ERROR(Lib_Voice, "Port {} is not an input port (type = {})", port_id, port->type);
        *size = 0;
        return ORBIS_VOICE_ERROR_ARGUMENT_INVALID;
    }
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
