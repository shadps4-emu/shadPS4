// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "playgo_chunk.h"

bool PlaygoFile::Open(const std::filesystem::path& filepath) {
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (file.IsOpen()) {
        file.Read(playgoHeader);
        if (LoadChunks(file)) {
            return true;
        }
    }
    return false;
}

bool PlaygoFile::LoadChunks(const Common::FS::IOFile& file) {
    if (file.IsOpen()) {
        if (playgoHeader.magic == PLAYGO_MAGIC) {
            bool ret = true;

            std::string chunk_attrs_data, chunk_mchunks_data, chunk_labels_data, mchunk_attrs_data;
            ret = ret && load_chunk_data(file, playgoHeader.chunk_attrs, chunk_attrs_data);
            ret = ret && load_chunk_data(file, playgoHeader.chunk_mchunks, chunk_mchunks_data);
            ret = ret && load_chunk_data(file, playgoHeader.chunk_labels, chunk_labels_data);
            ret = ret && load_chunk_data(file, playgoHeader.mchunk_attrs, mchunk_attrs_data);

            if (ret) {
                chunks.resize(playgoHeader.chunk_count);

                auto chunk_attrs =
                    reinterpret_cast<playgo_chunk_attr_entry_t*>(&chunk_attrs_data[0]);
                auto chunk_mchunks = reinterpret_cast<u16*>(&chunk_mchunks_data[0]);
                auto chunk_labels = reinterpret_cast<char*>(&chunk_labels_data[0]);
                auto mchunk_attrs =
                    reinterpret_cast<playgo_mchunk_attr_entry_t*>(&mchunk_attrs_data[0]);

                for (u16 i = 0; i < playgoHeader.chunk_count; i++) {
                    chunks[i].req_locus = chunk_attrs[i].req_locus;
                    chunks[i].language_mask = chunk_attrs[i].language_mask;
                    chunks[i].label_name = std::string(chunk_labels + chunk_attrs[i].label_offset);

                    u64 total_size = 0;
                    u16 mchunk_count = chunk_attrs[i].mchunk_count;
                    if (mchunk_count != 0) {
                        auto mchunks = reinterpret_cast<u16*>(
                            ((u8*)chunk_mchunks + chunk_attrs[i].mchunks_offset));
                        for (u16 j = 0; j < mchunk_count; j++) {
                            u16 mchunk_id = mchunks[j];
                            total_size += mchunk_attrs[mchunk_id].size.size;
                        }
                    }
                    chunks[i].total_size = total_size;
                }
            }

            return ret;
        }
    }
    return false;
}

bool PlaygoFile::load_chunk_data(const Common::FS::IOFile& file, const chunk_t chunk,
                                 std::string& data) {
    if (file.IsOpen()) {
        if (file.Seek(chunk.offset)) {
            data.resize(chunk.length);
            if (data.size() == chunk.length) {
                file.ReadRaw<char>(&data[0], chunk.length);
                return true;
            }
        }
    }
    return false;
}