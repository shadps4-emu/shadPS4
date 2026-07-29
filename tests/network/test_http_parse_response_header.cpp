// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "common/types.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http_error.h"

#ifndef ORBIS_OK
#define ORBIS_OK 0
#endif

using namespace Libraries::Http;

namespace {

class HttpParseResponseHeader : public ::testing::Test {
protected:
    const char* value{nullptr};
    u64 valueLen{0};

    int Parse(std::string_view header, const char* field) {
        value = nullptr;
        valueLen = 0;
        return sceHttpParseResponseHeader(header.data(), header.size(), field, &value, &valueLen);
    }
    std::string ValueAsString() const {
        if (!value || valueLen == 0)
            return std::string{};
        return std::string(value, valueLen);
    }
};

TEST_F(HttpParseResponseHeader, SimpleField) {
    const char* hdr = "Content-Type: text/html\r\n";
    const int ret = Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Type");
    EXPECT_GT(ret, 0);
    EXPECT_EQ(ret, 25); // length up to and including the '\n'
    EXPECT_EQ(ValueAsString(), "text/html");
}

TEST_F(HttpParseResponseHeader, LFOnlyAccepted) {
    const char* hdr = "Content-Type: text/html\n";
    const int ret = Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Type");
    EXPECT_GT(ret, 0);
    EXPECT_EQ(ValueAsString(), "text/html");
}

TEST_F(HttpParseResponseHeader, CaseInsensitiveFieldName) {
    const char* hdr = "Content-Type: text/html\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "content-type"), 0);
    EXPECT_EQ(ValueAsString(), "text/html");

    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "CONTENT-TYPE"), 0);
    EXPECT_EQ(ValueAsString(), "text/html");
}

// Multiple headers, find second.
TEST_F(HttpParseResponseHeader, FindSecondHeader) {
    const char* hdr = "Date: Mon, 01 Jan 2024 00:00:00 GMT\r\n"
                      "Content-Length: 1234\r\n"
                      "Content-Type: text/plain\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Length"), 0);
    EXPECT_EQ(ValueAsString(), "1234");
}

// Field with multiple leading spaces after colon
TEST_F(HttpParseResponseHeader, MultipleLeadingSpacesStripped) {
    const char* hdr = "Content-Type:    text/html\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Type"), 0);
    EXPECT_EQ(ValueAsString(), "text/html");
}

// No space after colon
TEST_F(HttpParseResponseHeader, NoSpaceAfterColon) {
    const char* hdr = "Content-Type:text/html\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Type"), 0);
    EXPECT_EQ(ValueAsString(), "text/html");
}

// Tab as separator (whitespace).
TEST_F(HttpParseResponseHeader, TabAfterColon) {
    const char* hdr = "Content-Type:\ttext/html\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Content-Type"), 0);
    EXPECT_EQ(ValueAsString(), "text/html");
}

// CRLF stripped from value length but LF position included in return.
TEST_F(HttpParseResponseHeader, CRStrippedFromValueLength) {
    const char* hdr = "Server: nginx\r\n";
    const int ret = Parse(std::string_view(hdr, std::strlen(hdr)), "Server");
    EXPECT_EQ(ret, 15);
    EXPECT_EQ(valueLen, 5u); // "nginx" - no trailing \r
    EXPECT_EQ(ValueAsString(), "nginx");
}

TEST_F(HttpParseResponseHeader, LineFoldingWithSpace) {
    // Value continues onto next line because second line starts with ' '.
    const char* hdr = "X-Custom: part1\r\n part2\r\nNext-Header: x\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "X-Custom"), 0);
    EXPECT_NE(value, nullptr);
    EXPECT_GT(valueLen, std::strlen("part1"));
    // The captured value should include the folded continuation.
    const std::string got(value, valueLen);
    EXPECT_NE(got.find("part1"), std::string::npos);
    EXPECT_NE(got.find("part2"), std::string::npos);
}

TEST_F(HttpParseResponseHeader, LineFoldingWithTab) {
    const char* hdr = "X-Custom: part1\r\n\tpart2\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "X-Custom"), 0);
    const std::string got(value, valueLen);
    EXPECT_NE(got.find("part1"), std::string::npos);
    EXPECT_NE(got.find("part2"), std::string::npos);
}

TEST_F(HttpParseResponseHeader, FieldNotFound) {
    const char* hdr = "Content-Type: text/html\r\n";
    EXPECT_EQ(Parse(std::string_view(hdr, std::strlen(hdr)), "Server"),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_NOT_FOUND));
}

// Empty header buffer (headerLen == 0)
TEST_F(HttpParseResponseHeader, EmptyHeaderBuffer) {
    EXPECT_EQ(Parse(std::string_view("", 0u), "X-Anything"),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_NOT_FOUND));
}

// Field name appears mid-line (not at start of line)
TEST_F(HttpParseResponseHeader, MidLineFieldNameNotMatched) {
    const char* hdr = "X-Reason: not-a-Server: header\r\nServer: nginx\r\n";
    const int ret = Parse(std::string_view(hdr, std::strlen(hdr)), "Server");
    EXPECT_GT(ret, 0);
    EXPECT_EQ(ValueAsString(), "nginx");
}

// Continuation line (starts with whitespace) must not be treated as a field
// name match start.
TEST_F(HttpParseResponseHeader, ContinuationLineNotMatched) {
    const char* hdr = "X-A: v1\r\n Server: not-here\r\nServer: real\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Server"), 0);
    EXPECT_EQ(ValueAsString(), "real");
}

// Field name without colon - skipped.
TEST_F(HttpParseResponseHeader, FieldNameWithoutColon) {
    const char* hdr = "Servernotacolon here\r\nServer: nginx\r\n";
    EXPECT_GT(Parse(std::string_view(hdr, std::strlen(hdr)), "Server"), 0);
    EXPECT_EQ(ValueAsString(), "nginx");
}

TEST_F(HttpParseResponseHeader, NullHeaderReturnsInvalidResponse) {
    const char* v;
    u64 vl;
    EXPECT_EQ(sceHttpParseResponseHeader(nullptr, 0, "X", &v, &vl),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE));
}

TEST_F(HttpParseResponseHeader, NullFieldStrReturnsInvalidValue) {
    const char* v;
    u64 vl;
    EXPECT_EQ(sceHttpParseResponseHeader("X: y\r\n", 6, nullptr, &v, &vl),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE));
}

TEST_F(HttpParseResponseHeader, NullFieldValueReturnsInvalidValue) {
    u64 vl;
    EXPECT_EQ(sceHttpParseResponseHeader("X: y\r\n", 6, "X", nullptr, &vl),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE));
}

TEST_F(HttpParseResponseHeader, NullValueLenReturnsInvalidValue) {
    const char* v;
    EXPECT_EQ(sceHttpParseResponseHeader("X: y\r\n", 6, "X", &v, nullptr),
              static_cast<int>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE));
}

TEST_F(HttpParseResponseHeader, ErrorCodesMatchDocs) {
    EXPECT_EQ(static_cast<unsigned>(ORBIS_HTTP_ERROR_PARSE_HTTP_NOT_FOUND), 0x80432025u);
    EXPECT_EQ(static_cast<unsigned>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE), 0x80432060u);
    EXPECT_EQ(static_cast<unsigned>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE), 0x804321feu);
}

TEST_F(HttpParseResponseHeader, DocsExampleIteration) {
    // Docs example loops calling the function with counter += ret to find
    // multiple matching fields.
    const char* hdr = "Date: Mon, 01 Jan 2024 12:00:00 GMT\r\n"
                      "Date: Tue, 02 Jan 2024 12:00:00 GMT\r\n"
                      "Server: x\r\n";
    const u64 hdrLen = std::strlen(hdr);

    std::vector<std::string> seen;
    u64 counter = 0;
    while (counter < hdrLen) {
        const int ret =
            sceHttpParseResponseHeader(hdr + counter, hdrLen - counter, "Date", &value, &valueLen);
        if (ret <= 0)
            break;
        seen.emplace_back(value, valueLen);
        counter += ret;
    }
    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[0], "Mon, 01 Jan 2024 12:00:00 GMT");
    EXPECT_EQ(seen[1], "Tue, 02 Jan 2024 12:00:00 GMT");
}

} // namespace
