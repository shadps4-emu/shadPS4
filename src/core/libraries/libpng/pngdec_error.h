// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/error_codes.h"

// PngDec library
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_ADDR = 0x80690001;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_SIZE = 0x80690002;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_PARAM = 0x80690003;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_HANDLE = 0x80690004;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_WORK_MEMORY = 0x80690005;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_DATA = 0x80690010;
constexpr int ORBIS_PNG_DEC_ERROR_UNSUPPORT_DATA = 0x80690011;
constexpr int ORBIS_PNG_DEC_ERROR_DECODE_ERROR = 0x80690012;
constexpr int ORBIS_PNG_DEC_ERROR_FATAL = 0x80690020;
