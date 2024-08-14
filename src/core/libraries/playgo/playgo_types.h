// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

typedef u32 OrbisPlayGoHandle;
typedef u16 OrbisPlayGoChunkId;
typedef s8 OrbisPlayGoLocus;
typedef s32 OrbisPlayGoInstallSpeed;
typedef s64 OrbisPlayGoEta;
typedef u64 OrbisPlayGoLanguageMask;

typedef struct OrbisPlayGoInitParams {
    const void* bufAddr;
    u32 bufSize;
    u32 reserved;
} OrbisPlayGoInitParams;

typedef struct OrbisPlayGoToDo {
    OrbisPlayGoChunkId chunkId;
    OrbisPlayGoLocus locus;
    s8 reserved;
} OrbisPlayGoToDo;

typedef struct OrbisPlayGoProgress {
    uint64_t progressSize;
    uint64_t totalSize;
} OrbisPlayGoProgress;

typedef enum OrbisPlayGoLocusValue {
    ORBIS_PLAYGO_LOCUS_NOT_DOWNLOADED = 0,
    ORBIS_PLAYGO_LOCUS_LOCAL_SLOW = 2,
    ORBIS_PLAYGO_LOCUS_LOCAL_FAST = 3
} OrbisPlayGoLocusValue;

typedef enum OrbisPlayGoInstallSpeedValue {
    ORBIS_PLAYGO_INSTALL_SPEED_SUSPENDED = 0,
    ORBIS_PLAYGO_INSTALL_SPEED_TRICKLE = 1,
    ORBIS_PLAYGO_INSTALL_SPEED_FULL = 2
} OrbisPlayGoInstallSpeedValue;