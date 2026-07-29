// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/time.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"
#include "core/libraries/np/np_web_api2/np_web_api2_push_event.h"
#include "core/libraries/system/userservice.h"

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace Libraries::Np::NpWebApi2 {

class UserContext;
class Request;

class LibraryContext {
public:
    LibraryContext(s32 ctx_id, s32 type, s32 http_ctx_id, u64 pool_size)
        : id(ctx_id), type(static_cast<LibraryContextType>(type)), http_ctx_id(http_ctx_id),
          pool_size(pool_size) {}
    LibraryContext(s32 ctx_id, s32 type, s32 http_ctx_id, u64 pool_size, const char* name)
        : id(ctx_id), type(static_cast<LibraryContextType>(type)), http_ctx_id(http_ctx_id),
          pool_size(pool_size), name(name) {}

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

    bool IsDebug() {
        return type == LibraryContextType::Debug;
    }

    bool IsNormal() {
        return type == LibraryContextType::Normal;
    }

    bool IsInternal() {
        return type == LibraryContextType::Internal;
    }

    bool IsPresence() {
        return type == LibraryContextType::Presence;
    }

    bool IsDeleted() {
        return deleting;
    }

    void MarkForDeletion() {
        deleting = true;
    }

    bool IsBusy() {
        std::scoped_lock lk{lock};
        return user_count > 1;
    }

    void CheckTimeout();

    s32 CreatePushEventHandle();
    PushEventHandle* GetPushEventHandle(s32 handle_id);

    void SetHandleEndTime(s32 handle_id) {
        std::scoped_lock lk{lock};
        if (push_event_handles.contains(handle_id)) {
            auto& handle = push_event_handles[handle_id];
            handle->SetEndTime();
        }
    }

    void AbortAllPushEventHandles() {
        std::scoped_lock lk{lock};
        for (auto& [handle_id, handle] : push_event_handles) {
            abortPushEventHandle(id, handle_id);
        }
    }

    bool HasBusyPushEventHandles() {
        std::scoped_lock lk{lock};
        for (auto& [handle_id, handle] : push_event_handles) {
            if (handle->IsBusy()) {
                return true;
            }
        }
        return false;
    }

    s32 DeletePushEventHandle(PushEventHandle* handle);

    void DeleteAllPushEventHandles() {
        for (auto& [handle_id, handle] : push_event_handles) {
            delete handle;
        }
        push_event_handles.clear();
    }

    s32 CreatePushEventFilter(s32 handle_id, const char* np_service_name,
                              OrbisNpServiceLabel np_service_label,
                              const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                              u64 filter_param_num, bool internal);

    PushEventFilter* GetPushEventFilter(s32 filter_id) {
        std::scoped_lock lk{lock};
        if (!push_event_filters.contains(filter_id)) {
            return nullptr;
        }
        return push_event_filters[filter_id];
    }

    void DeletePushEventFilter(PushEventFilter* filter) {
        std::scoped_lock lk{lock};
        s32 filter_id = filter->GetId();
        if (push_event_filters.contains(filter_id)) {
            push_event_filters.erase(filter_id);
        }
        delete filter;
    }

    void DeleteAllPushEventFilters() {
        for (auto& [filter_id, filter] : push_event_filters) {
            delete filter;
        }
        push_event_filters.clear();
    }

    s32 CreateUserContext(Libraries::UserService::OrbisUserServiceUserId user_id);
    UserContext* GetUserContext(s32 user_ctx_id);
    UserContext* GetUserContextByUserId(Libraries::UserService::OrbisUserServiceUserId user_id);

    void RemoveUserContext(s32 user_ctx_id) {
        std::scoped_lock lk{lock};
        if (user_contexts.contains(user_ctx_id)) {
            user_contexts.erase(user_ctx_id);
        }
    }

    void DeleteAllUserContexts() {
        std::scoped_lock lk{lock};
        for (auto& [user_ctx_id, user_ctx] : user_contexts) {
            deleteUserContext(user_ctx_id);
        }
        user_contexts.clear();
    }

private:
    s32 id{};
    s32 http_ctx_id{};
    s32 user_count{};
    enum LibraryContextType : s32 {
        Debug = 0,
        Normal = 1,
        Internal = 2,
        Presence = 3,
    } type;
    u64 pool_size{};
    bool deleting{};
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
        : parent_ctx(parent), id(user_ctx_id), user_id(user_id) {}

    void Lock() {
        parent_ctx->Lock();
    }

    void Unlock() {
        parent_ctx->Unlock();
    }

    void AddUser() {
        Lock();
        user_count++;
        Unlock();
    }

    void RemoveUser() {
        Lock();
        user_count--;
        Unlock();
    }

    bool IsDeleted() {
        return deleting;
    }

    void MarkForDeletion() {
        deleting = true;
    }

    bool IsBusy() {
        parent_ctx->Lock();
        s32 users = user_count;
        parent_ctx->Unlock();
        // Assumes caller is a user
        return users > 1;
    }

    s32 GetId() {
        return id;
    }

    Libraries::UserService::OrbisUserServiceUserId GetUserId() {
        return user_id;
    }

    s32 GetHttpTemplateId() {
        return http_template_id;
    }

    s32 Initialize();

    void CheckTimeout();

    s32 CreatePushEventCallback(s32 filter_id, OrbisNpWebApi2PushEventCallback cb_func,
                                void* user_arg);
    s32 DeletePushEventCallback(s32 callback_id);

    void CreatePushContext(OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
    PushEventPushContext* GetPushContext(const OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
    void DeletePushContext(PushEventPushContext* push_ctx_id);

    s32 CreatePushContextCallback(s32 filter_id, OrbisNpWebApi2PushEventPushContextCallback cb_func,
                                  void* user_arg);
    s32 DeletePushContextCallback(s32 callback_id);

    s32 CreateRequest(const char* api_group, const char* path, const char* method,
                      const OrbisNpWebApi2ContentParameter* content_parameter, bool multipart,
                      Request** request);
    Request* GetRequest(s64 request_id);
    bool HasBusyRequests();
    void AbortAllRequests();

    void RemoveRequest(s64 request_id) {
        parent_ctx->Lock();
        if (requests.contains(request_id)) {
            requests.erase(request_id);
        }
        parent_ctx->Unlock();
    }

    void Delete();

private:
    s32 id{};
    Libraries::UserService::OrbisUserServiceUserId user_id{};
    s32 user_count{};
    s32 http_template_id{};
    bool deleting{};
    bool push_contexts_busy{};
    LibraryContext* parent_ctx{};
    std::string user_agent{};
    std::map<s64, Request*> requests{};
    std::map<s32, PushEventCallback*> push_event_callbacks{};
    std::map<s64, PushEventPushContext*> push_contexts{};
    std::map<s32, PushEventPushContextCallback*> push_context_callbacks{};
};

class Request {
public:
    Request(LibraryContext* parent, s64 request_id, const char* api_group, const char* path,
            const char* method, bool multipart)
        : parent_ctx(parent), id(request_id), api_group(api_group), path(path), method(method),
          multipart_supported(multipart) {}
    Request(LibraryContext* parent, s64 request_id, const char* api_group, const char* path,
            const char* method, bool multipart,
            const OrbisNpWebApi2ContentParameter* content_parameter)
        : parent_ctx(parent), id(request_id), api_group(api_group), path(path), method(method),
          multipart_supported(multipart), content_type(content_parameter->content_type),
          content_length(content_parameter->content_length) {}

    void Lock() {
        parent_ctx->Lock();
    }

    void Unlock() {
        parent_ctx->Unlock();
    }

    void AddUser() {
        Lock();
        user_count++;
        Unlock();
    }

    void RemoveUser() {
        Lock();
        user_count--;
        Unlock();
    }

    s64 GetId() {
        return id;
    }

    s32 GetHttpRequestId() {
        return http_request_id;
    }

    bool IsMultipart() {
        return multipart_supported;
    }

    bool HasSent() {
        return sent;
    }

    void MarkSent() {
        sent = true;
    }

    bool Aborted() {
        return aborted;
    }

    bool Expired() {
        return expired;
    }

    bool OutOfData() {
        return content_length == sent_data;
    }

    void SetEndTime() {
        if (timeout != 0 && end_time == 0) {
            end_time = Libraries::Kernel::sceKernelGetProcessTime() + timeout;
        }
    }

    void ClearEndTime() {
        end_time = 0;
    }

    std::string& GetApiGroup() {
        return api_group;
    }

    std::string& GetPath() {
        return path;
    }

    std::string& GetMethod() {
        return method;
    }

    std::string& GetContentType() {
        return content_type;
    }

    void SetState(s32 state) {
        send_state = state;
    }

    bool IsDeleted() {
        return deleting;
    }

    void MarkForDeletion() {
        deleting = true;
    }

    bool IsBusy() {
        parent_ctx->Lock();
        s32 users = user_count;
        parent_ctx->Unlock();
        // Assumes caller is a user
        return users > 1;
    }

    s32 AddHttpRequestHeader(const char* field_name, const char* field_value);

    void SetTimeout(u32 new_timeout) {
        this->timeout = new_timeout;
    }

    void CheckTimeout() {
        u64 time = Libraries::Kernel::sceKernelGetProcessTime();
        if (!expired && end_time != 0 && end_time < time) {
            expired = true;
            Abort();
        }
    }

    s32 CreateHttpRequest(s32 http_template_id, const char* url);
    s32 SendHttpRequest(void* data, u64 data_size);
    s32 GetAllHttpResponseHeaders();
    s32 ParseHttpResponseHeaders(const char* field_name, char* value, u64 value_size,
                                 u64* value_size_out);
    s32 Abort();
    s32 Delete(s32* http_request_id);

private:
    s64 id{};
    u64 content_length{};
    char* http_response_headers{};
    u64 http_response_header_size{};
    s32 user_count{};
    s32 http_request_id{};
    u32 timeout{};
    s32 sent_data{};
    s32 send_state{};
    bool multipart_supported{};
    bool sent{};
    bool aborted{};
    bool expired{};
    bool deleting{};
    u64 end_time{};
    LibraryContext* parent_ctx{};
    std::string api_group{};
    std::string path{};
    std::string method{};
    std::string content_type{};

    struct HttpRequestHeader {
        std::string field_name;
        std::string field_value;
    };
    std::vector<HttpRequestHeader*> http_headers{};
};

}; // namespace Libraries::Np::NpWebApi2