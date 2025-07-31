// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <list>
#include <map>
#include <string>

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Json { // sce::Json

s32 PS4_SYSV_ABI s_initparam();
s32 PS4_SYSV_ABI Malloc(unsigned long);
s32 PS4_SYSV_ABI Free(void*);

struct InitParameter {};

template <typename T>
class StlAlloc {};

struct ValueType {};

struct InputStream {};

class Initializer {
public:
    PS4_SYSV_ABI Initializer();
    PS4_SYSV_ABI ~Initializer();
    s32 PS4_SYSV_ABI initialize();
    s32 PS4_SYSV_ABI initialize(Json::InitParameter const*);
    s32 PS4_SYSV_ABI terminate();
};

class Value {
public:
    Value();
    Value(bool);
    Value(double);
    Value(long);
    Value(unsigned long);
    Value(Json::ValueType);
    Value(Json::Value const&);
    Value(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&);
    Value(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&);
    Value(
        std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>, Json::Value,
                 std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>>,
                 Json::StlAlloc<std::pair<
                     std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
                     Json::Value>>> const&);
    ~Value();

    s32 referValue(unsigned long);
    s32 referValue(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&);

    s32 referObject();
    s32 referString();
    s32 referBoolean();
    s32 referInteger();
    s32 referUInteger();
    s32 referArray();
    s32 referReal();

    s32 s_nullbool;
    s32 s_nullreal;
    s32 s_nullarray;
    s32 s_nullobject;
    s32 s_nullinteger;
    s32 s_nulluinteger;
    s32 s_nullstring;

    s32 serialize_internal(
        std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
        int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&, void*),
        void*, Json::Value*);
    s32 serialize(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&);
    s32 serialize(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
                  int (*)(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
                          void*),
                  void*);

    s32 setNullAccessCallBack(Json::Value const& (*)(Json::ValueType, Json::Value const*, void*),
                              void*);

    s32 set(bool);
    s32 set(double);
    s32 set(long);
    s32 set(unsigned long);
    s32 set(Json::ValueType);
    s32 set(Json::Value const&);
    s32 set(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&);
    s32 set(
        std::map<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>, Json::Value,
                 std::less<std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>>,
                 Json::StlAlloc<std::pair<
                     std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>,
                     Json::Value>>> const&);
    s32 set(std::list<Json::Value, Json::StlAlloc<Json::Value>> const&);

    s32 swap(Json::Value&);
    s32 clear();

    s32 setParent(Json::Value const*);
    s32 operator=(Json::Value const&);

    s32 getBoolean() const;
    s32 getInteger() const;
    s32 getUInteger() const;
    s32 count() const;
    s32 getReal() const;
    s32 getRoot() const;
    s32 getType() const;
    s32 getArray() const;
    s32 getValue(unsigned long) const;
    s32 getValue(
        std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) const;
    s32 toString(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&) const;
    s32 getObject() const;
    s32 getString() const;
    operator bool() const;
    s32 operator[](unsigned long) const;
    s32 operator[](char const*) const;
    s32 operator[](
        std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>> const&) const;
};

class Parser {
public:
    s32 parseArray(Json::Value&, Json::InputStream&, Json::Value*);
    s32 parseValue(Json::Value&, Json::InputStream&, Json::Value*);
    s32 parseNumber(Json::Value&, Json::InputStream&, Json::Value*);
    s32 parseObject(Json::Value&, Json::InputStream&, Json::Value*);
    s32 parseString(Json::Value&, Json::InputStream&, Json::Value*);
    s32 parseString(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
                    Json::InputStream&);
    s32 parseQuadHex(Json::InputStream&);
    s32 parseCodePoint(std::basic_string<char, std::char_traits<char>, Json::StlAlloc<char>>&,
                       Json::InputStream&);
    s32 parse(Json::Value&, int (*)(char&, void*), void*);
    s32 parse(Json::Value&, char const*);
    s32 parse(Json::Value&, char const*, unsigned long);
};

class RootParam {
public:
    RootParam();
    ~RootParam();
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Json