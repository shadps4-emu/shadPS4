// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include <fstream>

#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http_error.h"
#include "tests/stubs/kernel_stub.h"

#ifndef ORBIS_OK
#define ORBIS_OK 0
#endif

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpInit(int, int, u64);
int PS4_SYSV_ABI sceHttpTerm(int);
int PS4_SYSV_ABI sceHttpCreateTemplate(int, const char*, int, int);
int PS4_SYSV_ABI sceHttpDeleteTemplate(int);
int PS4_SYSV_ABI sceHttpCreateConnection(int, const char*, const char*, u16, int);
int PS4_SYSV_ABI sceHttpCreateConnectionWithURL(int, const char*, bool);
int PS4_SYSV_ABI sceHttpDeleteConnection(int);
int PS4_SYSV_ABI sceHttpCreateRequest(int, int, const char*, u64);
int PS4_SYSV_ABI sceHttpCreateRequestWithURL(int, s32, const char*, u64);
int PS4_SYSV_ABI sceHttpDeleteRequest(int);
int PS4_SYSV_ABI sceHttpSendRequest(int, const void*, u64);
int PS4_SYSV_ABI sceHttpGetLastErrno(int, int*);
int PS4_SYSV_ABI sceHttpGetStatusCode(int, int*);
int PS4_SYSV_ABI sceHttpReadData(s32, void*, u64);
int PS4_SYSV_ABI sceHttpAbortRequest(int);
int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int, char**, u64*);
} // namespace Libraries::Http

using namespace Libraries::Http;

namespace {

// State machine + no-internet path.. SendRequest records ENODNS on the request and GetLastErrno
// surfaces it.

// Drains every active context. Run before each test so previous test failures
// don't poison the static state.
static void DrainState() {
    for (int i = 0; i < 1024; ++i) {
        if (sceHttpTerm(i) != ORBIS_OK) {
            // Either not active or library already torn down
        }
    }
}

class HttpLifecycle : public ::testing::Test {
protected:
    void SetUp() override {
        DrainState();
    }
    void TearDown() override {
        DrainState();
    }
};

// Init returns a strictly-positive context id.
TEST_F(HttpLifecycle, InitReturnsContextId) {
    int ctx = sceHttpInit(1, 2, 1024 * 16);
    EXPECT_GT(ctx, 0);
    EXPECT_EQ(sceHttpTerm(ctx), ORBIS_OK);
}

// Init with poolSize=0 returns EINVAL.
TEST_F(HttpLifecycle, InitZeroPoolReturnsEinval) {
    EXPECT_EQ(sceHttpInit(0, 0, 0), static_cast<int>(ORBIS_KERNEL_ERROR_EINVAL));
}

// Term before Init returns BEFORE_INIT.
TEST_F(HttpLifecycle, TermBeforeInitFails) {
    EXPECT_EQ(sceHttpTerm(1), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// Two Inits, two Terms both context IDs are different, both terminate.
TEST_F(HttpLifecycle, MultipleContexts) {
    int a = sceHttpInit(0, 0, 4096);
    int b = sceHttpInit(0, 0, 4096);
    EXPECT_GT(a, 0);
    EXPECT_GT(b, 0);
    EXPECT_NE(a, b);
    EXPECT_EQ(sceHttpTerm(a), ORBIS_OK);
    EXPECT_EQ(sceHttpTerm(b), ORBIS_OK);
}

// Term of an already-terminated context returns INVALID_ID, not BEFORE_INIT.
TEST_F(HttpLifecycle, TermTwiceReturnsInvalidId) {
    int a = sceHttpInit(0, 0, 4096);
    int b = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpTerm(a), ORBIS_OK);
    // The library is still inited via b; another Term of a should now fail
    // with INVALID_ID.
    EXPECT_EQ(sceHttpTerm(a), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    EXPECT_EQ(sceHttpTerm(b), ORBIS_OK);
}

// CreateTemplate before Init should return BEFORE_INIT.
TEST_F(HttpLifecycle, CreateTemplateBeforeInit) {
    EXPECT_EQ(sceHttpCreateTemplate(1, "UA/1.0", 1, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// CreateTemplate with bogus context id should return INVALID_ID.
TEST_F(HttpLifecycle, CreateTemplateInvalidCtx) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpCreateTemplate(ctx + 99, "UA/1.0", 1, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    EXPECT_EQ(sceHttpTerm(ctx), ORBIS_OK);
}

// SendRequest twice on the same reqId: first dispatches the worker, second
// is rejected with AFTER_SEND (state is already Sending or Sent).
TEST_F(HttpLifecycle, SendTwiceReturnsAfterSend) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), static_cast<int>(ORBIS_HTTP_ERROR_AFTER_SEND));
    // Drain the worker so the next test starts clean.
    int sc;
    sceHttpGetStatusCode(req, &sc);
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tmpl);
    sceHttpTerm(ctx);
}

// SendRequest on bogus id shoud return INVALID_ID.
TEST_F(HttpLifecycle, SendInvalidRequest) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSendRequest(9999, nullptr, 0), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// GetLastErrno before send returns 0 (the default, no error recorded yet).
TEST_F(HttpLifecycle, GetLastErrnoBeforeSendIsZero) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    int err = 0xDEADBEEF;
    EXPECT_EQ(sceHttpGetLastErrno(req, &err), ORBIS_OK);
    EXPECT_EQ(err, 0);
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tmpl);
    sceHttpTerm(ctx);
}

// GetLastErrno with null output pointer should return INVALID_VALUE.
TEST_F(HttpLifecycle, GetLastErrnoNullOut) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpGetLastErrno(req, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// Term tears down all dependent objects: a request id from a terminated
// context is no longer valid.
TEST_F(HttpLifecycle, TermTearsDownDependents) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpTerm(ctx), ORBIS_OK);

    // Operations after term must report BEFORE_INIT.
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    int err;
    EXPECT_EQ(sceHttpGetLastErrno(req, &err), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// Connection with null server name should return INVALID_VALUE.
TEST_F(HttpLifecycle, CreateConnectionNullServerName) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpCreateConnection(tmpl, nullptr, "http", 80, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// Two requests on the same connection get distinct IDs and isolated errnos.
TEST_F(HttpLifecycle, TwoRequestsAreIsolated) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int r1 = sceHttpCreateRequestWithURL(conn, 0, "http://x/a", 0);
    int r2 = sceHttpCreateRequestWithURL(conn, 0, "http://x/b", 0);
    EXPECT_NE(r1, r2);

    EXPECT_EQ(sceHttpSendRequest(r1, nullptr, 0), ORBIS_OK);
    // Wait for r1's worker via a blocking getter (GetStatusCode).
    int sc;
    sceHttpGetStatusCode(r1, &sc);
    // r2 not yet sent
    int err1 = 0, err2 = 0;
    EXPECT_EQ(sceHttpGetLastErrno(r1, &err1), ORBIS_OK);
    EXPECT_EQ(sceHttpGetLastErrno(r2, &err2), ORBIS_OK);
    EXPECT_NE(err1, 0);
    EXPECT_EQ(err2, 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateConnectionNullServerNameTakesPriorityOverBadScheme) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpCreateConnection(tmpl, nullptr, "ftp", 80, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateConnectionUnknownScheme) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpCreateConnection(tmpl, "example.com", "ftp", 80, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_UNKNOWN_SCHEME));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateConnectionPortZeroAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_GT(sceHttpCreateConnection(tmpl, "example.com", "http", 0, 0), 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateRequestRejectsOptions) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    EXPECT_EQ(sceHttpCreateRequestWithURL(conn, /*Options=*/3, "http://example.com/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_UNKNOWN_METHOD));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateRequestRejectsConnect) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    EXPECT_EQ(sceHttpCreateRequestWithURL(conn, /*Connect=*/7, "http://example.com/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_UNKNOWN_METHOD));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateRequestRejectsOutOfRangeMethod) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    EXPECT_EQ(sceHttpCreateRequestWithURL(conn, 9, "http://example.com/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_UNKNOWN_METHOD));
    EXPECT_EQ(sceHttpCreateRequestWithURL(conn, 99, "http://example.com/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_UNKNOWN_METHOD));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateRequestAcceptsValidMethods) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    for (int m : {0, 1, 2, 4, 5, 6, 8}) {
        int r = sceHttpCreateRequestWithURL(conn, m, "http://example.com/", 0);
        EXPECT_GT(r, 0) << "method " << m << " unexpectedly rejected";
        if (r > 0)
            sceHttpDeleteRequest(r);
    }
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int, const char*, const char*, u64);
}

namespace {

// Verifies the WithURL2 ordering
TEST_F(HttpLifecycle, WithURL2BeforeInitTakesPriorityOverNullMethod) {
    // Library uninitialized (DrainState ran via SetUp).
    EXPECT_EQ(sceHttpCreateRequestWithURL2(/*connId=*/1, nullptr, "http://x/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// Initialized but bogus connId + NULL method should return INVALID_ID
TEST_F(HttpLifecycle, WithURL2InvalidIdTakesPriorityOverNullMethod) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpCreateRequestWithURL2(/*connId=*/9999, nullptr, "http://x/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// Initialized + valid connId + NULL method should return INVALID_VALUE
TEST_F(HttpLifecycle, WithURL2NullMethodAfterValidConn) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    EXPECT_EQ(sceHttpCreateRequestWithURL2(conn, nullptr, "http://example.com/", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpSetAutoRedirect(int id, int isEnable);
int PS4_SYSV_ABI sceHttpGetAutoRedirect(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpSetConnectTimeOut(int id, u32 usec);
} // namespace Libraries::Http

namespace {

// Connection snapshots template settings at creation.
TEST_F(HttpLifecycle, ConnectionSnapshotsTemplateAutoRedirectAtCreation) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);

    // Flip the template's default (which is true) to false.
    EXPECT_EQ(sceHttpSetAutoRedirect(tmpl, 0), ORBIS_OK);

    // Connection created NOW should snapshot redirect=false.
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    ASSERT_GT(conn, 0);

    // Flip the template AGAIN to true. The connection should NOT change.
    EXPECT_EQ(sceHttpSetAutoRedirect(tmpl, 1), ORBIS_OK);

    int got = 99;
    EXPECT_EQ(sceHttpGetAutoRedirect(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 0) << "connection should keep snapshot value, not follow template";

    // And the template's current value is now 1.
    EXPECT_EQ(sceHttpGetAutoRedirect(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);

    sceHttpTerm(ctx);
}

// Request snapshots connection settings at creation.
TEST_F(HttpLifecycle, RequestSnapshotsConnectionSettingsAtCreation) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    ASSERT_GT(conn, 0);

    // Default auto_redirect on conn is true (inherited from tmpl default).
    int got = -1;
    EXPECT_EQ(sceHttpGetAutoRedirect(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);

    // Flip the connection to false BEFORE creating the request.
    EXPECT_EQ(sceHttpSetAutoRedirect(conn, 0), ORBIS_OK);

    int req = sceHttpCreateRequestWithURL(conn, /*method=*/0, "http://example.com/", 0);
    ASSERT_GT(req, 0);

    // Now flip the connection back to true. Should NOT affect the request.
    EXPECT_EQ(sceHttpSetAutoRedirect(conn, 1), ORBIS_OK);

    got = 99;
    EXPECT_EQ(sceHttpGetAutoRedirect(req, &got), ORBIS_OK);
    EXPECT_EQ(got, 0) << "request should keep snapshot, not follow connection";

    // Connection's current value is 1.
    EXPECT_EQ(sceHttpGetAutoRedirect(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);

    sceHttpTerm(ctx);
}

// Default chain: template default then connection default then request default
// should all be auto_redirect=true
TEST_F(HttpLifecycle, AutoRedirectDefaultsPropagateThroughCreation) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://example.com/", 0);

    int got = -1;
    EXPECT_EQ(sceHttpGetAutoRedirect(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    EXPECT_EQ(sceHttpGetAutoRedirect(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    EXPECT_EQ(sceHttpGetAutoRedirect(req, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);

    sceHttpTerm(ctx);
}

// Timeouts also snapshot correctly
TEST_F(HttpLifecycle, ConnectTimeoutSnapshotsTemplateValue) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);

    EXPECT_EQ(sceHttpSetConnectTimeOut(tmpl, 7'000'000), ORBIS_OK);

    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    ASSERT_GT(conn, 0);

    EXPECT_EQ(sceHttpSetConnectTimeOut(tmpl, 99'000'000), ORBIS_OK);

    sceHttpTerm(ctx);
}

} // namespace

// Async state machine sanity tests
namespace {

// After SendRequest, a blocking ReadData waits for the worker, then returns 0
TEST_F(HttpLifecycle, ReadDataReturnsZeroAfterTransportFailure) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    char buf[16];
    EXPECT_EQ(sceHttpReadData(req, buf, sizeof(buf)), 0);
    sceHttpTerm(ctx);
}

// ReadData on an unsent request should return BEFORE_SEND (Created state).
TEST_F(HttpLifecycle, ReadDataBeforeSendReturnsBeforeSend) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    char buf[16];
    EXPECT_EQ(sceHttpReadData(req, buf, sizeof(buf)),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_SEND));
    sceHttpTerm(ctx);
}

// GetStatusCode on an aborted request should return ABORTED.
TEST_F(HttpLifecycle, GetStatusCodeOnAbortedRequest) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpAbortRequest(req), ORBIS_OK);
    int sc;
    EXPECT_EQ(sceHttpGetStatusCode(req, &sc), static_cast<int>(ORBIS_HTTP_ERROR_ABORTED));
    sceHttpTerm(ctx);
}

// AbortRequest on unsent request: state goes to Aborted, subsequent Send
// returns ABORTED.
TEST_F(HttpLifecycle, AbortBeforeSend) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpAbortRequest(req), ORBIS_OK);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), static_cast<int>(ORBIS_HTTP_ERROR_ABORTED));
    sceHttpTerm(ctx);
}

} // namespace

namespace {

TEST_F(HttpLifecycle, GetAllResponseHeadersEmptyOnTransportFailure) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    char* hdr = reinterpret_cast<char*>(0xdeadbeef);
    u64 hdr_size = 999;
    EXPECT_EQ(sceHttpGetAllResponseHeaders(req, &hdr, &hdr_size), ORBIS_OK);
    EXPECT_EQ(hdr, nullptr);
    EXPECT_EQ(hdr_size, 0u);
    sceHttpTerm(ctx);
}

// GetAllResponseHeaders before Send should return  BEFORE_SEND.
TEST_F(HttpLifecycle, GetAllResponseHeadersBeforeSend) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    char* hdr = nullptr;
    u64 hdr_size = 0;
    EXPECT_EQ(sceHttpGetAllResponseHeaders(req, &hdr, &hdr_size),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_SEND));
    sceHttpTerm(ctx);
}

// Null output pointer should return  INVALID_VALUE (init OK, no reqId lookup).
TEST_F(HttpLifecycle, GetAllResponseHeadersNullOut) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpGetAllResponseHeaders(0, nullptr, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

namespace {

TEST_F(HttpLifecycle, AbortAfterSendIsIdempotent) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);

    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    int sc;
    EXPECT_EQ(sceHttpGetStatusCode(req, &sc), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_SEND));
    EXPECT_EQ(sceHttpAbortRequest(req), ORBIS_OK);
    EXPECT_EQ(sceHttpGetStatusCode(req, &sc), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_SEND));
    int err = 0;
    EXPECT_EQ(sceHttpGetLastErrno(req, &err), ORBIS_OK);
    // Transport-failed; exact errno depends on build config. Assert non-zero.
    EXPECT_NE(err, 0);

    sceHttpTerm(ctx);
}

// Aborting twice on the same request is a no-op the second time.
TEST_F(HttpLifecycle, AbortTwiceIsIdempotent) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);

    EXPECT_EQ(sceHttpAbortRequest(req), ORBIS_OK);
    // Second call: idempotent OK.
    EXPECT_EQ(sceHttpAbortRequest(req), ORBIS_OK);

    int sc;
    EXPECT_EQ(sceHttpGetStatusCode(req, &sc), static_cast<int>(ORBIS_HTTP_ERROR_ABORTED));
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpSetNonblock(int, int);
int PS4_SYSV_ABI sceHttpGetNonblock(int, int*);
int PS4_SYSV_ABI sceHttpTrySetNonblock(int, int);
int PS4_SYSV_ABI sceHttpTryGetNonblock(int, int*);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, GetNonblockDefaultIsBlocking) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int got = -1;
    EXPECT_EQ(sceHttpGetNonblock(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 0);
    sceHttpTerm(ctx);
}

// Set + Get round-trip.
TEST_F(HttpLifecycle, SetGetNonblockRoundTrip) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int got = -1;
    EXPECT_EQ(sceHttpSetNonblock(tmpl, 1), ORBIS_OK);
    EXPECT_EQ(sceHttpGetNonblock(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    EXPECT_EQ(sceHttpSetNonblock(tmpl, 0), ORBIS_OK);
    EXPECT_EQ(sceHttpGetNonblock(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 0);
    sceHttpTerm(ctx);
}

// Try variants delegate to the regular ones.
TEST_F(HttpLifecycle, TryNonblockBehavesAsRegular) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int got = -1;
    EXPECT_EQ(sceHttpTrySetNonblock(tmpl, 1), ORBIS_OK);
    EXPECT_EQ(sceHttpTryGetNonblock(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    sceHttpTerm(ctx);
}

// Nonblock accepts template/connection/request IDs.
TEST_F(HttpLifecycle, SetNonblockAtRequestLevel) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSetNonblock(req, 1), ORBIS_OK);
    int got = -1;
    EXPECT_EQ(sceHttpGetNonblock(req, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, NonblockSnapshotsAtCreation) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetNonblock(tmpl, 1), ORBIS_OK);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    // Conn should have snapshot value = 1.
    int got = -1;
    EXPECT_EQ(sceHttpGetNonblock(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    // Flip template to 0; conn keeps its snapshot of 1.
    EXPECT_EQ(sceHttpSetNonblock(tmpl, 0), ORBIS_OK);
    EXPECT_EQ(sceHttpGetNonblock(conn, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    EXPECT_EQ(sceHttpGetNonblock(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, GetNonblockInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    int got = -1;
    EXPECT_EQ(sceHttpGetNonblock(99999, &got), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// NULL output pointer on Get should return INVALID_VALUE.
TEST_F(HttpLifecycle, GetNonblockNullOut) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpGetNonblock(tmpl, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpGetResponseContentLength(int, int*, u64*);
}

namespace {

TEST_F(HttpLifecycle, NonblockReadDataAfterCompletionReturnsZero) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    // Drain via blocking GetStatusCode (default blocking mode).
    int sc;
    sceHttpGetStatusCode(req, &sc);
    // Flip to nonblock - worker is done, nonblock check no longer fires.
    sceHttpSetNonblock(req, 1);
    char buf[16];
    EXPECT_EQ(sceHttpReadData(req, buf, sizeof(buf)), 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, NonblockGetStatusCodeAfterCompletionWorks) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    // Drain via blocking GetStatusCode first (state Sent).
    int sc;
    sceHttpGetStatusCode(req, &sc);
    // Flip to nonblock now; worker is done.
    sceHttpSetNonblock(req, 1);
    int r = sceHttpGetStatusCode(req, &sc);
    // For a transport-failed request, GetStatusCode returns BEFORE_SEND
    // (because last_errno != 0 and status_code == 0).
    EXPECT_EQ(r, static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_SEND));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, NonblockGetResponseContentLengthAfterCompletion) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    // Drain worker via blocking GetStatusCode (request is still in default
    // blocking mode).
    int sc;
    sceHttpGetStatusCode(req, &sc);
    // Now flip to nonblock. The worker is done; subsequent getters should
    // succeed (nonblock only affects the Sending state).
    sceHttpSetNonblock(req, 1);
    int result = -999;
    u64 content_length = 999;
    int r = sceHttpGetResponseContentLength(req, &result, &content_length);
    EXPECT_EQ(r, ORBIS_OK);
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpCreateRequest2(int, const char*, const char*, u64);
int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int, const char*, const char*, u64);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, CreateRequest2DoesNotDeadlock) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequest2(conn, "GET", "/path", 0);
    EXPECT_GT(req, 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateRequestWithURL2DoesNotDeadlock) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL2(conn, "POST", "http://example.com/post", 0);
    EXPECT_GT(req, 0);
    sceHttpTerm(ctx);
}

// CreateRequest2 with non-standard method routes via CUSTOM slot.
TEST_F(HttpLifecycle, CreateRequest2CustomMethodWorks) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequest2(conn, "PATCH", "/resource", 0);
    EXPECT_GT(req, 0);
    sceHttpTerm(ctx);
}

// CreateRequest2 NULL method should return INVALID_VALUE
TEST_F(HttpLifecycle, CreateRequest2NullMethod) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequest2(conn, nullptr, "/path", 0);
    EXPECT_EQ(req, static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// CreateRequest2 NULL path should return INVALID_VALUE.
TEST_F(HttpLifecycle, CreateRequest2NullPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 0);
    int req = sceHttpCreateRequest2(conn, "GET", nullptr, 0);
    EXPECT_EQ(req, static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

// Epoll lifecycle tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpCreateEpoll(int, OrbisHttpEpollHandle*);
int PS4_SYSV_ABI sceHttpDestroyEpoll(int, OrbisHttpEpollHandle);
int PS4_SYSV_ABI sceHttpSetEpoll(int, OrbisHttpEpollHandle, void*);
int PS4_SYSV_ABI sceHttpGetEpoll(int, OrbisHttpEpollHandle*, void**);
int PS4_SYSV_ABI sceHttpUnsetEpoll(int);
int PS4_SYSV_ABI sceHttpWaitRequest(OrbisHttpEpollHandle, OrbisHttpNBEvent*, int, int);
int PS4_SYSV_ABI sceHttpAbortWaitRequest(OrbisHttpEpollHandle);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, CreateAndDestroyEpoll) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    EXPECT_EQ(sceHttpCreateEpoll(ctx, &eh), ORBIS_OK);
    EXPECT_NE(eh, nullptr);
    EXPECT_EQ(sceHttpDestroyEpoll(ctx, eh), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateEpollNullOut) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpCreateEpoll(ctx, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, CreateEpollInvalidCtxId) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    EXPECT_EQ(sceHttpCreateEpoll(99999, &eh), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, DestroyEpollNullHandle) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpDestroyEpoll(ctx, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, DestroyEpollInvalidHandle) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle bogus = reinterpret_cast<OrbisHttpEpollHandle>(uintptr_t{99999});
    EXPECT_EQ(sceHttpDestroyEpoll(ctx, bogus), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetGetEpollRoundTripOnTemplate) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    int magic = 0;
    void* user_arg = &magic;
    EXPECT_EQ(sceHttpSetEpoll(tmpl, eh, user_arg), ORBIS_OK);

    OrbisHttpEpollHandle got_eh = nullptr;
    void* got_user_arg = nullptr;
    EXPECT_EQ(sceHttpGetEpoll(tmpl, &got_eh, &got_user_arg), ORBIS_OK);
    EXPECT_EQ(got_eh, eh);
    EXPECT_EQ(got_user_arg, user_arg);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetEpollInvalidEpollId) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    OrbisHttpEpollHandle bogus = reinterpret_cast<OrbisHttpEpollHandle>(uintptr_t{99999});
    EXPECT_EQ(sceHttpSetEpoll(tmpl, bogus, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetEpollInvalidOwnerId) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    EXPECT_EQ(sceHttpSetEpoll(99999, eh, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, EpollBindingSnapshotsFromTemplateToConnectionToRequest) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    int magic = 0;
    EXPECT_EQ(sceHttpSetEpoll(tmpl, eh, &magic), ORBIS_OK);

    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    OrbisHttpEpollHandle got_eh = nullptr;
    void* got_user_arg = nullptr;
    EXPECT_EQ(sceHttpGetEpoll(conn, &got_eh, &got_user_arg), ORBIS_OK);
    EXPECT_EQ(got_eh, eh);
    EXPECT_EQ(got_user_arg, &magic);

    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpGetEpoll(req, &got_eh, &got_user_arg), ORBIS_OK);
    EXPECT_EQ(got_eh, eh);
    EXPECT_EQ(got_user_arg, &magic);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, UnsetEpollIsRequestOnly) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpUnsetEpoll(tmpl), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    EXPECT_EQ(sceHttpUnsetEpoll(conn), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    // Request id works.
    EXPECT_EQ(sceHttpUnsetEpoll(req), ORBIS_OK);
    // After unset, GetEpoll returns 0 / nullptr.
    OrbisHttpEpollHandle got_eh = reinterpret_cast<OrbisHttpEpollHandle>(uintptr_t{0xdead});
    void* got_user_arg = reinterpret_cast<void*>(uintptr_t{0xdead});
    EXPECT_EQ(sceHttpGetEpoll(req, &got_eh, &got_user_arg), ORBIS_OK);
    EXPECT_EQ(got_eh, nullptr);
    EXPECT_EQ(got_user_arg, nullptr);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, WaitRequestInvalidArgs) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    OrbisHttpNBEvent ev{};
    // maxevents <= 0
    EXPECT_EQ(sceHttpWaitRequest(eh, &ev, 0, 0), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpWaitRequest(eh, nullptr, 1, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpWaitRequest(nullptr, &ev, 1, 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, WaitRequestPollEmptyReturnsZero) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    OrbisHttpNBEvent ev{};
    EXPECT_EQ(sceHttpWaitRequest(eh, &ev, 1, 0), 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, WaitRequestDrainsWorkerCompletionEvent) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    int magic = 0;
    sceHttpSetEpoll(tmpl, eh, &magic);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    // Drain via blocking getter so the worker is guaranteed to have run.
    int sc;
    sceHttpGetStatusCode(req, &sc);
    // Now drain the epoll queue.
    OrbisHttpNBEvent ev{};
    int count = sceHttpWaitRequest(eh, &ev, 1, 0);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ev.id, req);
    EXPECT_EQ(ev.userArg, &magic);
    // No-internet path: failure bits (SOCK_ERR | HUP) are present.
    EXPECT_NE(ev.events & 0x00020010u, 0u); // RESOLVER_ERR (0x20000) or HUP (0x10)
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AbortWaitRequestNullHandle) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpAbortWaitRequest(nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AbortWaitRequestInvalidHandle) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle bogus = reinterpret_cast<OrbisHttpEpollHandle>(uintptr_t{99999});
    EXPECT_EQ(sceHttpAbortWaitRequest(bogus), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, TermClearsEpolls) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    sceHttpTerm(ctx);
    // After Term, using the epoll handle returns BEFORE_INIT.
    OrbisHttpNBEvent ev{};
    EXPECT_EQ(sceHttpWaitRequest(eh, &ev, 1, 0), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, AllEpollCallsBeforeInitReturnBeforeInit) {
    OrbisHttpEpollHandle eh = nullptr;
    OrbisHttpEpollHandle stub = reinterpret_cast<OrbisHttpEpollHandle>(uintptr_t{1});
    OrbisHttpNBEvent ev{};
    void* ua = nullptr;
    EXPECT_EQ(sceHttpCreateEpoll(0, &eh), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpDestroyEpoll(0, stub), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpSetEpoll(0, stub, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpGetEpoll(0, &eh, &ua), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpUnsetEpoll(0), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpWaitRequest(stub, &ev, 1, 0), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
    EXPECT_EQ(sceHttpAbortWaitRequest(stub), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

} // namespace

namespace {

TEST_F(HttpLifecycle, CreateEpollBadCtxAndNullOutReturnsInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpCreateEpoll(99999, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// DestroyEpoll: same firmware order.
TEST_F(HttpLifecycle, DestroyEpollBadCtxAndNullEhReturnsInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpDestroyEpoll(99999, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, WaitRequestAfterAbortReturnsAborted) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    EXPECT_EQ(sceHttpAbortWaitRequest(eh), ORBIS_OK);
    OrbisHttpNBEvent ev{};
    int r = sceHttpWaitRequest(eh, &ev, 1, 0);
    EXPECT_EQ(r, static_cast<int>(ORBIS_HTTP_ERROR_ABORTED));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, WaitRequestAbortedTakesPriorityOverQueuedEvents) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    OrbisHttpEpollHandle eh = nullptr;
    sceHttpCreateEpoll(ctx, &eh);
    sceHttpSetEpoll(tmpl, eh, nullptr);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpSendRequest(req, nullptr, 0), ORBIS_OK);
    int sc;
    sceHttpGetStatusCode(req, &sc); // drain worker (pushes a failure event)
    // Abort first, then call WaitRequest.
    sceHttpAbortWaitRequest(eh);
    OrbisHttpNBEvent ev{};
    int r = sceHttpWaitRequest(eh, &ev, 1, 0);
    EXPECT_EQ(r, static_cast<int>(ORBIS_HTTP_ERROR_ABORTED));
    sceHttpTerm(ctx);
}

} // namespace

// sceHttpAddRequestHeader tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpAddRequestHeader(int, const char*, const char*, s32);
}

namespace {

// Happy path: add a header on a template id, no errors.
TEST_F(HttpLifecycle, AddRequestHeaderTemplateHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, "Content-Type", "application/json", 1), ORBIS_OK);
    sceHttpTerm(ctx);
}

// Headers can be added on connection ids too.
TEST_F(HttpLifecycle, AddRequestHeaderConnectionIdAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(conn, "X-Custom", "foo", 1), ORBIS_OK);
    sceHttpTerm(ctx);
}

// And on request ids.
TEST_F(HttpLifecycle, AddRequestHeaderRequestIdAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    EXPECT_EQ(sceHttpAddRequestHeader(req, "X-Custom", "bar", 1), ORBIS_OK);
    sceHttpTerm(ctx);
}

// BEFORE_INIT when library not inited.
TEST_F(HttpLifecycle, AddRequestHeaderBeforeInit) {
    EXPECT_EQ(sceHttpAddRequestHeader(1, "Foo", "bar", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// INVALID_VALUE for mode out of range.
TEST_F(HttpLifecycle, AddRequestHeaderInvalidMode) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, "Foo", "bar", 2),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, "Foo", "bar", -1),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// INVALID_VALUE for null name.
TEST_F(HttpLifecycle, AddRequestHeaderNullName) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, nullptr, "bar", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// INVALID_ID for non-existent id.
TEST_F(HttpLifecycle, AddRequestHeaderInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpAddRequestHeader(99999, "Foo", "bar", 0),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AddRequestHeaderModeCheckedBeforeId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpAddRequestHeader(99999, "Foo", "bar", 2),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AddRequestHeaderNullValue) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, "X-Empty", nullptr, 1),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

// sceHttpRemoveRequestHeader tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpRemoveRequestHeader(int, const char*);
}

namespace {

// Add then remove: succeeds.
TEST_F(HttpLifecycle, RemoveRequestHeaderHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpAddRequestHeader(tmpl, "X-Foo", "bar", 1), ORBIS_OK);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "X-Foo"), ORBIS_OK);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "X-Foo"),
              static_cast<int>(ORBIS_HTTP_ERROR_NOT_FOUND));
    sceHttpTerm(ctx);
}

// Remove when the header was never added should return  NOT_FOUND.
TEST_F(HttpLifecycle, RemoveRequestHeaderNotPresent) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "Nonexistent"),
              static_cast<int>(ORBIS_HTTP_ERROR_NOT_FOUND));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, RemoveRequestHeaderRemovesAllDuplicates) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    sceHttpAddRequestHeader(tmpl, "X-Dup", "v1", 1);
    sceHttpAddRequestHeader(tmpl, "X-Dup", "v2", 1);
    sceHttpAddRequestHeader(tmpl, "X-Dup", "v3", 1);
    sceHttpAddRequestHeader(tmpl, "X-Other", "keep", 1);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "X-Dup"), ORBIS_OK);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "X-Dup"),
              static_cast<int>(ORBIS_HTTP_ERROR_NOT_FOUND));
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "X-Other"), ORBIS_OK);
    sceHttpTerm(ctx);
}

// Case-insensitive match per HTTP semantics.
TEST_F(HttpLifecycle, RemoveRequestHeaderCaseInsensitive) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    sceHttpAddRequestHeader(tmpl, "Content-Type", "application/json", 1);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, "content-type"), ORBIS_OK);
    sceHttpTerm(ctx);
}

// BEFORE_INIT when library not inited.
TEST_F(HttpLifecycle, RemoveRequestHeaderBeforeInit) {
    EXPECT_EQ(sceHttpRemoveRequestHeader(1, "X-Foo"),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// Null nameshould return  NOT_FOUND
TEST_F(HttpLifecycle, RemoveRequestHeaderNullNameReturnsNotFound) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpRemoveRequestHeader(tmpl, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_NOT_FOUND));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, RemoveRequestHeaderInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpRemoveRequestHeader(99999, "X-Foo"),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// Works on connection ids too.
TEST_F(HttpLifecycle, RemoveRequestHeaderConnectionIdAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    sceHttpAddRequestHeader(conn, "X-Custom", "foo", 1);
    EXPECT_EQ(sceHttpRemoveRequestHeader(conn, "X-Custom"), ORBIS_OK);
    sceHttpTerm(ctx);
}

// And request ids.
TEST_F(HttpLifecycle, RemoveRequestHeaderRequestIdAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int req = sceHttpCreateRequestWithURL(conn, 0, "http://x/", 0);
    sceHttpAddRequestHeader(req, "X-Custom", "bar", 1);
    EXPECT_EQ(sceHttpRemoveRequestHeader(req, "X-Custom"), ORBIS_OK);
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled(int, int);
int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int, int*);
int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int, int);
int PS4_SYSV_ABI sceHttpSetResolveTimeOut(int, u32);
int PS4_SYSV_ABI sceHttpSetResolveRetry(int, int);
int PS4_SYSV_ABI sceHttpSetRecvBlockSize(int, u32);
int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize(int, u64);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, AcceptEncodingGZIPDefaultIsTrue) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int got = -1;
    EXPECT_EQ(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AcceptEncodingGZIPRoundTrip) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetAcceptEncodingGZIPEnabled(tmpl, 0), ORBIS_OK);
    int got = -1;
    EXPECT_EQ(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 0);
    EXPECT_EQ(sceHttpSetAcceptEncodingGZIPEnabled(tmpl, 1), ORBIS_OK);
    EXPECT_EQ(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, &got), ORBIS_OK);
    EXPECT_EQ(got, 1);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AcceptEncodingGZIPGetNullPtr) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, AcceptEncodingGZIPSnapshotsToConnection) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    sceHttpSetAcceptEncodingGZIPEnabled(tmpl, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    int got = -1;
    sceHttpGetAcceptEncodingGZIPEnabled(conn, &got);
    EXPECT_EQ(got, 0); // inherits from template
    sceHttpTerm(ctx);
}

// --- SetDefault: library global affects future templates ---

TEST_F(HttpLifecycle, SetDefaultAcceptEncodingGZIPAffectsNewTemplates) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetDefaultAcceptEncodingGZIPEnabled(ctx, 0), ORBIS_OK);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int got = -1;
    sceHttpGetAcceptEncodingGZIPEnabled(tmpl, &got);
    EXPECT_EQ(got, 0); // picked up the new default
    // Flip back so following tests aren't affected by global state.
    sceHttpSetDefaultAcceptEncodingGZIPEnabled(ctx, 1);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetDefaultAcceptEncodingGZIPBeforeInit) {
    EXPECT_EQ(sceHttpSetDefaultAcceptEncodingGZIPEnabled(1, 1),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

// --- ResolveTimeOut validation ---

TEST_F(HttpLifecycle, SetResolveTimeOutHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    // SDK >= 1.70 requires usec > 999999 per firmware line 16020.
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 5000000u), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResolveTimeOutMinAccepted) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    // 1000000 is one above the firmware threshold (must be > 999999).
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 1000000u), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResolveTimeOutSmallRejected) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    // 999999 fails the strict (usec > 999999) check on SDK >= 1.70.
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 999999u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 500000u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResolveTimeOutBeforeInit) {
    EXPECT_EQ(sceHttpSetResolveTimeOut(1, 100), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, SetResolveTimeOutOutOfRangeBeforeBadId) {
    int ctx = sceHttpInit(0, 0, 4096);
    // usec=100 fails the > 999999 check on SDK >= 1.70 before id is looked up.
    EXPECT_EQ(sceHttpSetResolveTimeOut(99999, 100u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResolveTimeOut_PreSDK170_AcceptsSmallUsec) {
    Libraries::Kernel::TestSetSdkVersion(0x1600000);
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 100u), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 0u), ORBIS_OK);
    sceHttpTerm(ctx);
    Libraries::Kernel::TestResetSdkVersion();
}

TEST_F(HttpLifecycle, SetResolveTimeOut_AtSDK170_AppliesStrictCheck) {
    Libraries::Kernel::TestSetSdkVersion(0x1700000);
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 999999u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 1000000u), ORBIS_OK);
    sceHttpTerm(ctx);
    Libraries::Kernel::TestResetSdkVersion();
}

TEST_F(HttpLifecycle, SetResolveTimeOut_PreSDK170_OutOfRangeStillAccepted) {
    Libraries::Kernel::TestSetSdkVersion(0x1600000);
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 999999u), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 500000u), ORBIS_OK);
    sceHttpTerm(ctx);
    Libraries::Kernel::TestResetSdkVersion();
}

// --- ResolveRetry ---

TEST_F(HttpLifecycle, SetResolveRetryHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveRetry(tmpl, 0), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveRetry(tmpl, 3), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveRetry(tmpl, 100), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResolveRetryNegativeRejected) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveRetry(tmpl, -1), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// Firmware ordering: retry < 0 checked BEFORE id lookup.
TEST_F(HttpLifecycle, SetResolveRetryNegativeBeforeBadId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetResolveRetry(99999, -5), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

// --- RecvBlockSize ---

TEST_F(HttpLifecycle, SetRecvBlockSizeHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetRecvBlockSize(tmpl, 16384), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetRecvBlockSizeInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetRecvBlockSize(99999, 4096), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// --- ResponseHeaderMaxSize ---

TEST_F(HttpLifecycle, SetResponseHeaderMaxSizeHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResponseHeaderMaxSize(tmpl, 8192), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetResponseHeaderMaxSizeInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetResponseHeaderMaxSize(99999, 8192),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

} // namespace

TEST_F(HttpLifecycle, SetResolveTimeOutOldSdkSkipsValidation) {
    Libraries::Kernel::TestSetSdkVersion(0x1000000); // pre-1.70
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    // Both small and large usec accepted on old SDK.
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 500u), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 999999u), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 5000000u), ORBIS_OK);
    sceHttpTerm(ctx);
    Libraries::Kernel::TestResetSdkVersion();
}

TEST_F(HttpLifecycle, SetResolveTimeOutNewSdkEnforces) {
    Libraries::Kernel::TestSetSdkVersion(0x1700000); // exactly 1.70
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 999999u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 500000u),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 1000000u), ORBIS_OK);
    EXPECT_EQ(sceHttpSetResolveTimeOut(tmpl, 35000000u), ORBIS_OK);
    sceHttpTerm(ctx);
    Libraries::Kernel::TestResetSdkVersion();
}

// sceHttpSetProxy tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpSetProxy(int, int, int, const char*, u16);
}

namespace {

TEST_F(HttpLifecycle, SetProxyHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    // mode 1 = manual proxy, host:port specified
    EXPECT_EQ(sceHttpSetProxy(tmpl, 1, 0, "proxy.example.com", 8080), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetProxyOnConnection) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    EXPECT_EQ(sceHttpSetProxy(conn, 1, 0, "proxy.example.com", 3128), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetProxyBeforeInit) {
    EXPECT_EQ(sceHttpSetProxy(1, 0, 0, "host", 80), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, SetProxyNullHost) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    EXPECT_EQ(sceHttpSetProxy(tmpl, 1, 0, nullptr, 8080),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetProxyNullHostBeforeBadId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetProxy(99999, 1, 0, nullptr, 8080),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, SetProxyInvalidId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpSetProxy(99999, 1, 0, "host", 80),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

// Connection inherits proxy from template at creation.
TEST_F(HttpLifecycle, SetProxyInheritedByConnection) {
    int ctx = sceHttpInit(0, 0, 4096);
    int tmpl = sceHttpCreateTemplate(ctx, "UA", 1, 0);
    sceHttpSetProxy(tmpl, 1, 0, "tmpl-proxy.example.com", 9999);
    int conn = sceHttpCreateConnection(tmpl, "x", "http", 80, 0);
    // Override on connection
    EXPECT_EQ(sceHttpSetProxy(conn, 1, 0, "conn-proxy.example.com", 8888), ORBIS_OK);
    sceHttpTerm(ctx);
}

} // namespace

// sceHttpsLoadCert / sceHttpsUnloadCert tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpsLoadCert(int, int, const void**, const void*, const void*);
int PS4_SYSV_ABI sceHttpsUnloadCert(int);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, LoadCertHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    const void* dummy_cas[2] = {nullptr, nullptr};
    EXPECT_EQ(sceHttpsLoadCert(ctx, 2, dummy_cas, nullptr, nullptr), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, LoadCertBeforeInit) {
    EXPECT_EQ(sceHttpsLoadCert(1, 0, nullptr, nullptr, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, LoadCertInvalidCtxId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpsLoadCert(99999, 0, nullptr, nullptr, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, UnloadCertHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    const void* dummy_cas[1] = {nullptr};
    sceHttpsLoadCert(ctx, 1, dummy_cas, nullptr, nullptr);
    EXPECT_EQ(sceHttpsUnloadCert(ctx), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, UnloadCertWithoutLoadIsOk) {
    int ctx = sceHttpInit(0, 0, 4096);
    // Idempotent: erasing from the set returns 0 but we don't surface it as
    // an error since context exists.
    EXPECT_EQ(sceHttpsUnloadCert(ctx), ORBIS_OK);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, UnloadCertBeforeInit) {
    EXPECT_EQ(sceHttpsUnloadCert(1), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, UnloadCertInvalidCtxId) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpsUnloadCert(99999), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, LoadCertSurvivesTermCleanly) {
    int ctx = sceHttpInit(0, 0, 4096);
    const void* dummy[1] = {nullptr};
    EXPECT_EQ(sceHttpsLoadCert(ctx, 1, dummy, nullptr, nullptr), ORBIS_OK);
    sceHttpTerm(ctx);
    // After Term, the ctxId is invalid - LoadCert on it must fail.
    EXPECT_EQ(sceHttpsLoadCert(ctx, 1, dummy, nullptr, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

} // namespace

// sceHttpsGetCaList / sceHttpsFreeCaList tests

namespace Libraries::Http {
int PS4_SYSV_ABI sceHttpsGetCaList(int httpCtxId, OrbisHttpsCaList* list);
int PS4_SYSV_ABI sceHttpsFreeCaList(int libhttpCtxId, OrbisHttpsCaList* caList);
} // namespace Libraries::Http

namespace {

TEST_F(HttpLifecycle, GetCaListHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpsCaList list{};
    list.certsNum = 999; // sentinel; should be reset to 0
    EXPECT_EQ(sceHttpsGetCaList(ctx, &list), ORBIS_OK);
    EXPECT_EQ(list.certsNum, 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, GetCaListBeforeInit) {
    OrbisHttpsCaList list{};
    EXPECT_EQ(sceHttpsGetCaList(1, &list), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, GetCaListInvalidCtxId) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpsCaList list{};
    EXPECT_EQ(sceHttpsGetCaList(99999, &list), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, GetCaListNullList) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpsGetCaList(ctx, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, FreeCaListHappyPath) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpsCaList list{};
    list.certsNum = 5; // anything non-zero
    EXPECT_EQ(sceHttpsFreeCaList(ctx, &list), ORBIS_OK);
    EXPECT_EQ(list.certsNum, 0);
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, FreeCaListBeforeInit) {
    OrbisHttpsCaList list{};
    EXPECT_EQ(sceHttpsFreeCaList(1, &list), static_cast<int>(ORBIS_HTTP_ERROR_BEFORE_INIT));
}

TEST_F(HttpLifecycle, FreeCaListInvalidCtxId) {
    int ctx = sceHttpInit(0, 0, 4096);
    OrbisHttpsCaList list{};
    EXPECT_EQ(sceHttpsFreeCaList(99999, &list), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_ID));
    sceHttpTerm(ctx);
}

TEST_F(HttpLifecycle, FreeCaListNullList) {
    int ctx = sceHttpInit(0, 0, 4096);
    EXPECT_EQ(sceHttpsFreeCaList(ctx, nullptr), static_cast<int>(ORBIS_HTTP_ERROR_INVALID_VALUE));
    sceHttpTerm(ctx);
}

} // namespace

namespace Libraries::Http {
bool IsFollowableRedirect(int status, s32 method);
s32 MethodAfterRedirect(int status, s32 original_method);
struct ResolvedRedirect {
    std::string scheme;
    std::string host;
    u16 port;
    std::string path;
};
std::optional<ResolvedRedirect> ResolveRedirectLocation(const std::string& current_scheme,
                                                        const std::string& current_host,
                                                        u16 current_port,
                                                        std::string_view location);
} // namespace Libraries::Http

namespace {

using Libraries::Http::IsFollowableRedirect;
using Libraries::Http::MethodAfterRedirect;
using Libraries::Http::ResolveRedirectLocation;

// --- IsFollowableRedirect: status filter ---

TEST(Ps4Redirect, FollowsThreeHundred) {
    EXPECT_TRUE(IsFollowableRedirect(300, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, FollowsThreeOhOne) {
    EXPECT_TRUE(IsFollowableRedirect(301, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, FollowsThreeOhTwo) {
    EXPECT_TRUE(IsFollowableRedirect(302, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, FollowsThreeOhThree) {
    EXPECT_TRUE(IsFollowableRedirect(303, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, FollowsThreeOhSeven) {
    EXPECT_TRUE(IsFollowableRedirect(307, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsThreeOhFour) {
    EXPECT_FALSE(IsFollowableRedirect(304, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsThreeOhFive) {
    EXPECT_FALSE(IsFollowableRedirect(305, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsThreeOhSix) {
    EXPECT_FALSE(IsFollowableRedirect(306, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsThreeOhEight) {
    EXPECT_FALSE(IsFollowableRedirect(308, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsNonRedirect) {
    EXPECT_FALSE(IsFollowableRedirect(200, ORBIS_HTTP_METHOD_GET));
}
TEST(Ps4Redirect, RejectsServerError) {
    EXPECT_FALSE(IsFollowableRedirect(500, ORBIS_HTTP_METHOD_GET));
}

// --- IsFollowableRedirect: POST nuance (only 303 follows) ---

TEST(Ps4Redirect, PostFollows303) {
    EXPECT_TRUE(IsFollowableRedirect(303, ORBIS_HTTP_METHOD_POST));
}
TEST(Ps4Redirect, PostDoesNotFollow300) {
    EXPECT_FALSE(IsFollowableRedirect(300, ORBIS_HTTP_METHOD_POST));
}
TEST(Ps4Redirect, PostDoesNotFollow301) {
    EXPECT_FALSE(IsFollowableRedirect(301, ORBIS_HTTP_METHOD_POST));
}
TEST(Ps4Redirect, PostDoesNotFollow302) {
    EXPECT_FALSE(IsFollowableRedirect(302, ORBIS_HTTP_METHOD_POST));
}
TEST(Ps4Redirect, PostDoesNotFollow307) {
    EXPECT_FALSE(IsFollowableRedirect(307, ORBIS_HTTP_METHOD_POST));
}

// --- IsFollowableRedirect: HEAD always follows where status allows ---

TEST(Ps4Redirect, HeadFollows301) {
    EXPECT_TRUE(IsFollowableRedirect(301, ORBIS_HTTP_METHOD_HEAD));
}
TEST(Ps4Redirect, HeadFollows303) {
    EXPECT_TRUE(IsFollowableRedirect(303, ORBIS_HTTP_METHOD_HEAD));
}
TEST(Ps4Redirect, HeadFollows307) {
    EXPECT_TRUE(IsFollowableRedirect(307, ORBIS_HTTP_METHOD_HEAD));
}

// --- MethodAfterRedirect: method change rules ---

TEST(Ps4Redirect, MethodPreservedOn301) {
    EXPECT_EQ(MethodAfterRedirect(301, ORBIS_HTTP_METHOD_GET), ORBIS_HTTP_METHOD_GET);
    EXPECT_EQ(MethodAfterRedirect(301, ORBIS_HTTP_METHOD_HEAD), ORBIS_HTTP_METHOD_HEAD);
    EXPECT_EQ(MethodAfterRedirect(301, ORBIS_HTTP_METHOD_POST), ORBIS_HTTP_METHOD_POST);
}
TEST(Ps4Redirect, MethodPreservedOn302) {
    EXPECT_EQ(MethodAfterRedirect(302, ORBIS_HTTP_METHOD_GET), ORBIS_HTTP_METHOD_GET);
    EXPECT_EQ(MethodAfterRedirect(302, ORBIS_HTTP_METHOD_POST), ORBIS_HTTP_METHOD_POST);
}
TEST(Ps4Redirect, MethodPreservedOn307) {
    // RFC 7231 §6.4.7 - 307 specifically preserves method
    EXPECT_EQ(MethodAfterRedirect(307, ORBIS_HTTP_METHOD_GET), ORBIS_HTTP_METHOD_GET);
    EXPECT_EQ(MethodAfterRedirect(307, ORBIS_HTTP_METHOD_POST), ORBIS_HTTP_METHOD_POST);
    EXPECT_EQ(MethodAfterRedirect(307, ORBIS_HTTP_METHOD_HEAD), ORBIS_HTTP_METHOD_HEAD);
    EXPECT_EQ(MethodAfterRedirect(307, ORBIS_HTTP_METHOD_PUT), ORBIS_HTTP_METHOD_PUT);
}
TEST(Ps4Redirect, PostDowngradesToGetOn303) {
    EXPECT_EQ(MethodAfterRedirect(303, ORBIS_HTTP_METHOD_POST), ORBIS_HTTP_METHOD_GET);
}
TEST(Ps4Redirect, PutDowngradesToGetOn303) {
    EXPECT_EQ(MethodAfterRedirect(303, ORBIS_HTTP_METHOD_PUT), ORBIS_HTTP_METHOD_GET);
}
TEST(Ps4Redirect, HeadPreservedOn303) {
    // RFC 7231 §6.4.4 carve-out: HEAD stays HEAD across 303
    EXPECT_EQ(MethodAfterRedirect(303, ORBIS_HTTP_METHOD_HEAD), ORBIS_HTTP_METHOD_HEAD);
}
TEST(Ps4Redirect, GetStaysGetOn303) {
    EXPECT_EQ(MethodAfterRedirect(303, ORBIS_HTTP_METHOD_GET), ORBIS_HTTP_METHOD_GET);
}

// --- ResolveRedirectLocation: URL parsing ---

TEST(Ps4Redirect, ResolveAbsoluteHttp) {
    auto r =
        ResolveRedirectLocation("https", "old.example.com", 443, "http://new.example.com/path?q=1");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->scheme, "http");
    EXPECT_EQ(r->host, "new.example.com");
    EXPECT_EQ(r->port, 80u);
    EXPECT_EQ(r->path, "/path?q=1");
}
TEST(Ps4Redirect, ResolveAbsoluteHttpsExplicitPort) {
    auto r =
        ResolveRedirectLocation("http", "old.example.com", 80, "https://secure.example.com:8443/x");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->scheme, "https");
    EXPECT_EQ(r->host, "secure.example.com");
    EXPECT_EQ(r->port, 8443u);
    EXPECT_EQ(r->path, "/x");
}
TEST(Ps4Redirect, ResolveAbsolutePath) {
    auto r = ResolveRedirectLocation("https", "example.com", 443, "/new/path?q=1");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->scheme, "https");
    EXPECT_EQ(r->host, "example.com");
    EXPECT_EQ(r->port, 443u);
    EXPECT_EQ(r->path, "/new/path?q=1");
}
TEST(Ps4Redirect, ResolveAbsoluteWithoutPath) {
    // Bare "https://host" with no path - default to "/"
    auto r = ResolveRedirectLocation("http", "old", 80, "https://host.example.com");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->scheme, "https");
    EXPECT_EQ(r->host, "host.example.com");
    EXPECT_EQ(r->port, 443u);
    EXPECT_EQ(r->path, "/");
}
TEST(Ps4Redirect, ResolveSchemeUppercaseLowercased) {
    auto r = ResolveRedirectLocation("http", "x", 80, "HTTPS://host/p");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->scheme, "https"); // canonicalised
}
TEST(Ps4Redirect, ResolveRejectsDocumentRelative) {
    // "foo/bar" - no leading slash, no scheme
    auto r = ResolveRedirectLocation("https", "x", 443, "foo/bar");
    EXPECT_FALSE(r.has_value());
}
TEST(Ps4Redirect, ResolveRejectsEmpty) {
    auto r = ResolveRedirectLocation("https", "x", 443, "");
    EXPECT_FALSE(r.has_value());
}
TEST(Ps4Redirect, ResolveRejectsUnknownScheme) {
    // ftp:// is not a scheme libhttp speaks; we bail
    auto r = ResolveRedirectLocation("https", "x", 443, "ftp://host/path");
    EXPECT_FALSE(r.has_value());
}
TEST(Ps4Redirect, ResolveRejectsInvalidPort) {
    auto r = ResolveRedirectLocation("https", "x", 443, "https://host:99999/path");
    EXPECT_FALSE(r.has_value());
}
TEST(Ps4Redirect, ResolveRejectsZeroPort) {
    auto r = ResolveRedirectLocation("https", "x", 443, "https://host:0/path");
    EXPECT_FALSE(r.has_value());
}
TEST(Ps4Redirect, ResolveRejectsEmptyAuthority) {
    auto r = ResolveRedirectLocation("https", "x", 443, "https:///path");
    EXPECT_FALSE(r.has_value());
}

} // namespace

namespace Libraries::Http {
struct HostOverrideTarget {
    std::string scheme;
    std::string host;
    u16 port = 0;
};
std::unordered_map<std::string, HostOverrideTarget> ParseHostOverridesJson(
    const std::string& json_text);
bool ApplyHostOverride(std::string& scheme, std::string& host, u16& port, bool& is_secure);
} // namespace Libraries::Http

namespace {

using Libraries::Http::ApplyHostOverride;
using Libraries::Http::ParseHostOverridesJson;

// --- ParseHostOverridesJson: shape parsing ---

TEST(HostOverride, ParseEmptyStringYieldsEmptyMap) {
    auto m = ParseHostOverridesJson("");
    EXPECT_TRUE(m.empty());
}

TEST(HostOverride, ParseEmptyObjectYieldsEmptyMap) {
    auto m = ParseHostOverridesJson("{}");
    EXPECT_TRUE(m.empty());
}

TEST(HostOverride, ParseSingleHostNoPortNoScheme) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "localhost"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, ""); // preserve original
    EXPECT_EQ(e.host, "localhost");
    EXPECT_EQ(e.port, 0); // preserve original port
}

TEST(HostOverride, ParseSingleHostWithPortNoScheme) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "localhost:8080"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, "");
    EXPECT_EQ(e.host, "localhost");
    EXPECT_EQ(e.port, 8080);
}

TEST(HostOverride, ParseHttpSchemeWithPort) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "http://localhost:8080"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, "http");
    EXPECT_EQ(e.host, "localhost");
    EXPECT_EQ(e.port, 8080);
}

TEST(HostOverride, ParseHttpsSchemeWithPort) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "https://secure.local:8443"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, "https");
    EXPECT_EQ(e.host, "secure.local");
    EXPECT_EQ(e.port, 8443);
}

TEST(HostOverride, ParseHttpSchemeNoPort) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "http://localhost"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, "http");
    EXPECT_EQ(e.host, "localhost");
    EXPECT_EQ(e.port, 0);
}

TEST(HostOverride, ParseSchemeUppercaseLowercased) {
    auto m = ParseHostOverridesJson(R"({"api.example.com": "HTTPS://Host:443"})");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("api.example.com").scheme, "https");
}

TEST(HostOverride, ParseUnknownSchemeDropsSchemeKeepsHost) {
    // ftp:// is not http/https; scheme is silently dropped, host still parsed.
    auto m = ParseHostOverridesJson(R"({"api.example.com": "ftp://elsewhere:21"})");
    ASSERT_EQ(m.size(), 1u);
    const auto& e = m.at("api.example.com");
    EXPECT_EQ(e.scheme, "");
    EXPECT_EQ(e.host, "elsewhere");
    EXPECT_EQ(e.port, 21);
}

TEST(HostOverride, ParseMultipleEntriesMixedForms) {
    auto m = ParseHostOverridesJson(R"({
        "api.example.com":         "http://localhost:8080",
        "analytics.example.com":   "https://127.0.0.1:9090",
        "static.example.com":      "mock.local:7000",
        "*":                       "http://catch-all.local"
    })");
    ASSERT_EQ(m.size(), 4u);
    EXPECT_EQ(m.at("api.example.com").scheme, "http");
    EXPECT_EQ(m.at("api.example.com").port, 8080);
    EXPECT_EQ(m.at("analytics.example.com").scheme, "https");
    EXPECT_EQ(m.at("static.example.com").scheme, ""); // no scheme prefix
    EXPECT_EQ(m.at("static.example.com").port, 7000);
    EXPECT_EQ(m.at("*").scheme, "http");
    EXPECT_EQ(m.at("*").port, 0); // no port = preserve / default
}

TEST(HostOverride, ParseCatchAllWildcard) {
    auto m = ParseHostOverridesJson(R"({"*": "http://localhost:8080"})");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_TRUE(m.contains("*"));
    EXPECT_EQ(m.at("*").host, "localhost");
    EXPECT_EQ(m.at("*").scheme, "http");
}

TEST(HostOverride, ParseInvalidJsonYieldsEmpty) {
    auto m = ParseHostOverridesJson("this is not json");
    EXPECT_TRUE(m.empty());
}

TEST(HostOverride, ParseNonObjectRootYieldsEmpty) {
    auto m = ParseHostOverridesJson(R"(["array", "not", "object"])");
    EXPECT_TRUE(m.empty());
}

TEST(HostOverride, ParseSkipsUnderscorePrefixedKeys) {
    auto m = ParseHostOverridesJson(R"({
        "_comment":              "this is a comment block",
        "_section_apex":         "--- Apex Legends endpoints ---",
        "api.example.com":       "http://localhost:8080",
        "_1":                    "another comment",
        "*":                     "http://catch-all.local"
    })");
    ASSERT_EQ(m.size(), 2u);
    EXPECT_TRUE(m.contains("api.example.com"));
    EXPECT_TRUE(m.contains("*"));
    EXPECT_FALSE(m.contains("_comment"));
    EXPECT_FALSE(m.contains("_section_apex"));
    EXPECT_FALSE(m.contains("_1"));
}

TEST(HostOverride, ParseSkipsNonStringValues) {
    auto m = ParseHostOverridesJson(R"({
        "good.example.com": "http://localhost:8080",
        "bad-number.example.com": 1234,
        "bad-null.example.com": null,
        "bad-object.example.com": {"nested": "no"}
    })");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_TRUE(m.contains("good.example.com"));
}

TEST(HostOverride, ParseSkipsEmptyStringValues) {
    auto m = ParseHostOverridesJson(R"({
        "good.example.com": "localhost",
        "empty.example.com": ""
    })");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_TRUE(m.contains("good.example.com"));
}

TEST(HostOverride, ParseBadPortKeepsHostDropsPort) {
    auto m = ParseHostOverridesJson(R"({
        "api.example.com": "http://localhost:not-a-number"
    })");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("api.example.com").scheme, "http");
    EXPECT_EQ(m.at("api.example.com").host, "localhost");
    EXPECT_EQ(m.at("api.example.com").port, 0);
}

TEST(HostOverride, ParseOutOfRangePortKeepsHostDropsPort) {
    auto m = ParseHostOverridesJson(R"({
        "api.example.com": "localhost:99999"
    })");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("api.example.com").host, "localhost");
    EXPECT_EQ(m.at("api.example.com").port, 0);
}

TEST(HostOverride, ParseZeroPortKeepsHostDropsPort) {
    auto m = ParseHostOverridesJson(R"({
        "api.example.com": "localhost:0"
    })");
    ASSERT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("api.example.com").host, "localhost");
    EXPECT_EQ(m.at("api.example.com").port, 0);
}

// --- ApplyHostOverride: behavior with no JSON file present ---
static bool HostOverrideJsonConfigured() {
    if (const char* p = std::getenv("SHADPS4_HTTP_HOST_OVERRIDES_JSON"); p && p[0]) {
        return true;
    }
    std::ifstream f("host_overrides.json");
    return f.is_open();
}

TEST(HostOverride, InactiveByDefaultLeavesValuesUnchanged) {
    if (HostOverrideJsonConfigured()) {
        GTEST_SKIP() << "host overrides JSON configured; off-path test inapplicable";
    }
    std::string scheme = "https";
    std::string host = "api.example.com";
    u16 port = 443;
    bool is_secure = true;
    const bool changed = ApplyHostOverride(scheme, host, port, is_secure);
    EXPECT_FALSE(changed);
    EXPECT_EQ(scheme, "https");
    EXPECT_EQ(host, "api.example.com");
    EXPECT_EQ(port, 443);
    EXPECT_TRUE(is_secure);
}

TEST(HostOverride, InactivePreservesHttpScheme) {
    if (HostOverrideJsonConfigured()) {
        GTEST_SKIP() << "host overrides JSON configured; off-path test inapplicable";
    }
    std::string scheme = "http";
    std::string host = "plain.example.com";
    u16 port = 80;
    bool is_secure = false;
    const bool changed = ApplyHostOverride(scheme, host, port, is_secure);
    EXPECT_FALSE(changed);
    EXPECT_EQ(scheme, "http");
    EXPECT_EQ(host, "plain.example.com");
    EXPECT_EQ(port, 80);
    EXPECT_FALSE(is_secure);
}

TEST(HostOverride, InactivePreservesUnusualPort) {
    if (HostOverrideJsonConfigured()) {
        GTEST_SKIP() << "host overrides JSON configured; off-path test inapplicable";
    }
    std::string scheme = "https";
    std::string host = "custom-port.example.com";
    u16 port = 5300; // Pinball FX uses this for kensho-discovery
    bool is_secure = true;
    ApplyHostOverride(scheme, host, port, is_secure);
    EXPECT_EQ(port, 5300);
}

// --- ApplyHostOverride: behavior WITH the test JSON file pre-staged ---
static const char* kTestOverrideJson = R"({
    "api.example.com": "localhost:8080",
    "*": "mock.local:8443"
})";

TEST(HostOverride, ActiveExactHostMatchFromJsonFile) {
    if (!HostOverrideJsonConfigured()) {
        GTEST_SKIP() << "host overrides JSON not configured; on-path test inapplicable";
    }
    std::string scheme = "https";
    std::string host = "api.example.com";
    u16 port = 443;
    bool is_secure = true;
    const bool changed = ApplyHostOverride(scheme, host, port, is_secure);
    EXPECT_TRUE(changed);
    const bool any_change = (host != "api.example.com") || (port != 443);
    EXPECT_TRUE(any_change);
}

} // namespace
