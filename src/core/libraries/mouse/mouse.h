// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Mouse {

template <class T>
class RingBufferQueue {
public:
    RingBufferQueue(size_t size) : m_storage(size) {}

    void Push(T item) {
        const size_t index = (m_begin + m_size) % m_storage.size();
        m_storage[index] = std::move(item);
        if (m_size < m_storage.size()) {
            m_size += 1;
        } else {
            m_begin = (m_begin + 1) % m_storage.size();
        }
    }

    std::optional<T> Pop() {
        if (m_size == 0) {
            return {};
        }
        const size_t index = m_begin;
        m_begin = (m_begin + 1) % m_storage.size();
        m_size -= 1;
        return std::move(m_storage[index]);
    }

    std::optional<T> Peek() {
        if (m_size == 0) {
            return {};
        }
        return m_storage[m_begin];
    }

private:
    size_t m_begin = 0;
    size_t m_size = 0;
    std::vector<T> m_storage;
};

struct OrbisMouseData {
    u64 timestamp;
    bool connected;
    u32 buttons;
    s32 x_axis;
    s32 y_axis;
    s32 wheel;
    s32 tilt;
    u8 reserve[8];
};

enum class MouseOpenBehaviour : u8 {
    Normal = 0,
    Merged = 1,
};

struct OrbisMouseOpenParam {
    MouseOpenBehaviour flag;
    u8 reserve[7];
};

extern RingBufferQueue<OrbisMouseData> mouse_states[2];
extern s32 mouse_handles[2];
extern s32 mouse_sdl_handles[2];
extern bool g_lib_init;

s32 PS4_SYSV_ABI sceMouseClose(s32 handle);
s32 PS4_SYSV_ABI sceMouseConnectPort();
s32 PS4_SYSV_ABI sceMouseDebugGetDeviceId();
s32 PS4_SYSV_ABI sceMouseDeviceOpen();
s32 PS4_SYSV_ABI sceMouseDisconnectDevice();
s32 PS4_SYSV_ABI sceMouseDisconnectPort();
s32 PS4_SYSV_ABI sceMouseGetDeviceInfo();
s32 PS4_SYSV_ABI sceMouseInit();
s32 PS4_SYSV_ABI sceMouseMbusInit();
s32 PS4_SYSV_ABI sceMouseOpen(Libraries::UserService::OrbisUserServiceUserId userId, s32 type,
                              s32 index, OrbisMouseOpenParam* pParam);
s32 PS4_SYSV_ABI sceMouseRead(s32 handle, OrbisMouseData* pData, s32 num);
s32 PS4_SYSV_ABI sceMouseSetHandType();
s32 PS4_SYSV_ABI sceMouseSetPointerSpeed();
s32 PS4_SYSV_ABI sceMouseSetProcessPrivilege();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Mouse