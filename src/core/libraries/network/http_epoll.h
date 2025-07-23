// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "http_object.h"

namespace Libraries::Http {

class OrbisHttpNBEvent;

struct HttpEpoll : public HttpObject {
    ~HttpEpoll() override {}
    HttpEpoll();
    int httpCtxId{};

    int Start(CURL* easy);
    int StartBlocking(CURL* easy);

    int Wait(OrbisHttpNBEvent* events, int maxevents, int timeout);

private:
    CURLM* multi = nullptr;
};

} // namespace Libraries::Http