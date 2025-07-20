// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Json {

s32 PS4_SYSV_ABI Json::Initializer::initialize(Json::InitParameter const*)();
s32 PS4_SYSV_ABI Json::Initializer::terminate()();
s32 PS4_SYSV_ABI Json::Initializer::Initializer()();
s32 PS4_SYSV_ABI Json::Initializer::Initializer()();
s32 PS4_SYSV_ABI Json::Initializer::~Initializer()();
s32 PS4_SYSV_ABI Json::Initializer::~Initializer()();
s32 PS4_SYSV_ABI Json::s_initparam();
s32 PS4_SYSV_ABI Json::Free(void*)();
s32 PS4_SYSV_ABI Json::Value::referArray()();
s32 PS4_SYSV_ABI Json::Value::referValue(unsigned long)();
s32 PS4_SYSV_ABI Json::Value::referValue(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&)();
s32 PS4_SYSV_ABI Json::Value::s_nullbool();
s32 PS4_SYSV_ABI Json::Value::s_nullreal();
s32 PS4_SYSV_ABI Json::Value::referObject()();
s32 PS4_SYSV_ABI Json::Value::referString()();
s32 PS4_SYSV_ABI Json::Value::s_nullarray();
s32 PS4_SYSV_ABI Json::Value::referBoolean()();
s32 PS4_SYSV_ABI Json::Value::referInteger()();
s32 PS4_SYSV_ABI Json::Value::s_nullobject();
s32 PS4_SYSV_ABI Json::Value::s_nullstring();
s32 PS4_SYSV_ABI Json::Value::referUInteger()();
s32 PS4_SYSV_ABI Json::Value::s_nullinteger();
s32 PS4_SYSV_ABI Json::Value::s_nulluinteger();
s32 PS4_SYSV_ABI
Json::Value::serialize_internal(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, void*), void*, Json::Value*)();
s32 PS4_SYSV_ABI Json::Value::setNullAccessCallBack(Json::Value const& (*)(Json::ValueType, Json::Value const*, void*), void*)();
s32 PS4_SYSV_ABI Json::Value::set(bool)();
s32 PS4_SYSV_ABI Json::Value::set(double)();
s32 PS4_SYSV_ABI Json::Value::set(long)();
s32 PS4_SYSV_ABI Json::Value::set(unsigned long)();
s32 PS4_SYSV_ABI Json::Value::set(Json::ValueType)();
s32 PS4_SYSV_ABI Json::Value::set(Json::Value const&)();
s32 PS4_SYSV_ABI Json::Value::set(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&)();
s32 PS4_SYSV_ABI
Json::Value::set(std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value, std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > >, Json::StlAlloc<std::pair<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value> > > const&)();
s32 PS4_SYSV_ABI Json::Value::set(std::list<Json::Value, Json::StlAlloc<Json::Value> > const&)();
s32 PS4_SYSV_ABI Json::Value::swap(Json::Value&)();
s32 PS4_SYSV_ABI Json::Value::clear()();
s32 PS4_SYSV_ABI Json::Value::referReal()();
s32 PS4_SYSV_ABI Json::Value::serialize(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&)();
s32 PS4_SYSV_ABI
Json::Value::serialize(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, void*), void*)();
s32 PS4_SYSV_ABI Json::Value::setParent(Json::Value const*)();
s32 PS4_SYSV_ABI Json::Value::operator=(Json::Value const&)();
s32 PS4_SYSV_ABI Json::Value::Value(bool)();
s32 PS4_SYSV_ABI Json::Value::Value(double)();
s32 PS4_SYSV_ABI Json::Value::Value(long)();
s32 PS4_SYSV_ABI Json::Value::Value(unsigned long)();
s32 PS4_SYSV_ABI Json::Value::Value(Json::ValueType)();
s32 PS4_SYSV_ABI Json::Value::Value(Json::Value const&)();
s32 PS4_SYSV_ABI Json::Value::Value(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&)();
s32 PS4_SYSV_ABI
Json::Value::Value(std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value, std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > >, Json::StlAlloc<std::pair<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value> > > const&)();
s32 PS4_SYSV_ABI Json::Value::Value(std::list<Json::Value, Json::StlAlloc<Json::Value> > const&)();
s32 PS4_SYSV_ABI Json::Value::Value()();
s32 PS4_SYSV_ABI Json::Value::Value(bool)();
s32 PS4_SYSV_ABI Json::Value::Value(double)();
s32 PS4_SYSV_ABI Json::Value::Value(long)();
s32 PS4_SYSV_ABI Json::Value::Value(unsigned long)();
s32 PS4_SYSV_ABI Json::Value::Value(Json::ValueType)();
s32 PS4_SYSV_ABI Json::Value::Value(Json::Value const&)();
s32 PS4_SYSV_ABI Json::Value::Value(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&)();
s32 PS4_SYSV_ABI
Json::Value::Value(std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value, std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > >, Json::StlAlloc<std::pair<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >, Json::Value> > > const&)();
s32 PS4_SYSV_ABI Json::Value::Value(std::list<Json::Value, Json::StlAlloc<Json::Value> > const&)();
s32 PS4_SYSV_ABI Json::Value::Value()();
s32 PS4_SYSV_ABI Json::Value::~Value()();
s32 PS4_SYSV_ABI Json::Value::~Value()();
s32 PS4_SYSV_ABI Json::Malloc(unsigned long)();
s32 PS4_SYSV_ABI Json::Parser::parseArray(Json::Value&, Json::InputStream&, Json::Value*)();
s32 PS4_SYSV_ABI Json::Parser::parseValue(Json::Value&, Json::InputStream&, Json::Value*)();
s32 PS4_SYSV_ABI Json::Parser::parseNumber(Json::Value&, Json::InputStream&, Json::Value*)();
s32 PS4_SYSV_ABI Json::Parser::parseObject(Json::Value&, Json::InputStream&, Json::Value*)();
s32 PS4_SYSV_ABI Json::Parser::parseString(Json::Value&, Json::InputStream&, Json::Value*)();
s32 PS4_SYSV_ABI
Json::Parser::parseString(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, Json::InputStream&)();
s32 PS4_SYSV_ABI Json::Parser::parseQuadHex(Json::InputStream&)();
s32 PS4_SYSV_ABI
Json::Parser::parseCodePoint(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&, Json::InputStream&)();
s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, int (*)(char&, void*), void*)();
s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, char const*)();
s32 PS4_SYSV_ABI Json::Parser::parse(Json::Value&, char const*, unsigned long)();
s32 PS4_SYSV_ABI Json::RootParam::RootParam()();
s32 PS4_SYSV_ABI Json::RootParam::RootParam()();
s32 PS4_SYSV_ABI Json::RootParam::~RootParam()();
s32 PS4_SYSV_ABI Json::RootParam::~RootParam()();
s32 PS4_SYSV_ABI Json::Value::getBoolean() const();
s32 PS4_SYSV_ABI Json::Value::getInteger() const();
s32 PS4_SYSV_ABI Json::Value::getUInteger() const();
s32 PS4_SYSV_ABI Json::Value::count() const();
s32 PS4_SYSV_ABI Json::Value::getReal() const();
s32 PS4_SYSV_ABI Json::Value::getRoot() const();
s32 PS4_SYSV_ABI Json::Value::getType() const();
s32 PS4_SYSV_ABI Json::Value::getArray() const();
s32 PS4_SYSV_ABI Json::Value::getValue(unsigned long) const();
s32 PS4_SYSV_ABI Json::Value::getValue(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&) const();
s32 PS4_SYSV_ABI Json::Value::toString(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> >&) const();
s32 PS4_SYSV_ABI Json::Value::getObject() const();
s32 PS4_SYSV_ABI Json::Value::getString() const();
s32 PS4_SYSV_ABI Json::Value::operator bool() const();
s32 PS4_SYSV_ABI Json::Value::operator[](unsigned long) const();
s32 PS4_SYSV_ABI Json::Value::operator[](char const*) const();
s32 PS4_SYSV_ABI Json::Value::operator[](std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char> > const&) const();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Json