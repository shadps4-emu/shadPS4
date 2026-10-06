// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <string_view>

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/ipmi/ipmi.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"

namespace Libraries::Ipmi {

constexpr u64 DefaultBufferSize = 3840;
constexpr u64 ServerWorkingMemoryOverhead = 0x100;

struct ServerConfig {
    u64 buffer_sizes[2];
    void* event_handler;
    u8 flag_0x18;
    char name[25];
    u16 flag_0x32;
};
static_assert(offsetof(ServerConfig, event_handler) == 0x10);
static_assert(offsetof(ServerConfig, name) == 0x19);
static_assert(offsetof(ServerConfig, flag_0x32) == 0x32);
static_assert(sizeof(ServerConfig) == 0x38);

enum ServerState : u8 {
    Idle = 0,
    Running = 1 << 1,
    ShutdownRequested = 1 << 2,
};

struct ServerImpl {
    std::mutex mutex;
    std::condition_variable cv;
    u8 state = Idle;
    void* user_data = nullptr;
};

struct Server {
    void* const* vtable;
    ServerImpl* impl;
};

static s32 PS4_SYSV_ABI ServerDestroy(Server* server) {
    if (server->impl == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    {
        std::scoped_lock lock{server->impl->mutex};
        if (server->impl->state != Idle) {
            return ORBIS_KERNEL_ERROR_EBUSY;
        }
    }
    delete server->impl;
    server->impl = nullptr;
    return ORBIS_OK;
}

static void* PS4_SYSV_ABI ServerGetUserData(Server* server) {
    return server->impl != nullptr ? server->impl->user_data : nullptr;
}

static s32 PS4_SYSV_ABI ServerRunDispatcher(Server* server, void* memory, u64 size) {
    ServerImpl* impl = server->impl;
    if (impl == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    std::unique_lock lock{impl->mutex};
    if (impl->state != Idle) {
        impl->state &= ~ShutdownRequested;
        return ORBIS_KERNEL_ERROR_EBUSY;
    }
    impl->state = Running;
    impl->cv.wait(lock, [impl] { return (impl->state & ShutdownRequested) != 0; });
    impl->state = Idle;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI ServerTryDispatch(Server* server, void* memory, u64 size) {
    ServerImpl* impl = server->impl;
    if (impl == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    std::scoped_lock lock{impl->mutex};
    if (impl->state != Idle) {
        impl->state &= ~ShutdownRequested;
        return ORBIS_KERNEL_ERROR_EBUSY;
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI ServerShutdownDispatcher(Server* server) {
    ServerImpl* impl = server->impl;
    if (impl == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    std::scoped_lock lock{impl->mutex};
    if ((impl->state & ShutdownRequested) != 0) {
        return ORBIS_KERNEL_ERROR_EBUSY;
    }
    impl->state |= ShutdownRequested;
    impl->cv.notify_all();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI ServerCreateSession(Server* server, void** session, void* arg1,
                                            void* arg2) {
    LOG_ERROR(Lib_Ipmi, "(STUBBED) called");
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

static void PS4_SYSV_ABI ServerDestructor(Server* server) {
    delete server->impl;
    server->impl = nullptr;
}

static void* const ServerVtable[] = {
    reinterpret_cast<void*>(&ServerDestroy),
    reinterpret_cast<void*>(&ServerGetUserData),
    reinterpret_cast<void*>(&ServerRunDispatcher),
    reinterpret_cast<void*>(&ServerTryDispatch),
    reinterpret_cast<void*>(&ServerShutdownDispatcher),
    reinterpret_cast<void*>(&ServerCreateSession),
    reinterpret_cast<void*>(&ServerDestructor),
    reinterpret_cast<void*>(&ServerDestructor),
};

static void PS4_SYSV_ABI ServerConfigConstructor(ServerConfig* config) {
    config->buffer_sizes[0] = DefaultBufferSize;
    config->buffer_sizes[1] = DefaultBufferSize;
    config->event_handler = nullptr;
    config->flag_0x18 = 0;
    config->name[0] = '\0';
    config->flag_0x32 = 0;
}

static u64 PS4_SYSV_ABI ServerConfigEstimateTempWorkingMemorySize(const ServerConfig* config) {
    return std::max(config->buffer_sizes[0], config->buffer_sizes[1]) + ServerWorkingMemoryOverhead;
}

static s32 PS4_SYSV_ABI ServerCreate(Server** out_server, const ServerConfig* config,
                                     void* user_data, void* memory) {
    if (out_server == nullptr || config == nullptr || memory == nullptr ||
        config->event_handler == nullptr || config->name[0] == '\0') {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const std::string_view name{config->name, strnlen(config->name, sizeof(config->name))};
    LOG_INFO(Lib_Ipmi, "Creating server '{}'", name);

    auto* server = static_cast<Server*>(memory);
    server->vtable = ServerVtable;
    server->impl = new ServerImpl;
    server->impl->user_data = user_data;
    *out_server = server;
    return ORBIS_OK;
}

static void PS4_SYSV_ABI EventHandlerOnSyncMethodDispatch(void*, void*, u32, void*, u64, u64, void*,
                                                          u64) {}

static void PS4_SYSV_ABI EventHandlerOnAsyncMethodDispatch(void*, void*, u32, u32, void*, u64,
                                                           void*, u64) {}

static void PS4_SYSV_ABI EventHandlerDestructor(void*) {}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("vo2FiTXAekk", "libSceIpmi", 1, "libSceIpmi", ServerConfigConstructor);
    LIB_FUNCTION("OXc1BksfEIg", "libSceIpmi", 1, "libSceIpmi",
                 ServerConfigEstimateTempWorkingMemorySize);
    LIB_FUNCTION("hoWSsRT0ktA", "libSceIpmi", 1, "libSceIpmi", ServerCreate);
    LIB_FUNCTION("iVUJ-VqTtek", "libSceIpmi", 1, "libSceIpmi", EventHandlerOnSyncMethodDispatch);
    LIB_FUNCTION("wQ9pO7tm3DU", "libSceIpmi", 1, "libSceIpmi", EventHandlerOnAsyncMethodDispatch);
    LIB_FUNCTION("Z5i0--Vqfwg", "libSceIpmi", 1, "libSceIpmi", EventHandlerDestructor);
}

} // namespace Libraries::Ipmi
