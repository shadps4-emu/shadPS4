// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Http {

enum HttpObjectType : u32 {
    HttpInvalidType = 0,
    HttpTemplateType = 1,
    HttpConnectionType = 2,
    HttpRequestType = 3,
    HttpEpollType = 4,
};

struct HttpObject {
    virtual ~HttpObject() {}
    bool nonBlocking{};
    u64 connectTimeout{};
    u64 sendTimeout{};
    u64 recvTimeout{};
    u64 resolveTimeout{};

    s32 Id() const {
        return m_id;
    }

private:
    HttpObjectType m_type;
    s32 m_id{};
    friend class HttpTable;
};

} // namespace Libraries::Http