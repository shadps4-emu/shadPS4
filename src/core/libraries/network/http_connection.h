// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "http_object.h"
#include "http_template.h"

namespace Libraries::Http {

struct HttpConnection : public HttpObject {
    ~HttpConnection() override {}
    HttpConnection(HttpTemplate& template_) : m_template(template_) {}
    HttpTemplate m_template;
    std::string url;
    bool keepAlive;
};

} // namespace Libraries::Http