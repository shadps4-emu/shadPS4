// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <filesystem>
#include <mutex>
#include <vector>
#include "common/io_file.h"
#include "core/libraries/playgo/playgo_types.h"

constexpr u32 PLAYGO_MAGIC = 0x6F676C70;

struct chunk_t {
    u32 offset;
    u32 length;
} __attribute__((packed));

struct PlaygoHeader {
    u32 magic;

    u16 version_major;
    u16 version_minor;
    u16 image_count;    // [0;1]
    u16 chunk_count;    // [0;1000]
    u16 mchunk_count;   // [0;8000]
    u16 scenario_count; // [0;32]

    u32 file_size;
    u16 default_scenario_id;
    u16 attrib;
    u32 sdk_version;
    u16 disc_count; // [0;2] (if equals to 0 then disc count = 1)
    u16 layer_bmp;

    u8 reserved[32];
    char content_id[128];

    chunk_t chunk_attrs; // [0;32000]
    chunk_t chunk_mchunks;
    chunk_t chunk_labels;   // [0;16000]
    chunk_t mchunk_attrs;   // [0;12800]
    chunk_t scenario_attrs; // [0;1024]
    chunk_t scenario_chunks;
    chunk_t scenario_labels;
    chunk_t inner_mchunk_attrs; // [0;12800]
} __attribute__((packed));

struct playgo_scenario_attr_entry_t {
    u8 _type;
    u8 _unk[19];
    u16 initial_chunk_count;
    u16 chunk_count;
    u32 chunks_offset; //<-scenario_chunks
    u32 label_offset;  //<-scenario_labels
} __attribute__((packed));

struct image_disc_layer_no_t {
    u8 layer_no : 2;
    u8 disc_no : 2;
    u8 image_no : 4;
} __attribute__((packed));

struct playgo_chunk_attr_entry_t {
    u8 flag;
    image_disc_layer_no_t image_disc_layer_no;
    u8 req_locus;
    u8 unk[11];
    u16 mchunk_count;
    u64 language_mask;
    u32 mchunks_offset; //<-chunk_mchunks
    u32 label_offset;   //<-chunk_labels
} __attribute__((packed));

struct playgo_chunk_loc_t {
    u64 offset : 48;
    u64 _align1 : 8;
    u64 image_no : 4;
    u64 _align2 : 4;
} __attribute__((packed));

struct playgo_chunk_size_t {
    u64 size : 48;
    u64 _align : 16;
} __attribute__((packed));

struct playgo_mchunk_attr_entry_t {
    playgo_chunk_loc_t loc;
    playgo_chunk_size_t size;
} __attribute__((packed));

struct PlaygoChunk {
    u64 req_locus;
    u64 language_mask;
    u64 total_size;
    std::string label_name;
};

class PlaygoFile {
public:
    OrbisPlayGoHandle handle = 0;
    OrbisPlayGoChunkId id = 0;
    OrbisPlayGoLocus locus = OrbisPlayGoLocus::NotDownloaded;
    OrbisPlayGoInstallSpeed speed = OrbisPlayGoInstallSpeed::Trickle;
    s64 speed_tick = 0;
    OrbisPlayGoEta eta = 0;
    OrbisPlayGoLanguageMask langMask = 0;
    std::vector<PlaygoChunk> chunks;

public:
    explicit PlaygoFile() = default;
    ~PlaygoFile() = default;

    bool Open(const std::filesystem::path& filepath);
    bool LoadChunks(const Common::FS::IOFile& file);

    PlaygoHeader& GetPlaygoHeader() {
        return playgoHeader;
    }
    std::mutex& GetSpeedMutex() {
        return speed_mutex;
    }

private:
    bool load_chunk_data(const Common::FS::IOFile& file, const chunk_t chunk, std::string& data);

private:
    PlaygoHeader playgoHeader;
    std::mutex speed_mutex;
};
