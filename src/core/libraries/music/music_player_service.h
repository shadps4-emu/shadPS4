// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::MusicPlayerService {

// Error codes for module 0x12f (libSceMusicPlayerService)
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_NOT_INITIALIZED = static_cast<s32>(0x812f0001);
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_ALREADY_INITIALIZED = static_cast<s32>(0x812f0002);
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_INVALID_PARAMETER = static_cast<s32>(0x812f0004);
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_ALREADY_IN_TRANSACTION =
    static_cast<s32>(0x812f0101);
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_NOT_IN_TRANSACTION = static_cast<s32>(0x812f0102);
constexpr s32 ORBIS_MUSIC_PLAYER_SERVICE_ERROR_TIMED_OUT = static_cast<s32>(0x812f010a);

struct OrbisMusicPlayerPlayStatusExtension {
    u64 size;              // +0x00 (0x48)
    u32 error_code;        // +0x08 (0: ok, non-zero: error)
    u32 play_state;        // +0x0c (0: playing, 1: paused, 2: stopped)
    u64 total_duration_s;  // +0x10 (total duration in seconds)
    u64 current_time_s;    // +0x18 (position in seconds)
    u32 tracklist_id;      // +0x20
    u32 track_index;       // +0x24 (0-based current track index)
    u32 play_index;        // +0x28 (0-based current playback sequence index)
    u32 total_track_count; // +0x2c (total tracks in tracklist)
    u32 volume;            // +0x30 (0 - 25)
    u32 repeat_mode;       // +0x34 (0: off, 1: all, 2: one)
    u32 shuffle_mode;      // +0x38 (0: off, 1: on)
    u32 can_skip_next;     // +0x3c (0: can skip next, 1: cannot)
    u32 can_skip_previous; // +0x40 (0: can skip previous, 1: cannot)
    u32 is_loading;        // +0x44 (0: not loading)
};
static_assert(sizeof(OrbisMusicPlayerPlayStatusExtension) == 0x48);

struct SceMusicPlayerServicePlayStatus {
    u32 result;        // +0x00 (0: ok)
    u32 play_status;   // +0x04 (0: stopped, 1: playing, 2: paused)
    u32 track_index;   // +0x08 (0-based current track index)
    u32 tracklist_id;  // +0x0c
    u64 playback_time; // +0x10 (position in seconds)
    u32 shuffle;       // +0x18 (0: off, 1: on)
    u32 repeat;        // +0x1c (0: off, 1: all, 2: one)
};
static_assert(sizeof(SceMusicPlayerServicePlayStatus) == 0x20);

struct OrbisMusicPlayerUsbDeviceInfo {
    u32 device_id;           // +0x00
    char serial_number[256]; // +0x04
    u8 pad0[4];              // +0x104
    u64 serial_number_len;   // +0x108
};
static_assert(offsetof(OrbisMusicPlayerUsbDeviceInfo, serial_number) == 0x04);
static_assert(offsetof(OrbisMusicPlayerUsbDeviceInfo, serial_number_len) == 0x108);
static_assert(sizeof(OrbisMusicPlayerUsbDeviceInfo) == 0x110);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

bool IsBgmPlaying();

} // namespace Libraries::MusicPlayerService
