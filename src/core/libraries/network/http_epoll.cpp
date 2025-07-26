// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <curl/curl.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "http_epoll.h"
#include "http_request.h"

namespace Libraries::Http {

HttpEpoll::HttpEpoll() : multi(curl_multi_init()) {
    ASSERT(multi);
    curl_easy_setopt(multi, CURLOPT_VERBOSE, 1L);
}

int HttpEpoll::Start(CURL* easy) {
    curl_multi_add_handle(multi, easy);
    // perform kicks off the request but we don't care about the result
    curl_multi_perform(multi, nullptr);

    return 0;
}

int HttpEpoll::StartBlocking(CURL* easy) {
    curl_multi_add_handle(multi, easy);
    curl_multi_perform(multi, nullptr);

    int still_running = 0;
    do {
        CURLMcode mc = curl_multi_perform(multi, &still_running);

        if (!mc && still_running) {
            LOG_DEBUG(Lib_Http, "{} transfers running", still_running);
            mc = curl_multi_poll(multi, nullptr, 0, 100, NULL);
        }
    } while (still_running);

    return 0;
}

int HttpEpoll::Wait(OrbisHttpNBEvent* events, int maxevents, int timeout) {
    int still_running = 0;
    CURLMcode mc = curl_multi_perform(multi, &still_running);

    if (!mc && still_running) {
        LOG_DEBUG(Lib_Http, "{} transfers running", still_running);
        mc = curl_multi_poll(multi, nullptr, 0, timeout / 1000, NULL);
    }

    CURLMsg* msg;
    int current_event = 0;
    do {
        int msgs_ready = 0;
        msg = curl_multi_info_read(multi, &msgs_ready);
        if (msg && (msg->msg == CURLMSG_DONE)) {
            LOG_DEBUG(Lib_Http, "transfer result: {}", (u32)msg->data.result);
            auto easy = msg->easy_handle;
            curl_multi_remove_handle(multi, easy);
            HttpRequest* req;
            ASSERT(!curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req));
            req->done = true;

            events[current_event++] = {.events = 0x3, .id = req->Id(), .userArg = req->arg};
        }
    } while (msg && current_event < maxevents - 1);

    return current_event;
}

} // namespace Libraries::Http