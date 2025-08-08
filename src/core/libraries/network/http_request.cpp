// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <curl/curl.h>

#include "common/assert.h"
#include "common/singleton.h"
#include "http_epoll.h"
#include "http_error.h"
#include "http_request.h"
#include "http_table.h"

namespace {

int ConvertCurlCodeToOrbis(CURLcode e) {
    switch (e) {
    case CURLE_OK:
        return ORBIS_OK;
    default:
        UNREACHABLE_MSG("unhandled curl result {}", (int)e);
    }
}

} // namespace

namespace Libraries::Http {

static size_t write_callback(char* data, size_t size, size_t nmemb, void* arg) {
    HttpRequest* req = static_cast<HttpRequest*>(arg);
    auto realsize = size * nmemb;
    LOG_DEBUG(Lib_Http, "got {} bytes of data in response to reqid = {}", realsize, req->Id());

    return req->ReceiveData(data, realsize);
}

static size_t header_callback(char* buffer, size_t size, size_t nitems, void* arg) {
    HttpRequest* req = static_cast<HttpRequest*>(arg);
    LOG_DEBUG(Lib_Http, "got header {} in response to reqid = {}", std::string_view{buffer, nitems},
              req->Id());

    return req->ReceiveHeader(buffer, nitems);
}

HttpRequest::HttpRequest() : curl(curl_easy_init()) {
    ASSERT(curl);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, this);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
}

u64 HttpRequest::ReceiveData(char* data, u64 bytes) {
    response_data.insert(response_data.end(), data, data + bytes);

    return bytes;
}

u64 HttpRequest::ReceiveHeader(char* data, u64 bytes) {
    response_headers.insert(response_headers.end(), data, data + bytes);

    return bytes;
}

bool HttpRequest::PrepareCurl() {
    ASSERT(!curl_easy_setopt(curl, CURLOPT_URL, url.data()));

    curl_slist_free_all(headers_list);
    headers_list = nullptr;
    headers_cache.clear();

    for (auto& h : headers) {
        LOG_INFO(Lib_Http, "header {}: {}", h.first, h.second);
        ASSERT_MSG(!h.second.empty(), "setting a header to an empty value");
        headers_cache.emplace_back(h.first + ": " + h.second);
        headers_list = curl_slist_append(headers_list, headers_cache.back().data());
    }
    ASSERT(!curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list));

    return true;
}

std::pair<int, u64> HttpRequest::GetResponseContentLength() {
    if (!done) {
        return {1, 0};
    }

    auto len = 0;
    ASSERT(!curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &len));

    std::pair<int, u64> err{1, 0};
    std::pair<int, u64> ok{0, len};
    return len == -1 ? err : ok;
}

std::pair<u8*, u64> HttpRequest::GetResponseHeaders() {
    if (!done) {
        return {nullptr, ORBIS_HTTP_ERROR_BEFORE_SEND};
    }

    return std::make_pair(response_headers.data(), response_headers.size());
}

int HttpRequest::GetStatusCode() {
    if (!done) {
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }

    long status_code = 0;
    ASSERT(!curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code));
    return status_code;
}

int HttpRequest::ReadData(char* buf, u64 size) {
    if (!done) {
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }

    auto left_to_transfer = response_data.size() - response_index;
    if (left_to_transfer == 0) {
        return 0;
    }

    auto to_transfer = std::min(left_to_transfer, size);
    memcpy(buf, response_data.data() + response_index, to_transfer);
    response_index += to_transfer;

    return to_transfer;
}

int HttpRequest::Send(const void* postData, size_t size) {
    HttpEpoll* e;
    auto h = Common::Singleton<HttpTable>::Instance();

    if (e = h->GetObject<HttpEpoll>(epoll); !e) {
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    PrepareCurl();
    return e->Start(curl);
}

int HttpRequest::SendBlocking(const void* postData, size_t size) {
    HttpEpoll* e;
    auto h = Common::Singleton<HttpTable>::Instance();

    if (e = h->GetObject<HttpEpoll>(epoll); !e) {
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    PrepareCurl();
    return e->StartBlocking(curl);
}

} // namespace Libraries::Http