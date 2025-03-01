// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "http_error.h"

namespace Libraries::Http {

int PS4_SYSV_ABI sceHttpAbortRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortRequestForce() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortWaitRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddCookie() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddQuery() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddRequestHeader() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddRequestHeaderRaw() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheExport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheFlush() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheImport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCacheRedirectedConnectionEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieExport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieFlush() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieImport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateConnection() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateConnectionWithURL() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateEpoll() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequest2() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL2() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateTemplate() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgEnableProfile() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgGetConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgGetRequestStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgSetPrintf() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowMemoryPoolStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowRequestStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteConnection() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteTemplate() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDestroyEpoll() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAllResponseHeaders() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAuthEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAutoRedirect() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookie() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookieEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookieStats() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpoll() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetLastErrno() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetMemoryPoolStats() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetNonblock() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetRegisteredCtxIds() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetResponseContentLength() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetStatusCode() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, std::size_t poolSize) {
    LOG_ERROR(Lib_Http, "(DUMMY) called libnetMemId = {} libsslCtxId = {} poolSize = {}",
              libnetMemId, libsslCtxId, poolSize);
    // return a value >1
    static int id = 0;
    return ++id;
}

int PS4_SYSV_ABI sceHttpParseResponseHeader() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpParseStatusLine() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpReadData() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRedirectCacheFlush() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRemoveRequestHeader() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRequestGetAllHeaders() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsDisableOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsDisableOptionPrivate() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOptionPrivate() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSendRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAuthEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAuthInfoCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAutoRedirect() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetChunkedTransferEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetConnectTimeOut() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxNum() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxNumPerDomain() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxSize() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieRecvCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieSendCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieTotalMaxSize() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetDelayBuildRequestEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpoll() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetHttp09Enabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetInflateGZIPEnabled() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetNonblock() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetPolicyOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetPriorityOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetProxy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRecvBlockSize() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRecvTimeOut() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRedirectCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRequestContentLength() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRequestStatusCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResolveRetry() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResolveTimeOut() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetSendTimeOut() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetSocketCreationCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsFreeCaList() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetCaList() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetSslError() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsLoadCert() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetMinSslVersion() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetSslCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetSslVersion() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsUnloadCert() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTerm() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTryGetNonblock() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTrySetNonblock() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUnsetEpoll() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriBuild() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriCopy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriEscape() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriMerge() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

// Helper function to calculate the length of a URI component
static size_t calculateComponentLength(const char* start, const char* end) {
    return (end > start) ? (size_t)(end - start) : 0;
}

// Helper function to parse the port from the URI
static int parsePort(const char* uri, int* offset) {
    int port = 0;
    int digits = 0;

    while (uri[*offset] >= '0' && uri[*offset] <= '9' && digits < 5) {
        port = port * 10 + (uri[*offset] - '0');
        (*offset)++;
        digits++;
    }

    if (digits == 0 || port > 65535) {
        return 0; // Invalid port
    }
    return port;
}

int PS4_SYSV_ABI sceHttpUriParse(OrbisHttpUriElement* out, const char* srcUri, void* pool,
                                 size_t* require, size_t prepare) {
    LOG_INFO(Lib_Http, "srcUri = {}", std::string(srcUri));
    if (!srcUri) {
        LOG_ERROR(Lib_Http, "invalid values");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }

    // Initialize the output structure if provided
    if (out) {
        memset(out, 0, sizeof(OrbisHttpUriElement));
    }

    // Calculate the required buffer size
    size_t requiredBufferSize = 0;
    char* currentPos = (char*)srcUri;

    // Scheme (e.g., "http:")
    char* schemeEnd = strchr(currentPos, ':');
    if (!schemeEnd) {
        LOG_ERROR(Lib_Http, "invalid url");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }
    requiredBufferSize +=
        calculateComponentLength(currentPos, schemeEnd) + 1; // Include null terminator
    currentPos = schemeEnd + 1;

    // Check if the URI is opaque or hierarchical
    bool isOpaque = true; // Assume opaque by default
    if (strncmp(currentPos, "//", 2) == 0) {
        isOpaque = false; // Hierarchical if "//" is present
        currentPos += 2;  // Skip "//"
    }

    // in case it starts with file://///
    if (strncmp(currentPos, "//", 2) == 0) {
        currentPos += 2; // Skip "//"
    }

    // Host and port (e.g., "example.com:8080")
    char* hostEnd = strchr(currentPos, '/');
    if (!hostEnd) {
        hostEnd = currentPos + strlen(currentPos);
    }

    // Check for credentials (username:password@host)
    char* atSymbol = strchr(currentPos, '@');
    if (atSymbol && atSymbol < hostEnd) {
        requiredBufferSize +=
            calculateComponentLength(currentPos, atSymbol) + 1; // Include null terminator
        currentPos = atSymbol + 1;
    }

    // Check for port (host:port)
    char* colon = strchr(currentPos, ':');
    if (colon && colon < hostEnd) {
        requiredBufferSize +=
            calculateComponentLength(currentPos, colon) + 1; // Include null terminator
        currentPos = colon + 1;
    }

    // Host
    requiredBufferSize +=
        calculateComponentLength(currentPos, hostEnd) + 1; // Include null terminator
    currentPos = hostEnd;

    // Path (e.g., "/path")
    char* pathEnd = strchr(currentPos, '?');
    if (!pathEnd) {
        pathEnd = strchr(currentPos, '#');
        if (!pathEnd) {
            pathEnd = currentPos + strlen(currentPos);
        }
    }
    requiredBufferSize +=
        calculateComponentLength(currentPos, pathEnd) + 1; // Include null terminator
    currentPos = pathEnd;

    // Query (e.g., "?query=value")
    if (*currentPos == '?') {
        currentPos++;
        char* queryEnd = strchr(currentPos, '#');
        if (!queryEnd) {
            queryEnd = currentPos + strlen(currentPos);
        }
        requiredBufferSize +=
            calculateComponentLength(currentPos, queryEnd) + 1; // Include null terminator
        currentPos = queryEnd;
    }

    // Fragment (e.g., "#fragment")
    if (*currentPos == '#') {
        currentPos++;
        requiredBufferSize +=
            calculateComponentLength(currentPos, currentPos + strlen(currentPos)) +
            1; // Include null terminator
    }

    // Return the required buffer size
    if (require) {
        *require = requiredBufferSize;
    }

    // If pool is NULL, we're done
    if (!pool) {
        return ORBIS_OK;
    }

    // Check if the provided buffer is large enough
    if (prepare < requiredBufferSize) {
        LOG_ERROR(Lib_Http, "out of memory");
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
    }

    // Copy the URI components to the buffer
    char* buffer = (char*)pool;
    strncpy(buffer, srcUri, prepare);
    buffer[prepare - 1] = '\0'; // Ensure null termination

    // Parse and assign the components to the output structure if provided
    if (out) {
        // Scheme
        schemeEnd = strchr(buffer, ':');
        *schemeEnd = '\0';
        out->scheme = buffer;
        buffer = schemeEnd + 1;

        // Opaque flag
        out->opaque = isOpaque;

        // Host and port
        if (!isOpaque) {
            buffer += 2; // Skip "//"
        }

        hostEnd = strchr(buffer, '/');
        if (!hostEnd) {
            hostEnd = buffer + strlen(buffer);
        }

        // Credentials (username:password@host)
        atSymbol = strchr(buffer, '@');
        if (atSymbol && atSymbol < hostEnd) {
            *atSymbol = '\0';
            colon = strchr(buffer, ':');
            if (colon) {
                *colon = '\0';
                out->username = buffer;
                out->password = colon + 1;
            } else {
                out->username = buffer;
            }
            buffer = atSymbol + 1;
        }

        // Port (host:port)
        colon = strchr(buffer, ':');
        if (colon && colon < hostEnd) {
            *colon = '\0';
            int offset = colon + 1 - buffer;
            out->port = parsePort(buffer, &offset);
            if (out->port == 0) {
                LOG_ERROR(Lib_Http, "invalid url");
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
        }

        // Host
        *hostEnd = '\0';
        out->hostname = buffer;
        buffer = hostEnd + 1;

        // Path
        pathEnd = strchr(buffer, '?');
        if (!pathEnd) {
            pathEnd = strchr(buffer, '#');
            if (!pathEnd) {
                pathEnd = buffer + strlen(buffer);
            }
        }
        *pathEnd = '\0';
        out->path = buffer;
        buffer = pathEnd + 1;

        // Query
        if (*buffer == '?') {
            buffer++;
            char* queryEnd = strchr(buffer, '#');
            if (!queryEnd) {
                queryEnd = buffer + strlen(buffer);
            }
            *queryEnd = '\0';
            out->query = buffer;
            buffer = queryEnd + 1;
        }

        // Fragment
        if (*buffer == '#') {
            buffer++;
            out->fragment = buffer;
        }

        // Initialize the reserved field to zero
        memset(out->reserved, 0, sizeof(out->reserved)); // RE indicates some works here but haven't
                                                         // been added not sure if it neccesary
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, size_t srcSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriUnescape(char* out, size_t* require, size_t prepare, const char* in) {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpWaitRequest() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceHttp(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hvG6GfBMXg8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAbortRequest);
    LIB_FUNCTION("JKl06ZIAl6A", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAbortRequestForce);
    LIB_FUNCTION("sWQiqKvYTVA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAbortWaitRequest);
    LIB_FUNCTION("mNan6QSnpeY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAddCookie);
    LIB_FUNCTION("JM58a21mtrQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAddQuery);
    LIB_FUNCTION("EY28T2bkN7k", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAddRequestHeader);
    LIB_FUNCTION("lGAjftanhFs", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAddRequestHeaderRaw);
    LIB_FUNCTION("Y1DCjN-s2BA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAuthCacheExport);
    LIB_FUNCTION("zzB0StvRab4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAuthCacheFlush);
    LIB_FUNCTION("wF0KcxK20BE", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpAuthCacheImport);
    LIB_FUNCTION("A7n9nNg7NBg", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpCacheRedirectedConnectionEnabled);
    LIB_FUNCTION("nOkViL17ZOo", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCookieExport);
    LIB_FUNCTION("seCvUt91WHY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCookieFlush);
    LIB_FUNCTION("pFnXDxo3aog", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCookieImport);
    LIB_FUNCTION("Kiwv9r4IZCc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateConnection);
    LIB_FUNCTION("qgxDBjorUxs", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpCreateConnectionWithURL);
    LIB_FUNCTION("6381dWF+xsQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateEpoll);
    LIB_FUNCTION("tsGVru3hCe8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateRequest);
    LIB_FUNCTION("rGNm+FjIXKk", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateRequest2);
    LIB_FUNCTION("Aeu5wVKkF9w", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateRequestWithURL);
    LIB_FUNCTION("Cnp77podkCU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateRequestWithURL2);
    LIB_FUNCTION("0gYjPTR-6cY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpCreateTemplate);
    LIB_FUNCTION("Lffcxao-QMM", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgEnableProfile);
    LIB_FUNCTION("6gyx-I0Oob4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgGetConnectionStat);
    LIB_FUNCTION("fzzBpJjm9Kw", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgGetRequestStat);
    LIB_FUNCTION("VmqSnjZ5mE4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgSetPrintf);
    LIB_FUNCTION("KJtUHtp6y0U", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgShowConnectionStat);
    LIB_FUNCTION("oEuPssSYskA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgShowMemoryPoolStat);
    LIB_FUNCTION("L2gM3qptqHs", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgShowRequestStat);
    LIB_FUNCTION("pxBsD-X9eH0", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDbgShowStat);
    LIB_FUNCTION("P6A3ytpsiYc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDeleteConnection);
    LIB_FUNCTION("qe7oZ+v4PWA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDeleteRequest);
    LIB_FUNCTION("4I8vEpuEhZ8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDeleteTemplate);
    LIB_FUNCTION("wYhXVfS2Et4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpDestroyEpoll);
    LIB_FUNCTION("1rpZqxdMRwQ", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpGetAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("aCYPMSUIaP8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetAllResponseHeaders);
    LIB_FUNCTION("9m8EcOGzcIQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetAuthEnabled);
    LIB_FUNCTION("mmLexUbtnfY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetAutoRedirect);
    LIB_FUNCTION("L-DwVoHXLtU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetConnectionStat);
    LIB_FUNCTION("+G+UsJpeXPc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetCookie);
    LIB_FUNCTION("iSZjWw1TGiA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetCookieEnabled);
    LIB_FUNCTION("xkymWiGdMiI", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetCookieStats);
    LIB_FUNCTION("7j9VcwnrZo4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetEpoll);
    LIB_FUNCTION("IQOP6McWJcY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetEpollId);
    LIB_FUNCTION("0onIrKx9NIE", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetLastErrno);
    LIB_FUNCTION("16sMmVuOvgU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetMemoryPoolStats);
    LIB_FUNCTION("Wq4RNB3snSQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetNonblock);
    LIB_FUNCTION("hkcfqAl+82w", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetRegisteredCtxIds);
    LIB_FUNCTION("yuO2H2Uvnos", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpGetResponseContentLength);
    LIB_FUNCTION("0a2TBNfE3BU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpGetStatusCode);
    LIB_FUNCTION("A9cVMUtEp4Y", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpInit);
    LIB_FUNCTION("hPTXo3bICzI", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpParseResponseHeader);
    LIB_FUNCTION("Qq8SfuJJJqE", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpParseStatusLine);
    LIB_FUNCTION("P5pdoykPYTk", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpReadData);
    LIB_FUNCTION("u05NnI+P+KY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpRedirectCacheFlush);
    LIB_FUNCTION("zNGh-zoQTD0", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpRemoveRequestHeader);
    LIB_FUNCTION("4fgkfVeVsGU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpRequestGetAllHeaders);
    LIB_FUNCTION("mSQCxzWTwVI", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsDisableOption);
    LIB_FUNCTION("zJYi5br6ZiQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsDisableOptionPrivate);
    LIB_FUNCTION("f42K37mm5RM", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsEnableOption);
    LIB_FUNCTION("I4+4hKttt1w", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsEnableOptionPrivate);
    LIB_FUNCTION("1e2BNwI-XzE", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSendRequest);
    LIB_FUNCTION("HRX1iyDoKR8", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("qFg2SuyTJJY", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetAuthEnabled);
    LIB_FUNCTION("jf4TB2nUO40", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetAuthInfoCallback);
    LIB_FUNCTION("T-mGo9f3Pu4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetAutoRedirect);
    LIB_FUNCTION("PDxS48xGQLs", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetChunkedTransferEnabled);
    LIB_FUNCTION("0S9tTH0uqTU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetConnectTimeOut);
    LIB_FUNCTION("XNUoD2B9a6A", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieEnabled);
    LIB_FUNCTION("pM--+kIeW-8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieMaxNum);
    LIB_FUNCTION("Kp6juCJUJGQ", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetCookieMaxNumPerDomain);
    LIB_FUNCTION("7Y4364GBras", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieMaxSize);
    LIB_FUNCTION("Kh6bS2HQKbo", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieRecvCallback);
    LIB_FUNCTION("GnVDzYfy-KI", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieSendCallback);
    LIB_FUNCTION("pHc3bxUzivU", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetCookieTotalMaxSize);
    LIB_FUNCTION("8kzIXsRy1bY", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetDefaultAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("22buO-UufJY", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetDelayBuildRequestEnabled);
    LIB_FUNCTION("-xm7kZQNpHI", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetEpoll);
    LIB_FUNCTION("LG1YW1Uhkgo", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetEpollId);
    LIB_FUNCTION("pk0AuomQM1o", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetHttp09Enabled);
    LIB_FUNCTION("i9mhafzkEi8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetInflateGZIPEnabled);
    LIB_FUNCTION("s2-NPIvz+iA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetNonblock);
    LIB_FUNCTION("gZ9TpeFQ7Gk", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetPolicyOption);
    LIB_FUNCTION("2NeZnMEP3-0", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetPriorityOption);
    LIB_FUNCTION("i+quCZCL+D8", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetProxy);
    LIB_FUNCTION("mMcB2XIDoV4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetRecvBlockSize);
    LIB_FUNCTION("yigr4V0-HTM", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetRecvTimeOut);
    LIB_FUNCTION("h9wmFZX4i-4", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetRedirectCallback);
    LIB_FUNCTION("PTiFIUxCpJc", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetRequestContentLength);
    LIB_FUNCTION("vO4B-42ef-k", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetRequestStatusCallback);
    LIB_FUNCTION("K1d1LqZRQHQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetResolveRetry);
    LIB_FUNCTION("Tc-hAYDKtQc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetResolveTimeOut);
    LIB_FUNCTION("a4VsZ4oqn68", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetResponseHeaderMaxSize);
    LIB_FUNCTION("xegFfZKBVlw", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpSetSendTimeOut);
    LIB_FUNCTION("POJ0azHZX3w", "libSceHttp", 1, "libSceHttp", 1, 1,
                 sceHttpSetSocketCreationCallback);
    LIB_FUNCTION("7WcNoAI9Zcw", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsFreeCaList);
    LIB_FUNCTION("gcUjwU3fa0M", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsGetCaList);
    LIB_FUNCTION("JBN6N-EY+3M", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsGetSslError);
    LIB_FUNCTION("DK+GoXCNT04", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsLoadCert);
    LIB_FUNCTION("jUjp+yqMNdQ", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsSetMinSslVersion);
    LIB_FUNCTION("htyBOoWeS58", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsSetSslCallback);
    LIB_FUNCTION("U5ExQGyyx9s", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsSetSslVersion);
    LIB_FUNCTION("zXqcE0fizz0", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpsUnloadCert);
    LIB_FUNCTION("Ik-KpLTlf7Q", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpTerm);
    LIB_FUNCTION("V-noPEjSB8c", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpTryGetNonblock);
    LIB_FUNCTION("fmOs6MzCRqk", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpTrySetNonblock);
    LIB_FUNCTION("59tL1AQBb8U", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUnsetEpoll);
    LIB_FUNCTION("5LZA+KPISVA", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriBuild);
    LIB_FUNCTION("CR-l-yI-o7o", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriCopy);
    LIB_FUNCTION("YuOW3dDAKYc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriEscape);
    LIB_FUNCTION("3lgQ5Qk42ok", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriMerge);
    LIB_FUNCTION("IWalAn-guFs", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriParse);
    LIB_FUNCTION("mUU363n4yc0", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriSweepPath);
    LIB_FUNCTION("thTS+57zoLM", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpUriUnescape);
    LIB_FUNCTION("qISjDHrxONc", "libSceHttp", 1, "libSceHttp", 1, 1, sceHttpWaitRequest);
};

} // namespace Libraries::Http
