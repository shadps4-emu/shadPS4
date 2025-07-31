// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>

#include "common/types.h"
#include "http_object.h"

namespace Libraries::Http {

class HttpConnection;
class HttpEpoll;
class HttpRequest;
class HttpTemplate;

template <typename T>
struct type_value {
    static HttpObjectType value() {
        if constexpr (std::is_same_v<T, HttpTemplate>) {
            return HttpTemplateType;
        } else if constexpr (std::is_same_v<T, HttpConnection>) {
            return HttpConnectionType;
        } else if constexpr (std::is_same_v<T, HttpRequest>) {
            return HttpRequestType;
        } else if constexpr (std::is_same_v<T, HttpEpoll>) {
            return HttpEpollType;
        } else {
            return HttpInvalidType;
        }
    }
};

class HttpTable {
public:
    HttpTable() = default;
    virtual ~HttpTable() = default;

    template <typename T, typename... Args>
    std::pair<s32, T*> Create(Args&&... args) {
        std::scoped_lock lock{m_mutex};
        m_objects.emplace_back(std::make_unique<T>(args...));
        m_objects.back()->m_type = type_value<T>::value();
        m_objects.back()->m_id = m_objects.size();
        return {m_objects.size(), static_cast<T*>(m_objects.back().get())};
    }

    void DeleteHandle(s32 d) {
        std::scoped_lock lock{m_mutex};
        m_objects[d - 1].reset(nullptr);
    }

    template <typename T>
    T* GetObject(s32 d) {
        std::scoped_lock lock{m_mutex};
        if (d > 0 && d <= m_objects.size() && m_objects[d - 1] &&
            m_objects[d - 1]->m_type == type_value<T>::value()) {
            return static_cast<T*>(m_objects[d - 1].get());
        }

        return nullptr;
    }

private:
    std::vector<std::unique_ptr<HttpObject>> m_objects;
    std::mutex m_mutex;
};

} // namespace Libraries::Http