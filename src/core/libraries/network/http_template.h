// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "http_object.h"

namespace Libraries::Http {

struct HttpTemplate : public HttpObject {
    ~HttpTemplate() override {}
    HttpTemplate() {}
    int httpCtxId;
    std::string userAgent;
    int httpVersion;
    int proxyConf;
};

} // namespace Libraries::Http