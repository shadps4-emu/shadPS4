// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_instance.h"

#include <csetjmp>
#include <cstring>
#include <system_error>
#include <fmt/format.h>
#include <png.h>

#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "common/logging/log.h"
#include "common/singleton.h"

namespace fs = std::filesystem;

namespace Libraries::SaveData {

namespace {

constexpr std::string_view SceSysDir = "sce_sys";
constexpr std::string_view ParamSfoFilename = "param.sfo";
constexpr std::string_view IconFilename = "icon0.png";
constexpr size_t ParamsBinarySize = 1024;
constexpr u64 BlockSizeBytes = 32768;

struct PngMemReader {
    const u8* data = nullptr;
    size_t size = 0;
    size_t offset = 0;
};

void ReadPngFromMemory(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count) {
    auto* reader = static_cast<PngMemReader*>(png_get_io_ptr(png_ptr));
    if (reader == nullptr || reader->offset + byte_count > reader->size) {
        png_error(png_ptr, "Unexpected EOF while reading PNG header");
        return;
    }
    std::memcpy(out_bytes, reader->data + reader->offset, byte_count);
    reader->offset += byte_count;
}

void PngSilentError(png_structp png_ptr, png_const_charp) {
    png_longjmp(png_ptr, 1);
}

void PngSilentWarning(png_structp, png_const_charp) {}

} // namespace

std::string TruncateUtf8(std::string_view str, size_t max_bytes) {
    if (str.size() <= max_bytes) {
        return std::string{str};
    }
    size_t len = max_bytes;
    while (len > 0 && (static_cast<u8>(str[len]) & 0xC0u) == 0x80u) {
        --len;
    }
    return std::string{str.substr(0, len)};
}

bool ValidateIconPng(std::span<const u8> png_data) {
    if (png_data.size() < 8 || png_data.size() > SaveInstance::IconMaxBytes) {
        return false;
    }
    if (png_sig_cmp(png_data.data(), 0, 8) != 0) {
        return false;
    }

    png_structp png_ptr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, PngSilentError, PngSilentWarning);
    if (png_ptr == nullptr) {
        return false;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return false;
    }

    PngMemReader reader{
        .data = png_data.data(),
        .size = png_data.size(),
        .offset = 0,
    };
    png_set_read_fn(png_ptr, &reader, ReadPngFromMemory);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int bit_depth = 0;
    int color_type = 0;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr,
                 nullptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    // Icon validation: 228x128, 8-bit RGB (color_space == 3)
    return width == SaveInstance::IconWidth && height == SaveInstance::IconHeight &&
           bit_depth == 8 && color_type == PNG_COLOR_TYPE_RGB;
}

fs::path SaveInstance::MakeDirSavePath(Libraries::UserService::OrbisUserServiceUserId user_id,
                                       std::string_view game_serial, std::string_view dir_name) {
    return GetSaveDirPath(user_id, game_serial, dir_name);
}

fs::path SaveInstance::GetParamSFOPath(const fs::path& save_path) {
    return save_path / SceSysDir / ParamSfoFilename;
}

fs::path SaveInstance::GetIconPath(const fs::path& save_path) {
    return save_path / SceSysDir / IconFilename;
}

void SaveInstance::SetupDefaultParamSFO(PSF& param_sfo, std::string_view dir_name,
                                        std::string_view title_id,
                                        Libraries::UserService::OrbisUserServiceUserId user_id,
                                        u64 blocks, bool update) {
    const std::string clean_dir = TruncateUtf8(dir_name, 31);
    const std::string clean_tid = TruncateUtf8(title_id, 11);

    std::vector<u8> params_blob(ParamsBinarySize, 0);
    const u32 uid_le = static_cast<u32>(user_id);
    std::memcpy(params_blob.data() + 4, &uid_le, sizeof(u32));

    param_sfo.AddBinary("ACCOUNT_ID", u64{0}, update);
    param_sfo.AddInteger("ATTRIBUTE", 0, update);
    param_sfo.AddString("CATEGORY", "sd", update);
    param_sfo.AddString("DETAIL", "", update);
    param_sfo.AddString("FORMAT", "obs", update);
    param_sfo.AddString("MAINTITLE", "", update);
    param_sfo.AddBinary("PARAMS", std::move(params_blob), update);
    param_sfo.AddBinary("SAVEDATA_BLOCKS", blocks, update);
    param_sfo.AddString("SAVEDATA_DIRECTORY", clean_dir, update);
    param_sfo.AddInteger("SAVEDATA_LIST_PARAM", 0, update);
    param_sfo.AddString("SUBTITLE", "", update);
    param_sfo.AddString("TITLE_ID", clean_tid, update);
}

bool SaveInstance::WriteParamSFOAtomic(const PSF& sfo, const fs::path& sfo_path) {
    std::error_code ec;
    fs::create_directories(sfo_path.parent_path(), ec);
    if (ec) {
        return false;
    }
    const fs::path tmp_path = fs::path(sfo_path).concat(".tmp");
    if (!sfo.Encode(tmp_path)) {
        fs::remove(tmp_path, ec);
        return false;
    }
    fs::rename(tmp_path, sfo_path, ec);
    if (ec) {
        fs::remove(tmp_path, ec);
        return false;
    }
    return true;
}

SaveDbEntry SaveInstance::BuildDbEntryFromSfo(
    const PSF& sfo, Libraries::UserService::OrbisUserServiceUserId user_id,
    std::string_view title_id, std::string_view game_title_id, std::string_view dir_name,
    const fs::path& save_path, u64 default_blocks, bool is_broken, bool fake_broken) {
    SaveDbEntry entry;
    entry.title_id = std::string(title_id);
    entry.dir_name = std::string(dir_name);
    entry.game_title_id = std::string(game_title_id.empty() ? title_id : game_title_id);
    entry.main_title = std::string(sfo.GetString("MAINTITLE").value_or(""));
    entry.sub_title = std::string(sfo.GetString("SUBTITLE").value_or(""));
    entry.detail = std::string(sfo.GetString("DETAIL").value_or(""));
    entry.user_param = static_cast<u32>(sfo.GetInteger("SAVEDATA_LIST_PARAM").value_or(0));

    u64 blocks = default_blocks;
    if (const auto blocks_bin = sfo.GetBinary("SAVEDATA_BLOCKS");
        blocks_bin && blocks_bin->size() >= sizeof(u64)) {
        u64 sfo_blocks = 0;
        std::memcpy(&sfo_blocks, blocks_bin->data(), sizeof(u64));
        if (sfo_blocks != 0) {
            blocks = sfo_blocks;
        }
    }
    if (blocks == 0) {
        blocks = 96;
    }
    entry.blocks = blocks;
    entry.size_kib = blocks * 32ULL;
    entry.free_blocks = ComputeFreeBlocks(save_path, blocks);

    if (const auto acct_bin = sfo.GetBinary("ACCOUNT_ID");
        acct_bin && acct_bin->size() >= sizeof(u64)) {
        std::memcpy(&entry.account_id, acct_bin->data(), sizeof(u64));
    }
    entry.user_id = static_cast<u32>(user_id);
    if (const auto params_bin = sfo.GetBinary("PARAMS"); params_bin && params_bin->size() >= 8) {
        u32 uid = 0;
        std::memcpy(&uid, params_bin->data() + 4, sizeof(u32));
        if (uid != 0) {
            entry.user_id = uid;
        }
    }

    entry.mtime = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    entry.is_broken = is_broken;
    entry.fake_broken = fake_broken;
    return entry;
}

SaveInstance::SaveInstance(int slot_num_, Libraries::UserService::OrbisUserServiceUserId user_id_,
                           std::string save_title_id_, std::string game_title_id_,
                           std::string dir_name_, u64 max_blocks_, bool read_only_)
    : slot_num{slot_num_}, user_id{user_id_}, title_id{std::move(save_title_id_)},
      game_title_id{std::move(game_title_id_)}, dir_name{std::move(dir_name_)},
      max_blocks{max_blocks_ == 0 ? 96 : max_blocks_}, read_only{read_only_} {
    save_path = GetSaveDirPath(user_id, title_id, dir_name);
    param_sfo_path = GetParamSFOPath(save_path);
    icon_path = GetIconPath(save_path);
    exists = fs::exists(save_path);

    if (const auto db_entry = SaveDb::GetEntry(user_id, title_id, dir_name)) {
        fake_broken = db_entry->fake_broken;
    }

    if (exists && fs::exists(param_sfo_path)) {
        if (!param_sfo.Open(param_sfo_path)) {
            SetupDefaultParamSFO(param_sfo, dir_name, title_id, user_id, max_blocks);
            sfo_dirty = true;
        } else if (const auto blocks_bin = param_sfo.GetBinary("SAVEDATA_BLOCKS");
                   blocks_bin && blocks_bin->size() >= sizeof(u64)) {
            u64 stored_blocks = 0;
            std::memcpy(&stored_blocks, blocks_bin->data(), sizeof(u64));
            if (stored_blocks != 0) {
                max_blocks = stored_blocks;
            }
        }
    } else {
        SetupDefaultParamSFO(param_sfo, dir_name, title_id, user_id, max_blocks);
        sfo_dirty = true;
    }
}

SaveInstance::~SaveInstance() {
    if (mounted) {
        Umount();
    }
}

bool SaveInstance::SyncMetaIcon(std::span<const u8> icon_bytes) const {
    const fs::path meta_icon = GetMetaIconPath(user_id, title_id, dir_name);
    std::error_code ec;
    fs::create_directories(meta_icon.parent_path(), ec);
    if (ec) {
        return false;
    }

    if (!icon_bytes.empty()) {
        const fs::path tmp_path = fs::path(meta_icon).concat(".tmp");
        Common::FS::IOFile dst{tmp_path, Common::FS::FileAccessMode::Create};
        if (!dst.IsOpen()) {
            return false;
        }
        const size_t written = dst.WriteRaw<u8>(icon_bytes.data(), icon_bytes.size());
        dst.Close();
        if (written != icon_bytes.size()) {
            fs::remove(tmp_path, ec);
            return false;
        }
        fs::rename(tmp_path, meta_icon, ec);
        if (ec) {
            fs::remove(tmp_path, ec);
            return false;
        }
        return true;
    }

    if (fs::exists(icon_path, ec)) {
        fs::copy_file(icon_path, meta_icon, fs::copy_options::overwrite_existing, ec);
        return !ec;
    }
    return true;
}

bool SaveInstance::CreateFiles() {
    if (!exists) {
        SaveDbEntry creating;
        creating.title_id = title_id;
        creating.dir_name = dir_name;
        creating.game_title_id = game_title_id;
        creating.user_id = static_cast<u32>(user_id);
        creating.blocks = max_blocks;
        creating.free_blocks = max_blocks;
        creating.is_broken = true;
        SaveDb::UpsertCreating(user_id, creating);
    }

    std::error_code ec;
    fs::create_directories(save_path / SceSysDir, ec);
    if (ec) {
        LOG_ERROR(Lib_SaveData, "Failed to create save container {}: {}",
                  fmt::UTF(save_path.u8string()), ec.message());
        return false;
    }

    if (!fs::exists(param_sfo_path, ec) || sfo_dirty) {
        if (!SaveParamSFO(true)) {
            return false;
        }
    } else {
        SaveDb::Finalize(user_id,
                         BuildDbEntryFromSfo(param_sfo, user_id, title_id, game_title_id, dir_name,
                                             save_path, max_blocks, false, fake_broken));
    }

    SyncMetaIcon();
    exists = true;
    return true;
}

bool SaveInstance::SaveParamSFO(bool finalize_db) {
    if (!WriteParamSFOAtomic(param_sfo, param_sfo_path)) {
        LOG_ERROR(Lib_SaveData, "Failed to write param.sfo at {}",
                  fmt::UTF(param_sfo_path.u8string()));
        return false;
    }
    sfo_dirty = false;
    if (finalize_db) {
        SaveDb::Finalize(user_id,
                         BuildDbEntryFromSfo(param_sfo, user_id, title_id, game_title_id, dir_name,
                                             save_path, max_blocks, false, fake_broken));
    }
    return true;
}

bool SaveInstance::Mount() {
    if (mounted) {
        return true;
    }
    if (!exists) {
        if (read_only || !CreateFiles()) {
            return false;
        }
    } else if (!read_only) {
        // Ensure sce_sys and DB are synced on RW mount
        std::error_code ec;
        fs::create_directories(save_path / SceSysDir, ec);
        if (!fs::exists(param_sfo_path, ec)) {
            if (!SaveParamSFO(true)) {
                return false;
            }
        }
        SyncMetaIcon();
    }

    mount_point = fmt::format("/savedata{}", slot_num);
    mnt->Mount(save_path, mount_point, read_only);
    mounted = true;
    return true;
}

bool SaveInstance::Umount() {
    if (!mounted) {
        return true;
    }
    if (!read_only) {
        if (sfo_dirty || !fs::exists(param_sfo_path)) {
            SaveParamSFO(true);
        } else {
            SaveDb::Finalize(user_id, BuildDbEntryFromSfo(param_sfo, user_id, title_id,
                                                          game_title_id, dir_name, save_path,
                                                          max_blocks, false, fake_broken));
        }
        SyncMetaIcon();
    }
    if (!mnt->Unmount(mount_point)) {
        LOG_WARNING(Lib_SaveData, "Unmount {} refused (EBUSY: open handles remain)", mount_point);
        return false;
    }
    mounted = false;
    mount_point.clear();
    return true;
}

} // namespace Libraries::SaveData
