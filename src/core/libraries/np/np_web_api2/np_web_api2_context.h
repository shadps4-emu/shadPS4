// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/np/np_web_api2/np_web_api2_push_event.h"
#include "core/libraries/system/userservice.h"

#include <atomic>
#include <map>
#include <mutex>
#include <string>

namespace Libraries::Np::NpWebApi2 {

class LibraryContext;
class UserContext;
class Request;

class LibraryContext {
public:
    LibraryContext(s32 ctx_id, s32 http_ctx_id, u64 pool_size)
        : id(ctx_id), http_ctx_id(http_ctx_id), pool_size(pool_size) {}
    LibraryContext(s32 ctx_id, s32 http_ctx_id, u64 pool_size, const char* name)
        : id(ctx_id), http_ctx_id(http_ctx_id), pool_size(pool_size), name(name) {}

    void Lock() {
        lock.lock();
    }

    void Unlock() {
        lock.unlock();
    }

    void AddUser() {
        std::scoped_lock lk{lock};
        user_count++;
    }

    void RemoveUser() {
        std::scoped_lock lk{lock};
        user_count--;
    }

    s32 GetId() {
        return id;
    }

    s32 GetHttpCtxId() {
        return http_ctx_id;
    }

    u64 GetPoolSize() {
        return pool_size;
    }

    const char* GetName() {
        return name.empty() ? nullptr : name.data();
    }

    s32 CreateUserContext(Libraries::UserService::OrbisUserServiceUserId user_id);
    UserContext* GetUserContext(s32 user_ctx_id);
    UserContext* GetUserContextByUserId(Libraries::UserService::OrbisUserServiceUserId user_id);

    void RemoveUserContext(s32 user_ctx_id) {
        std::scoped_lock lk{lock};
        user_contexts.erase(user_ctx_id);
    }

private:
    s32 id{};
    s32 http_ctx_id{};
    s32 user_count{};
    u64 pool_size{};
    std::recursive_mutex lock{};
    std::string name{};
    std::map<s32, UserContext*> user_contexts{};
    std::map<s32, PushEventHandle*> push_event_handles{};
    std::map<s32, PushEventFilter*> push_event_filters{};
};

class UserContext {
public:
    UserContext(LibraryContext* parent, s32 user_ctx_id,
                Libraries::UserService::OrbisUserServiceUserId user_id)
        : parent_ctx(parent), id(user_ctx_id), user_id(user_id) {};

    void Lock() {
        parent_ctx->Lock();
    }

    void Unlock() {
        parent_ctx->Unlock();
    }

    void AddUser() {
        parent_ctx->Lock();
        user_count++;
        parent_ctx->Unlock();
    }

    void RemoveUser() {
        parent_ctx->Lock();
        user_count--;
        parent_ctx->Unlock();
    }

    s32 GetId() {
        return id;
    }

    Libraries::UserService::OrbisUserServiceUserId GetUserId() {
        return user_id;
    }

    s32 Initialize();

private:
    s32 id{};
    Libraries::UserService::OrbisUserServiceUserId user_id{};
    s32 user_count{};
    s32 http_template_id{};
    LibraryContext* parent_ctx{};
    std::string user_agent{};
};

class Request {};

}; // namespace Libraries::Np::NpWebApi2