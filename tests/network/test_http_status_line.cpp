// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "common/types.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http_error.h"

#ifndef ORBIS_OK
#define ORBIS_OK 0
#endif

using namespace Libraries::Http;

namespace {

class HttpStatusLine : public ::testing::Test {
protected:
    int32_t major{}, minor{}, code{};
    const char* phrase{nullptr};
    u64 phraseLen{0};

    int Parse(std::string_view sv) {
        major = -1;
        minor = -1;
        code = -1;
        phrase = nullptr;
        phraseLen = 0;
        return sceHttpParseStatusLine(sv.data(), sv.size(), &major, &minor, &code, &phrase,
                                      &phraseLen);
    }
};

// Canonical "HTTP/1.1 200 OK\n"
TEST_F(HttpStatusLine, CanonicalParse) {
    const char* line = "HTTP/1.1 200 OK\n";
    EXPECT_EQ(Parse(std::string_view(line, 16)), 16);
    EXPECT_EQ(major, 1);
    EXPECT_EQ(minor, 1);
    EXPECT_EQ(code, 200);
    ASSERT_NE(phrase, nullptr);
    EXPECT_EQ(phrase, line + 12);
    EXPECT_EQ(phraseLen, 3u);
    EXPECT_EQ(std::string(phrase, phraseLen), " OK");
}

TEST_F(HttpStatusLine, CRLFStripped) {
    const char* line = "HTTP/1.1 200 OK\r\n";
    EXPECT_EQ(Parse(std::string_view(line, 17)), 17);
    EXPECT_EQ(phraseLen, 3u); // " OK" - '\r' stripped
    EXPECT_EQ(std::string(phrase, phraseLen), " OK");
}

TEST_F(HttpStatusLine, NoSpaceAfterCodeAccepted) {
    const char* line = "HTTP/1.1 200OK\n";
    EXPECT_EQ(Parse(std::string_view(line, 15)), 15);
    EXPECT_EQ(code, 200);
    EXPECT_EQ(std::string(phrase, phraseLen), "OK");
}

TEST_F(HttpStatusLine, EmptyReasonPhrase) {
    const char* line = "HTTP/1.1 200\n";
    EXPECT_EQ(Parse(std::string_view(line, 13)), 13);
    EXPECT_EQ(code, 200);
    EXPECT_EQ(phraseLen, 0u);
    EXPECT_EQ(phrase, line + 12);
}

TEST_F(HttpStatusLine, MissingNewlineRejected) {
    const char* line = "HTTP/1.1 200 OK\r";
    EXPECT_EQ(Parse(std::string_view(line, 16)), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Multi-digit version numbers.
TEST_F(HttpStatusLine, MultiDigitVersions) {
    const char* line = "HTTP/12.34 200 X\n";
    EXPECT_EQ(Parse(std::string_view(line, 17)), 17);
    EXPECT_EQ(major, 12);
    EXPECT_EQ(minor, 34);
}

// Null check ordering.
TEST_F(HttpStatusLine, NullStatusLine) {
    int32_t a, b, c;
    const char* p;
    u64 l;
    EXPECT_EQ(sceHttpParseStatusLine(nullptr, 0, &a, &b, &c, &p, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

TEST_F(HttpStatusLine, NullMajor) {
    int32_t b, c;
    const char* p;
    u64 l;
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.1 200 \n", 14, nullptr, &b, &c, &p, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
}

// Too short.
TEST_F(HttpStatusLine, TooShort) {
    EXPECT_EQ(Parse("HTTP/1."), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Wrong prefix.
TEST_F(HttpStatusLine, WrongPrefix) {
    EXPECT_EQ(Parse("XTTP/1.1 200 OK\n"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Non-digit after HTTP/.
TEST_F(HttpStatusLine, NonDigitMajor) {
    EXPECT_EQ(Parse("HTTP/A.1 200 OK\n"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Missing dot.
TEST_F(HttpStatusLine, MissingDot) {
    EXPECT_EQ(Parse("HTTP/11 200 OK\n"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Missing space after minor version.
TEST_F(HttpStatusLine, MissingSpaceAfterMinor) {
    EXPECT_EQ(Parse("HTTP/1.1A200 OK\n"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

TEST_F(HttpStatusLine, HighBitByteRejected) {
    char line[] = "HTTP/1\x80"
                  "1 200 OK\n";
    EXPECT_EQ(Parse(std::string_view(line, sizeof(line) - 1)),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Response code with non-digit.
TEST_F(HttpStatusLine, NonDigitResponseCode) {
    EXPECT_EQ(Parse("HTTP/1.1 2X0 OK\n"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

// Not enough room for response code.
TEST_F(HttpStatusLine, ResponseCodeTruncated) {
    EXPECT_EQ(Parse("HTTP/1.1 20"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
}

} // namespace

namespace {

class HttpStatusLineDocs : public ::testing::Test {
protected:
    int32_t major{-1}, minor{-1}, code{-1};
    const char* phrase{nullptr};
    u64 phraseLen{0};

    int Parse(std::string_view sv) {
        return sceHttpParseStatusLine(sv.data(), sv.size(), &major, &minor, &code, &phrase,
                                      &phraseLen);
    }
};

TEST_F(HttpStatusLineDocs, ExampleReturnsLineLengthIncludingCRLF) {
    const char* header = "HTTP/1.0 200 OK\r\n";
    const int ret = Parse(std::string_view(header, std::strlen(header)));
    EXPECT_EQ(ret, 17);
    EXPECT_EQ(major, 1);
    EXPECT_EQ(minor, 0);
    EXPECT_EQ(code, 200);
    ASSERT_NE(phrase, nullptr);
    EXPECT_EQ(phrase, header + 12);
    EXPECT_EQ(phraseLen, 3u);
    EXPECT_EQ(std::string(phrase, phraseLen), " OK");
}

TEST_F(HttpStatusLineDocs, LFAloneAcceptedByFirmware) {
    const char* header = "HTTP/1.0 200 OK\n";
    EXPECT_GT(Parse(std::string_view(header, std::strlen(header))), 0);
    EXPECT_EQ(major, 1);
    EXPECT_EQ(minor, 0);
    EXPECT_EQ(code, 200);
}

TEST_F(HttpStatusLineDocs, ExtraBytesAfterCRLFIgnored) {
    const char* header = "HTTP/1.0 200 OK\r\nHost: x\r\n\r\n";
    const int ret = Parse(std::string_view(header, std::strlen(header)));
    EXPECT_EQ(ret, 17); // stops right after first '\n'
}

TEST_F(HttpStatusLineDocs, EmptyPhraseWithCRLF) {
    const char* header = "HTTP/1.0 200\r\n";
    EXPECT_EQ(Parse(std::string_view(header, std::strlen(header))), 14);
    EXPECT_EQ(phraseLen, 0u);
    ASSERT_NE(phrase, nullptr);
    EXPECT_EQ(phrase, header + 12); // points to '\r'
}

TEST_F(HttpStatusLineDocs, ErrorCodeForInvalidResponse) {
    EXPECT_EQ(Parse("HTTP/1.0 200 OK"), ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE);
    EXPECT_EQ(static_cast<unsigned>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE), 0x80432060u);
}

TEST_F(HttpStatusLineDocs, ErrorCodeForInvalidValue) {
    int32_t a, b, c;
    const char* p;
    u64 l;
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.0 200 OK\r\n", 17, nullptr, &b, &c, &p, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.0 200 OK\r\n", 17, &a, nullptr, &c, &p, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.0 200 OK\r\n", 17, &a, &b, nullptr, &p, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.0 200 OK\r\n", 17, &a, &b, &c, nullptr, &l),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
    EXPECT_EQ(sceHttpParseStatusLine("HTTP/1.0 200 OK\r\n", 17, &a, &b, &c, &p, nullptr),
              ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE);
    EXPECT_EQ(static_cast<unsigned>(ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE), 0x804321feu);
}

} // namespace
