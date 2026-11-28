// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audiodec {

enum class OrbisAudiodecCodecType : u32 {
    At9 = 1,
    Mp3 = 2,
    Aac = 3,
};

struct OrbisAudiodecAuInfo {
    u32 size;
    u32 reserved0;
    u8* buf;
    u32 buf_size;
    u32 reserved1;
};
static_assert(sizeof(OrbisAudiodecAuInfo) == 0x18);

struct OrbisAudiodecPcmItem {
    u32 size;
    u32 reserved0;
    u8* buf;
    u32 buf_size;
    u32 reserved1;
};
static_assert(sizeof(OrbisAudiodecPcmItem) == 0x18);

struct OrbisAudiodecCtrl {
    void* param;
    void* bsi_info;
    OrbisAudiodecAuInfo* au_info;
    OrbisAudiodecPcmItem* pcm_item;
};
static_assert(sizeof(OrbisAudiodecCtrl) == 0x20);

struct OrbisAudiodecMp3Param {
    u32 size;
    u32 version;
};
static_assert(sizeof(OrbisAudiodecMp3Param) == 0x8);

struct OrbisAudiodecMp3BsiInfo {
    u32 size;
    u32 header;
    u8 has_crc;
    u8 channel_mode;
    u8 mode_extension;
    u8 copyright;
    u8 original;
    u8 emphasis;
    u16 reserved;
    s32 result;
};
static_assert(sizeof(OrbisAudiodecMp3BsiInfo) == 0x14);

struct OrbisAudiodecAacParam {
    u32 size;
    u32 sample_encoding;
    u32 config_type;
    u32 sampling_freq_type;
    u32 num_channels;
    u32 enable_sbr;
    u32 enable_nondelay_output;
    u32 surround_channel_order;
};
static_assert(sizeof(OrbisAudiodecAacParam) == 0x20);

struct OrbisAudiodecAacBsiInfo {
    u32 size;
    u32 sample_rate;
    u32 num_channels;
    u32 heaac;
    s32 result;
};
static_assert(sizeof(OrbisAudiodecAacBsiInfo) == 0x14);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Audiodec
