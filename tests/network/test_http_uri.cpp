// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "common/types.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http_error.h"

#ifndef ORBIS_OK
#define ORBIS_OK 0
#endif

using namespace Libraries::Http;

namespace {

class HttpUri : public ::testing::Test {
protected:
    static constexpr size_t kPoolSize = 4096;
    static constexpr size_t kBufSize = 4096;

    OrbisHttpUriElement el{};
    std::array<char, kPoolSize> pool{};
    std::array<char, kBufSize> buf{};
    u64 require = 0;

    // Compute pool size needed by sceHttpUriParse, then call it with that exact size.
    int Parse(const char* uri) {
        u64 needed = 0;
        const int sz = sceHttpUriParse(nullptr, uri, nullptr, &needed, 0);
        if (sz != ORBIS_OK) {
            return sz;
        }
        EXPECT_LE(needed, kPoolSize);
        std::memset(&el, 0, sizeof(el));
        std::memset(pool.data(), 0, kPoolSize);
        return sceHttpUriParse(&el, uri, pool.data(), &require, needed);
    }
};

TEST_F(HttpUri, ParseHttpDefaultPort) {
    ASSERT_EQ(Parse("http://example.com/path"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "http");
    EXPECT_STREQ(el.hostname, "example.com");
    EXPECT_STREQ(el.path, "/path");
    EXPECT_EQ(el.port, 80);
    EXPECT_FALSE(el.opaque);
}

TEST_F(HttpUri, ParseHttpsDefaultPort) {
    ASSERT_EQ(Parse("https://example.com/foo"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "https");
    EXPECT_EQ(el.port, 443);
    EXPECT_FALSE(el.opaque);
}

TEST_F(HttpUri, ParseExplicitPortOverridesDefault) {
    ASSERT_EQ(Parse("http://example.com:8080/foo"), ORBIS_OK);
    EXPECT_STREQ(el.hostname, "example.com");
    EXPECT_EQ(el.port, 8080);
    EXPECT_STREQ(el.path, "/foo");
}

TEST_F(HttpUri, ParseUserAndPassword) {
    ASSERT_EQ(Parse("http://alice:secret@example.com/path"), ORBIS_OK);
    EXPECT_STREQ(el.username, "alice");
    EXPECT_STREQ(el.password, "secret");
    EXPECT_STREQ(el.hostname, "example.com");
    EXPECT_STREQ(el.path, "/path");
}

TEST_F(HttpUri, ParseUserOnlyHasEmptyPassword) {
    ASSERT_EQ(Parse("http://alice@example.com/"), ORBIS_OK);
    EXPECT_STREQ(el.username, "alice");
    EXPECT_STREQ(el.password, "");
}

TEST_F(HttpUri, ParseQueryAndFragment) {
    ASSERT_EQ(Parse("http://h/p?q=1#frag"), ORBIS_OK);
    EXPECT_STREQ(el.path, "/p");
    EXPECT_STREQ(el.query, "?q=1");
    EXPECT_STREQ(el.fragment, "#frag");
}

TEST_F(HttpUri, ParseSchemeRelativeUrl) {
    ASSERT_EQ(Parse("//cdn.example.com/asset.js"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "");
    EXPECT_STREQ(el.hostname, "cdn.example.com");
    EXPECT_STREQ(el.path, "/asset.js");
    EXPECT_FALSE(el.opaque);
}

TEST_F(HttpUri, ParsePathIsNormalizedViaSweep) {
    // sceHttpUriParse calls sceHttpUriSweepPath on the path component, so /a/b/../c -> /a/c
    ASSERT_EQ(Parse("http://h/a/b/../c"), ORBIS_OK);
    EXPECT_STREQ(el.path, "/a/c");
}

TEST_F(HttpUri, ParseSchemePrefixMatchHttps) {
    ASSERT_EQ(Parse("HTTPSFOO://x/"), ORBIS_OK);
    EXPECT_EQ(el.port, 443);
}

TEST_F(HttpUri, ParseIPv6HostStripsBrackets) {
    ASSERT_EQ(Parse("http://[2001:db8::1]:8080/x"), ORBIS_OK);
    EXPECT_STREQ(el.hostname, "2001:db8::1");
    EXPECT_EQ(el.port, 8080);
    EXPECT_STREQ(el.path, "/x");
}

TEST_F(HttpUri, ParseMailtoUsesAuthorityScan) {
    ASSERT_EQ(Parse("mailto:user@example.com"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "mailto");
    EXPECT_TRUE(el.opaque);
    EXPECT_STREQ(el.username, "user");
    EXPECT_STREQ(el.hostname, "example.com");
    EXPECT_STREQ(el.path, "");
}

TEST_F(HttpUri, ParseDotFirstCharProducesEmptyHost) {
    ASSERT_EQ(Parse("./bar.html"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "");
    EXPECT_TRUE(el.opaque);
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "./bar.html");
}

TEST_F(HttpUri, ParseOpaqueSchemeStillGetsDefaultPort) {
    ASSERT_EQ(Parse("http:"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "http");
    EXPECT_EQ(el.port, 80);
    EXPECT_TRUE(el.opaque);
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "");
}

TEST_F(HttpUri, ParseRejectsCharAfterPortOtherThanSlashOrNull) {
    u64 needed = 0;
    EXPECT_EQ(sceHttpUriParse(nullptr, "http://h:80?q=1", nullptr, &needed, 0),
              ORBIS_HTTP_ERROR_INVALID_URL);
}

TEST_F(HttpUri, ParseRejectsPortGreaterThanLimit) {
    u64 needed = 0;
    EXPECT_EQ(sceHttpUriParse(nullptr, "http://h:999999/x", nullptr, &needed, 0),
              ORBIS_HTTP_ERROR_INVALID_URL);
}

TEST_F(HttpUri, ParseNullUriReturnsInvalidUrl) {
    u64 needed = 0;
    EXPECT_EQ(sceHttpUriParse(nullptr, nullptr, nullptr, &needed, 0), ORBIS_HTTP_ERROR_INVALID_URL);
}

TEST_F(HttpUri, ParseSizeQueryYieldsNonZeroRequirement) {
    u64 needed = 0;
    ASSERT_EQ(sceHttpUriParse(nullptr, "http://example.com/foo", nullptr, &needed, 0), ORBIS_OK);
    EXPECT_GT(needed, 0u);
}

TEST_F(HttpUri, ParseNoOutputArgsAndNoRequireReturnsInvalidValue) {
    EXPECT_EQ(sceHttpUriParse(nullptr, "http://x/", nullptr, nullptr, 0),
              ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, ParseInsufficientPoolReturnsOutOfMemory) {
    OrbisHttpUriElement local{};
    char tiny[2] = {};
    EXPECT_EQ(sceHttpUriParse(&local, "http://example.com/", tiny, &require, sizeof(tiny)),
              ORBIS_HTTP_ERROR_OUT_OF_MEMORY);
}

TEST_F(HttpUri, BuildHttpWithDefaultPortOmitsPort) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("http");
    src.hostname = const_cast<char*>("example.com");
    src.port = 80;
    src.path = const_cast<char*>("/foo");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://example.com/foo");
}

TEST_F(HttpUri, BuildHttpWithExplicitNonDefaultPort) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("http");
    src.hostname = const_cast<char*>("example.com");
    src.port = 8080;
    src.path = const_cast<char*>("/foo");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://example.com:8080/foo");
}

TEST_F(HttpUri, BuildHttpsWithDefaultPortOmitsPort) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("https");
    src.hostname = const_cast<char*>("a.b");
    src.port = 443;
    src.path = const_cast<char*>("/x");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "https://a.b/x");
}

TEST_F(HttpUri, BuildMailtoPortSkipIsCaseSensitive) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("MAILTO");
    src.hostname = const_cast<char*>("h");
    src.port = 25;
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "MAILTO://h:25");
}

TEST_F(HttpUri, BuildOpaqueUriOmitsDoubleSlash) {
    OrbisHttpUriElement src{};
    src.opaque = true;
    src.scheme = const_cast<char*>("mailto");
    src.path = const_cast<char*>("user@example.com");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "mailto:user@example.com");
}

TEST_F(HttpUri, BuildNullSourceReturnsInvalidUrl) {
    EXPECT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, nullptr, 0xFF),
              ORBIS_HTTP_ERROR_INVALID_URL);
}

TEST_F(HttpUri, BuildBothOutputAndRequireNullReturnsInvalidValue) {
    OrbisHttpUriElement src{};
    src.scheme = const_cast<char*>("http");
    EXPECT_EQ(sceHttpUriBuild(nullptr, nullptr, 0, &src, 0xFF), ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, BuildSizeQueryWithNullOutPopulatesRequire) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("http");
    src.hostname = const_cast<char*>("example.com");
    src.path = const_cast<char*>("/foo");
    EXPECT_EQ(sceHttpUriBuild(nullptr, &require, 0, &src, 0xFF), ORBIS_OK);
    EXPECT_GE(require, std::strlen("http://example.com/foo") + 1);
}

TEST_F(HttpUri, BuildInsufficientBufferReturnsOutOfMemory) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("http");
    src.hostname = const_cast<char*>("example.com");
    src.path = const_cast<char*>("/foo");
    char tiny[4] = {};
    EXPECT_EQ(sceHttpUriBuild(tiny, &require, sizeof(tiny), &src, 0xFF),
              ORBIS_HTTP_ERROR_OUT_OF_MEMORY);
}

TEST_F(HttpUri, BuildRespectsOptionMask) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("http");
    src.hostname = const_cast<char*>("example.com");
    src.path = const_cast<char*>("/foo");
    src.query = const_cast<char*>("?q=1");
    // Without WITH_QUERY the query is suppressed.
    const u32 opt = ORBIS_HTTP_URI_BUILD_WITH_SCHEME | ORBIS_HTTP_URI_BUILD_WITH_HOSTNAME |
                    ORBIS_HTTP_URI_BUILD_WITH_PATH;
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, opt), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://example.com/foo");
}

TEST_F(HttpUri, SweepPathZeroSizeShortCircuitsBeforeNullCheck) {
    EXPECT_EQ(sceHttpUriSweepPath(nullptr, nullptr, 0), ORBIS_OK);
}

TEST_F(HttpUri, SweepPathNullDstNonZeroSize) {
    EXPECT_EQ(sceHttpUriSweepPath(nullptr, "/foo", 5), ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, SweepPathNullSrcNonZeroSize) {
    char dst[16];
    EXPECT_EQ(sceHttpUriSweepPath(dst, nullptr, 5), ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, SweepPathNonAbsoluteCopiesVerbatim) {
    char dst[64] = {};
    const char* src = "foo/../bar";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "foo/../bar");
}

TEST_F(HttpUri, SweepPathAbsoluteRemovesDotDot) {
    char dst[64] = {};
    const char* src = "/foo/../bar";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "/bar");
}

TEST_F(HttpUri, SweepPathAbsoluteRemovesSingleDot) {
    char dst[64] = {};
    const char* src = "/foo/./bar";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "/foo/bar");
}

TEST_F(HttpUri, SweepPathTrailingDotIsPreservedLiterally) {
    char dst[64] = {};
    const char* src = "/foo/.";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "/foo/.");
}

TEST_F(HttpUri, SweepPathTrailingDotDotIsPreservedLiterally) {
    char dst[64] = {};
    const char* src = "/foo/..";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "/foo/..");
}

TEST_F(HttpUri, SweepPathSdkDocExample) {
    // Example from the PS4 SDK docs / sceHttpUriSweepPath reference.
    char dst[128] = {};
    const char* src = "/foo/bar/../foo/././../../../test/index.html";
    ASSERT_EQ(sceHttpUriSweepPath(dst, src, std::strlen(src) + 1), ORBIS_OK);
    EXPECT_STREQ(dst, "/test/index.html");
}

TEST_F(HttpUri, SweepPathRootOnly) {
    char dst[8] = {};
    ASSERT_EQ(sceHttpUriSweepPath(dst, "/", 2), ORBIS_OK);
    EXPECT_STREQ(dst, "/");
}

TEST_F(HttpUri, MergeWithAbsoluteRelativeReturnsRelativeAsIs) {
    char base[] = "http://foo.com/foo/index.html";
    char absolute[] = "http://bar.com/other";
    ASSERT_EQ(sceHttpUriMerge(buf.data(), base, absolute, &require, kBufSize, 0), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://bar.com/other");
}

TEST_F(HttpUri, MergeWithRelativeDoesNotSweepDotSegments) {
    char base[] = "http://foo.com/foo/index.html";
    char rel[] = "./default.html";
    ASSERT_EQ(sceHttpUriMerge(buf.data(), base, rel, &require, kBufSize, 0), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://foo.com/foo/./default.html");
}

TEST_F(HttpUri, MergeWithRelativeDoesNotSweepParentSegments) {
    char base[] = "http://foo.com/foo/index.html";
    char up[] = "../sibling.html";
    ASSERT_EQ(sceHttpUriMerge(buf.data(), base, up, &require, kBufSize, 0), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "http://foo.com/foo/../sibling.html");
}

TEST_F(HttpUri, MergeNullBaseReturnsInvalidValue) {
    char rel[] = "./x";
    EXPECT_EQ(sceHttpUriMerge(buf.data(), nullptr, rel, &require, kBufSize, 0),
              ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, MergeNullRelativeReturnsInvalidValue) {
    char base[] = "http://h/";
    EXPECT_EQ(sceHttpUriMerge(buf.data(), base, nullptr, &require, kBufSize, 0),
              ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, MergeNonZeroFlagReturnsInvalidValue) {
    // Per firmware: the last param must be 0; non-zero is rejected.
    char base[] = "http://h/";
    char rel[] = "./x";
    EXPECT_EQ(sceHttpUriMerge(buf.data(), base, rel, &require, kBufSize, 1),
              ORBIS_HTTP_ERROR_INVALID_VALUE);
}

TEST_F(HttpUri, EscapeSpace) {
    ASSERT_EQ(sceHttpUriEscape(buf.data(), &require, kBufSize, "hello world"), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "hello%20world");
}

TEST_F(HttpUri, UnescapePercentTwenty) {
    ASSERT_EQ(sceHttpUriUnescape(buf.data(), &require, kBufSize, "hello%20world"), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "hello world");
}

TEST_F(HttpUri, EscapeUnescapeRoundtrip) {
    const char* original = "a b/c?d=e&f=g h";
    char escaped[128] = {};
    ASSERT_EQ(sceHttpUriEscape(escaped, &require, sizeof(escaped), original), ORBIS_OK);
    char unescaped[128] = {};
    ASSERT_EQ(sceHttpUriUnescape(unescaped, &require, sizeof(unescaped), escaped), ORBIS_OK);
    EXPECT_STREQ(unescaped, original);
}

TEST_F(HttpUri, ParseBuildRoundtripFull) {
    const char* original = "http://alice:secret@host.example.com:8080/path?q=1#frag";
    ASSERT_EQ(Parse(original), ORBIS_OK);
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &el, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), original);
}

TEST_F(HttpUri, ParseBuildRoundtripSimple) {
    const char* original = "https://example.com/";
    ASSERT_EQ(Parse(original), ORBIS_OK);
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &el, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), original);
}

} // namespace

namespace {

// "file:///etc/passwd" - canonical form with empty authority + absolute path
TEST_F(HttpUri, ParseFileUrlEmptyAuthority) {
    ASSERT_EQ(Parse("file:///etc/passwd"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "file");
    EXPECT_FALSE(el.opaque);
    EXPECT_STREQ(el.username, "");
    EXPECT_STREQ(el.password, "");
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "/etc/passwd");
    EXPECT_EQ(el.port, 0);
}

TEST_F(HttpUri, ParseFileUrlSingleSlashIsOpaque) {
    ASSERT_EQ(Parse("file:/etc/passwd"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "file");
    EXPECT_TRUE(el.opaque);
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "/etc/passwd");
    EXPECT_EQ(el.port, 0);
}

TEST_F(HttpUri, ParseFileUrlWithLocalhost) {
    ASSERT_EQ(Parse("file://localhost/etc/passwd"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "file");
    EXPECT_FALSE(el.opaque);
    EXPECT_STREQ(el.hostname, "localhost");
    EXPECT_STREQ(el.path, "/etc/passwd");
    EXPECT_EQ(el.port, 0);
}

TEST_F(HttpUri, ParseFileUrlBareAuthority) {
    ASSERT_EQ(Parse("file://"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "file");
    EXPECT_FALSE(el.opaque);
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "");
    EXPECT_EQ(el.port, 0);
}

TEST_F(HttpUri, ParseFileUrlSchemeOnly) {
    ASSERT_EQ(Parse("file:"), ORBIS_OK);
    EXPECT_STREQ(el.scheme, "file");
    EXPECT_TRUE(el.opaque); // no slashes -> opaque
    EXPECT_STREQ(el.hostname, "");
    EXPECT_STREQ(el.path, "");
    EXPECT_EQ(el.port, 0);
}

TEST_F(HttpUri, BuildFileUrlEmptyAuthority) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("file");
    src.hostname = const_cast<char*>("");
    src.path = const_cast<char*>("/etc/passwd");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "file:///etc/passwd");
}

TEST_F(HttpUri, BuildFileUrlWithHost) {
    OrbisHttpUriElement src{};
    src.opaque = false;
    src.scheme = const_cast<char*>("file");
    src.hostname = const_cast<char*>("localhost");
    src.path = const_cast<char*>("/etc/passwd");
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &src, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), "file://localhost/etc/passwd");
}

TEST_F(HttpUri, ParseBuildFileUrlRoundtrip) {
    const char* original = "file:///etc/passwd";
    ASSERT_EQ(Parse(original), ORBIS_OK);
    ASSERT_EQ(sceHttpUriBuild(buf.data(), &require, kBufSize, &el, 0xFF), ORBIS_OK);
    EXPECT_STREQ(buf.data(), original);
}

} // namespace
