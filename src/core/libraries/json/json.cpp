// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/json/json.h"
#include "core/libraries/libs.h"

namespace Libraries::Json { // sce::Json

s32 PS4_SYSV_ABI Json::Initializer::initialize(Json::InitParameter const*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Initializer::initialize() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Initializer::terminate() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

PS4_SYSV_ABI Json::Initializer::Initializer() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Initializer::~Initializer() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

s32 PS4_SYSV_ABI s_initparam() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Free(void*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referArray() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referValue(unsigned long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referValue(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referObject() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referString() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referBoolean() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referInteger() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referUInteger() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::serialize_internal(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&, void*), void*,
    Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::setNullAccessCallBack(
    Json::Value const& (*)(Json::ValueType, Json::Value const*, void*), void*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(bool) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(double) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(unsigned long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(Json::ValueType) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(Json::Value const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
Json::Value::set(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(
    std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>, Json::Value,
             std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>>,
             Json::StlAlloc<
                 std::pair<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
                           Json::Value>>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::set(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::swap(Json::Value&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::clear() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::referReal() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
Json::Value::serialize(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::serialize(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&, void*), void*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::setParent(Json::Value const*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::operator=(Json::Value const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

PS4_SYSV_ABI Json::Value::Value(bool) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(double) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(unsigned long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(Json::ValueType) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(Json::Value const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI
Json::Value::Value(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(
    std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>, Json::Value,
             std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>>,
             Json::StlAlloc<
                 std::pair<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
                           Json::Value>>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::Value() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::Value::~Value() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

s32 PS4_SYSV_ABI Malloc(unsigned long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseArray(Json::Value&, Json::InputStream&, Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseValue(Json::Value&, Json::InputStream&, Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseNumber(Json::Value&, Json::InputStream&, Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseObject(Json::Value&, Json::InputStream&, Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseString(Json::Value&, Json::InputStream&, Json::Value*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseString(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&, Json::InputStream&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseQuadHex(Json::InputStream&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parseCodePoint(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&, Json::InputStream&) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, int (*)(char&, void*), void*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, char const*) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, char const*, unsigned long) {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

PS4_SYSV_ABI Json::RootParam::RootParam() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

PS4_SYSV_ABI Json::RootParam::~RootParam() {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return;
}

s32 PS4_SYSV_ABI Json::Value::getBoolean() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getInteger() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getUInteger() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::count() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getReal() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getRoot() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getType() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getArray() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getValue(unsigned long) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getValue(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::toString(
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getObject() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::getString() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

PS4_SYSV_ABI Json::Value::operator bool() const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::operator[](unsigned long) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::operator[](char const*) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Json::Value::operator[](
    std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) const {
    LOG_ERROR(Lib_Json, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {

    LIB_OVERLOADED_MEMBER_FUNCTION("Cxwy7wHq4J0", "libSceJson", 1, "libSceJson", 1, 1,
                                   Json::Initializer, initialize,
                                   s32 (Json::Initializer::*)(Json::InitParameter const*));

    LIB_MEMBER_FUNCTION("PR5k1penBLM", "libSceJson", 1, "libSceJson", 1, 1, Json::Initializer,
                        terminate);

    // LIB_FUNCTION("cK6bYHf-Q5E", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Initializer::Initializer());
    // LIB_FUNCTION("6qFqND4iwPA", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Initializer::Initializer());
    // LIB_FUNCTION("RujUxbr3haM", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Initializer::~Initializer());
    // LIB_FUNCTION("qQZGdYkEytk", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Initializer::~Initializer());
    // LIB_FUNCTION("7Xc95EMbk9I", "libSceJson", 1, "libSceJson", 1, 1, Json::s_initparam);
    // LIB_FUNCTION("bnk+edDbqMk", "libSceJson", 1, "libSceJson", 1, 1, Json::Free(void*));
    // LIB_FUNCTION("nM5XqdeXFPw", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referArray());
    // LIB_FUNCTION("gLzCc67aTbw", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::referValue(unsigned long));
    // LIB_FUNCTION("Z3mrBp7y1Lc", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::referValue(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&));
    // LIB_FUNCTION("bUw2Go-rxtE", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::s_nullbool);
    // LIB_FUNCTION("GpEG1IcP86s", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::s_nullreal);
    // LIB_FUNCTION("-NxEk7XLkDY", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referObject);
    // LIB_FUNCTION("m5J28iw-QPs", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referString);
    // LIB_FUNCTION("zwDiN3-CRTA", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::s_nullarray);
    // LIB_FUNCTION("QxjfcfBhEdc", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referBoolean);
    // LIB_FUNCTION("R0ac5QOdlpo", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referInteger);
    // LIB_FUNCTION("3YODFwSqgtc", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::s_nullobject);
    // LIB_FUNCTION("24kDIV0aDzs", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::s_nullstring);
    // LIB_FUNCTION("Nx5tez4siL4", "libSceJson", 1, "libSceJson", 1, 1,
    // Json::Value::referUInteger()); LIB_FUNCTION("dJSD90mnj1w", "libSceJson", 1, "libSceJson", 1,
    // 1, Json::Value::s_nullinteger); LIB_FUNCTION("xxBnIBiUVzY", "libSceJson", 1, "libSceJson", 1,
    // 1, Json::Value::s_nulluinteger); LIB_FUNCTION(
    //     "bkVdWPESKhQ", "libSceJson", 1, "libSceJson", 1, 1,
    //     Json::Value::serialize_internal(
    //         std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //         int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //         void*), void*, Json::Value*));
    // LIB_FUNCTION("Xbl-LYVFNEE", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::setNullAccessCallBack(
    //                  Json::Value const& (*)(Json::ValueType, Json::Value const*, void*), void*));
    // LIB_FUNCTION("5yHuiWXo2gg", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::set(bool));
    // LIB_FUNCTION("BSmWDIkV4w4", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::set(double));
    // LIB_FUNCTION("QxVVYhP-mvg", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::set(long));
    // LIB_FUNCTION("SIe1ZmW7e7s", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::set(unsigned long));
    // LIB_FUNCTION("IKQimvG9Wqs", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::set(Json::ValueType));
    // LIB_FUNCTION("XL8+BUqjB1w", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::set(Json::Value const&));
    // LIB_FUNCTION("pcLFw6F9XHo", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::set(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&));
    // LIB_FUNCTION(
    //     "kLzq1bVqeWg", "libSceJson", 1, "libSceJson", 1, 1,
    //     Json::Value::set(
    //         std::map<
    //             std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
    //             Json::Value, std::less<std::basic_string<char, std::char_traits<char>,
    //             Json::StlAlloc<char>>>, Json::StlAlloc<
    //                 std::pair<std::basic_string<char, std::char_traits<char>,
    //                 Json::StlAlloc<char>>,
    //                           Json::Value>>> const&));
    // LIB_FUNCTION("I8H3EdOu8cw", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::set(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&));
    // LIB_FUNCTION("9PqrCLfKZZI", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::swap(Json::Value&));
    // LIB_FUNCTION("FIjXN2TkuTs", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::clear());
    // LIB_FUNCTION("t7UEJrhojhk", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::referReal());
    // LIB_FUNCTION("GAccTNWvZ8o", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::serialize(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&));
    // LIB_FUNCTION(
    //     "HlDiu7tCnh8", "libSceJson", 1, "libSceJson", 1, 1,
    //     Json::Value::serialize(
    //         std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //         int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //         void*), void*));
    // LIB_FUNCTION("M-tWavI4cgg", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::setParent(Json::Value const*));
    // LIB_FUNCTION("4zrm6VrgIAw", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::operator=(Json::Value const&));
    // LIB_FUNCTION("UeuWT+yNdCQ", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(bool));
    // LIB_FUNCTION("sOmU4vnx3s0", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(double));
    // LIB_FUNCTION("0lLK8+kDqmE", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(long));
    // LIB_FUNCTION("x4AUdbhpRB0", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(unsigned long));
    // LIB_FUNCTION("CbrT3dwDILo", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(Json::ValueType));
    // LIB_FUNCTION("fSb2oQTNrgA", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(Json::Value const&));
    // LIB_FUNCTION("geskGcdamVE", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&));
    // LIB_FUNCTION(
    //     "xxZjYrZssiI", "libSceJson", 1, "libSceJson", 1, 1,
    //     Json::Value::Value(
    //         std::map<
    //             std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
    //             Json::Value, std::less<std::basic_string<char, std::char_traits<char>,
    //             Json::StlAlloc<char>>>, Json::StlAlloc<
    //                 std::pair<std::basic_string<char, std::char_traits<char>,
    //                 Json::StlAlloc<char>>,
    //                           Json::Value>>> const&));
    // LIB_FUNCTION("JGF+59cTHl0", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&));
    // LIB_FUNCTION("qBMjqyBn3OM", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value());
    // LIB_FUNCTION("OK4Ot0ue7J0", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(bool));
    // LIB_FUNCTION("23nm4oXHlfI", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(double));
    // LIB_FUNCTION("EjxUpzR2Yx8", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value(long));
    // LIB_FUNCTION("cqucBPSeVDA", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(unsigned long));
    // LIB_FUNCTION("PWm9MyJJVqU", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(Json::ValueType));
    // LIB_FUNCTION("TZyuFeGuw9Y", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(Json::Value const&));
    // LIB_FUNCTION("QaLXUjWq5uw", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&));
    // LIB_FUNCTION(
    //     "iATcGLFMLw4", "libSceJson", 1, "libSceJson", 1, 1,
    //     Json::Value::Value(
    //         std::map<
    //             std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
    //             Json::Value, std::less<std::basic_string<char, std::char_traits<char>,
    //             Json::StlAlloc<char>>>, Json::StlAlloc<
    //                 std::pair<std::basic_string<char, std::char_traits<char>,
    //                 Json::StlAlloc<char>>,
    //                           Json::Value>>> const&));
    // LIB_FUNCTION("qwP9U9HsZ6k", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::Value(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&));
    // LIB_FUNCTION("-wa17B7TGnw", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::Value());
    // LIB_FUNCTION("WTtYf+cNnXI", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::~Value());
    // LIB_FUNCTION("0eUrW9JAxM0", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::~Value());
    // LIB_FUNCTION("Xzy0onDzSAc", "libSceJson", 1, "libSceJson", 1, 1, Json::Malloc(unsigned
    // long)); LIB_FUNCTION("cYDA0oEX5Y4", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseArray(Json::Value&, Json::InputStream&, Json::Value*));
    // LIB_FUNCTION("vR3+bfUo-Qo", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseValue(Json::Value&, Json::InputStream&, Json::Value*));
    // LIB_FUNCTION("PpR7I7RCsY0", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseNumber(Json::Value&, Json::InputStream&, Json::Value*));
    // LIB_FUNCTION("HwBrtLl-5IY", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseObject(Json::Value&, Json::InputStream&, Json::Value*));
    // LIB_FUNCTION("bglw6PJL4nw", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseString(Json::Value&, Json::InputStream&, Json::Value*));
    // LIB_FUNCTION("QXUHGgXn4JA", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseString(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //                  Json::InputStream&));
    // LIB_FUNCTION("hjDFaEWCV20", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseQuadHex(Json::InputStream&));
    // LIB_FUNCTION("6mTvUfQJa1I", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parseCodePoint(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
    //                  Json::InputStream&));
    // LIB_FUNCTION("itqj2YmuAa8", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parse(Json::Value&, int (*)(char&, void*), void*));
    // LIB_FUNCTION("LB3jxppxyKU", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parse(Json::Value&, char const*));
    // LIB_FUNCTION("S5JxQnoGF3E", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Parser::parse(Json::Value&, char const*, unsigned long));
    // LIB_FUNCTION("Xkq8nvo4tKg", "libSceJson", 1, "libSceJson", 1, 1,
    // Json::RootParam::RootParam()); LIB_FUNCTION("qDu+t28zExY", "libSceJson", 1, "libSceJson", 1,
    // 1, Json::RootParam::RootParam()); LIB_FUNCTION("vOTH9Iec7Yc", "libSceJson", 1, "libSceJson",
    // 1, 1, Json::RootParam::~RootParam()); LIB_FUNCTION("gyz287utQ6c", "libSceJson", 1,
    // "libSceJson", 1, 1, Json::RootParam::~RootParam()); LIB_FUNCTION("zTwZdI8AZ5Y", "libSceJson",
    // 1, "libSceJson", 1, 1,
    //              Json::Value::getBoolean() const);
    // LIB_FUNCTION("DIxvoy7Ngvk", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::getInteger() const);
    // LIB_FUNCTION("sn4HNCtNRzY", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::getUInteger() const);
    // LIB_FUNCTION("RBw+4NukeGQ", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::count() const);
    // LIB_FUNCTION("3qrge7L-AU4", "libSceJson", 1, "libSceJson", 1, 1, Json::Value::getReal()
    // const); LIB_FUNCTION("47s0O9-6E-0", "libSceJson", 1, "libSceJson", 1, 1,
    // Json::Value::getRoot() const); LIB_FUNCTION("SHtAad20YYM", "libSceJson", 1, "libSceJson", 1,
    // 1, Json::Value::getType() const); LIB_FUNCTION("ONT8As5R1ug", "libSceJson", 1, "libSceJson",
    // 1, 1, Json::Value::getArray() const); LIB_FUNCTION("0YqYAoO-+Uo", "libSceJson", 1,
    // "libSceJson", 1, 1,
    //              Json::Value::getValue(unsigned long) const);
    // LIB_FUNCTION("1sKGLPtkdow", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::getValue(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&) const);
    // LIB_FUNCTION("RQpBshscViM", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::toString(
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&)
    //                  const);
    // LIB_FUNCTION("IlsmvBtMkak", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::getObject() const);
    // LIB_FUNCTION("epJ6x2LV0kU", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::getString() const);
    // LIB_FUNCTION("a-aMMUXqrN0", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::operator bool() const);
    // LIB_FUNCTION("XlWbvieLj2M", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::operator[](unsigned long) const);
    // LIB_FUNCTION("HwDt5lD9Bfo", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::operator[](char const*) const);
    // LIB_FUNCTION("F-bQmUqmTpI", "libSceJson", 1, "libSceJson", 1, 1,
    //              Json::Value::operator[](
    //                  std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>
    //                  const&) const);
};

} // namespace Libraries::Json