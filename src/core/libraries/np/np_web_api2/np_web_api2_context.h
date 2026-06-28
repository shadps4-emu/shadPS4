// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/np/np_web_api2/np_web_api2_push_event.h"

#include <map>
#include <mutex>
#include <string>

namespace Libraries::Np::NpWebApi2 {

// Forward declarations to prevent compile issues
class LibraryContext;
class UserContext;
class Request;

class LibraryContext {
public:
    LibraryContext(s32 ctx_id, s32 http_ctx_id) : id(ctx_id), http_ctx_id(http_ctx_id) {};
    LibraryContext(s32 ctx_id, s32 http_ctx_id, const char* name)
        : id(ctx_id), http_ctx_id(http_ctx_id), name(name) {};

    void Lock() {
        lock.lock();
    };

    void Unlock() {
        lock.unlock();
    };

    s32 GetId() {
        return id;
    };

    s32 GetHttpCtxId() {
        return http_ctx_id;
    };

private:
    s32 id{};
    s32 http_ctx_id{};
    std::recursive_mutex lock{};
    std::string name{};
    std::map<s32, PushEventHandle*> push_event_handles{};
    std::map<s32, PushEventFilter*> push_event_filters{};
};

class UserContext {};

class Request {};

}; // namespace Libraries::Np::NpWebApi2