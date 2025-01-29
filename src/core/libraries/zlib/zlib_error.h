// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/error_codes.h"

// Zlib library
constexpr int ORBIS_ZLIB_ERROR_NOT_FOUND = 0x81120002;
constexpr int ORBIS_ZLIB_ERROR_BUSY = 0x8112000B;
constexpr int ORBIS_ZLIB_ERROR_FAULT = 0x8112000E;
constexpr int ORBIS_ZLIB_ERROR_INVALID = 0x81120016;
constexpr int ORBIS_ZLIB_ERROR_NOSPACE = 0x8112001C;
constexpr int ORBIS_ZLIB_ERROR_NOT_SUPPORTED = 0x81120025;
constexpr int ORBIS_ZLIB_ERROR_TIMEDOUT = 0x81120027;
constexpr int ORBIS_ZLIB_ERROR_NOT_INITIALIZED = 0x81120032;
constexpr int ORBIS_ZLIB_ERROR_ALREADY_INITIALIZED = 0x81120033;
constexpr int ORBIS_ZLIB_ERROR_FATAL = 0x811200FF;
