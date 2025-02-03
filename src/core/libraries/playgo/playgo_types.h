// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

using OrbisPlayGoHandle = u32;
using OrbisPlayGoChunkId = u16;
using OrbisPlayGoEta = s64;
using OrbisPlayGoLanguageMask = u64;

enum class OrbisPlayGoLocus : s8 {
    NotDownloaded = 0,
    LocalSlow = 2,
    LocalFast = 3,
};

enum class OrbisPlayGoInstallSpeed : s32 {
    Suspended = 0,
    Trickle = 1,
    Full = 2,
};

struct OrbisPlayGoInitParams {
    const void* bufAddr;
    u32 bufSize;
    u32 reserved;
};

struct OrbisPlayGoToDo {
    OrbisPlayGoChunkId chunkId;
    OrbisPlayGoLocus locus;
    s8 reserved;
};

struct OrbisPlayGoProgress {
    u64 progressSize;
    u64 totalSize;
};

constexpr int ORBIS_PLAYGO_ERROR_UNKNOWN = -2135818239;             /* 0x80B20001 */
constexpr int ORBIS_PLAYGO_ERROR_FATAL = -2135818238;               /* 0x80B20002 */
constexpr int ORBIS_PLAYGO_ERROR_NO_MEMORY = -2135818237;           /* 0x80B20003 */
constexpr int ORBIS_PLAYGO_ERROR_INVALID_ARGUMENT = -2135818236;    /* 0x80B20004 */
constexpr int ORBIS_PLAYGO_ERROR_NOT_INITIALIZED = -2135818235;     /* 0x80B20005 */
constexpr int ORBIS_PLAYGO_ERROR_ALREADY_INITIALIZED = -2135818234; /* 0x80B20006 */
constexpr int ORBIS_PLAYGO_ERROR_ALREADY_STARTED = -2135818233;     /* 0x80B20007 */
constexpr int ORBIS_PLAYGO_ERROR_NOT_STARTED = -2135818232;         /* 0x80B20008 */
constexpr int ORBIS_PLAYGO_ERROR_BAD_HANDLE = -2135818231;          /* 0x80B20009 */
constexpr int ORBIS_PLAYGO_ERROR_BAD_POINTER = -2135818230;         /* 0x80B2000A */
constexpr int ORBIS_PLAYGO_ERROR_BAD_SIZE = -2135818229;            /* 0x80B2000B */
constexpr int ORBIS_PLAYGO_ERROR_BAD_CHUNK_ID = -2135818228;        /* 0x80B2000C */
constexpr int ORBIS_PLAYGO_ERROR_BAD_SPEED = -2135818227;           /* 0x80B2000D */
constexpr int ORBIS_PLAYGO_ERROR_NOT_SUPPORT_PLAYGO = -2135818226;  /* 0x80B2000E */
constexpr int ORBIS_PLAYGO_ERROR_EPERM = -2135818225;               /* 0x80B2000F */
constexpr int ORBIS_PLAYGO_ERROR_BAD_LOCUS = -2135818224;           /* 0x80B20010 */
constexpr int ORBIS_PLAYGO_ERROR_NEED_DATA_DISC = -2135818223;      /* 0x80B20011 */
