// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>

#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/config.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "save_instance.h"

constexpr auto OrbisSaveDataBlocksMin2 = 96;    // 3MiB
constexpr auto OrbisSaveDataBlocksMax = 32768;  // 1 GiB
constexpr std::string_view sce_sys = "sce_sys"; // system folder inside save

static Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

namespace fs = std::filesystem;

// clang-format off
static const std::unordered_map<std::string, std::string> default_title = {
    {"ja_JP", "セーブデータ"},
    {"en", "Saved Data"},
    {"fr", "Données sauvegardées"},
    {"es_ES", "Datos guardados"},
    {"de", "Gespeicherte Daten"},
    {"it", "Dati salvati"},
    {"nl", "Opgeslagen data"},
    {"pt_PT", "Dados guardados"},
    {"ru", "Сохраненные данные"},
    {"ko_KR", "저장 데이터"},
    {"zh_CN", "保存数据"},
    {"fi", "Tallennetut tiedot"},
    {"sv_SE", "Sparade data"},
    {"da_DK", "Gemte data"},
    {"no_NO", "Lagrede data"},
    {"pl_PL", "Zapisane dane"},
    {"pt_BR", "Dados salvos"},
    {"tr_TR", "Kayıtlı Veriler"},
};
// clang-format on

namespace Libraries::SaveData {

std::filesystem::path SaveInstance::MakeTitleSavePath(OrbisUserServiceUserId user_id,
                                                      std::string_view game_serial) {
    return Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) / std::to_string(user_id) /
           game_serial;
}

std::filesystem::path SaveInstance::MakeDirSavePath(OrbisUserServiceUserId user_id,
                                                    std::string_view game_serial,
                                                    std::string_view dir_name) {
    return Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) / std::to_string(user_id) /
           game_serial / dir_name;
}

uint64_t SaveInstance::GetMaxBlockFromSFO(const PSF& psf) {
    const auto vec = psf.GetBinary(std::string{SaveParams::SAVEDATA_BLOCKS});
    if (!vec.has_value()) {
        return OrbisSaveDataBlocksMax;
    }
    auto value = vec.value();
    return *(uint64_t*)value.data();
}

std::filesystem::path SaveInstance::GetParamSFOPath(const std::filesystem::path& dir_path) {
    return dir_path / sce_sys / "param.sfo";
}

void SaveInstance::SetupDefaultParamSFO(PSF& param_sfo, std::string dir_name,
                                        std::string game_serial) {
    std::string locale = Config::getEmulatorLanguage();
    if (!default_title.contains(locale)) {
        locale = "en";
    }

#define P(type, key, ...) param_sfo.Add##type(std::string{key}, __VA_ARGS__)
    // TODO Link with user service
    P(Binary, SaveParams::ACCOUNT_ID, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    P(String, SaveParams::MAINTITLE, default_title.at(locale));
    P(String, SaveParams::SUBTITLE, "");
    P(String, SaveParams::DETAIL, "");
    P(String, SaveParams::SAVEDATA_DIRECTORY, std::move(dir_name));
    P(Integer, SaveParams::SAVEDATA_LIST_PARAM, 0);
    P(String, SaveParams::TITLE_ID, std::move(game_serial));
    P(Binary, SaveParams::SAVEDATA_BLOCKS, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
#undef P
}

SaveInstance::SaveInstance(int slot_num, OrbisUserServiceUserId user_id, std::string _game_serial,
                           std::string_view _dir_name, int max_blocks)
    : slot_num(slot_num), user_id(user_id), game_serial(std::move(_game_serial)),
      dir_name(_dir_name),
      max_blocks(std::clamp(max_blocks, OrbisSaveDataBlocksMin2, OrbisSaveDataBlocksMax)) {
    ASSERT(slot_num >= 0 && slot_num < 16);

    save_path = MakeDirSavePath(user_id, game_serial, dir_name);

    const auto sce_sys_path = save_path / sce_sys;
    param_sfo_path = sce_sys_path / "param.sfo";
    corrupt_file_path = sce_sys_path / "corrupted";

    mount_point = "/savedata" + std::to_string(slot_num);

    this->exists = fs::exists(param_sfo_path);
    this->mounted = g_mnt->GetMount(mount_point) != nullptr;
}

SaveInstance::~SaveInstance() {
    if (mounted) {
        Umount();
    }
}
SaveInstance::SaveInstance(SaveInstance&& other) noexcept {
    if (this == &other)
        return;
    *this = std::move(other);
}

SaveInstance& SaveInstance::operator=(SaveInstance&& other) noexcept {
    if (this == &other)
        return *this;
    slot_num = other.slot_num;
    user_id = other.user_id;
    game_serial = std::move(other.game_serial);
    dir_name = std::move(other.dir_name);
    save_path = std::move(other.save_path);
    param_sfo_path = std::move(other.param_sfo_path);
    corrupt_file_path = std::move(other.corrupt_file_path);
    corrupt_file = std::move(other.corrupt_file);
    param_sfo = std::move(other.param_sfo);
    mount_point = std::move(other.mount_point);
    max_blocks = other.max_blocks;
    exists = other.exists;
    mounted = other.mounted;
    read_only = other.read_only;

    other.mounted = false;

    return *this;
}

void SaveInstance::SetupAndMount(bool read_only, bool copy_icon, bool ignore_corrupt) {
    if (mounted) {
        UNREACHABLE_MSG("Save instance is already mounted");
    }
    this->exists = fs::exists(param_sfo_path); // check again just in case
    if (!exists) {
        CreateFiles();
        if (copy_icon) {
            const auto& src_icon = g_mnt->GetHostPath("/app0/sce_sys/save_data.png");
            if (fs::exists(src_icon)) {
                auto output_icon = GetIconPath();
                if (fs::exists(output_icon)) {
                    fs::remove(output_icon);
                }
                fs::copy_file(src_icon, output_icon);
            }
        }
        exists = true;
    } else {
        if (!ignore_corrupt && fs::exists(corrupt_file_path)) {
            throw std::filesystem::filesystem_error(
                "Corrupted save data", corrupt_file_path,
                std::make_error_code(std::errc::illegal_byte_sequence));
        }
        if (!param_sfo.Open(param_sfo_path)) {
            throw std::filesystem::filesystem_error(
                "Failed to read param.sfo", param_sfo_path,
                std::make_error_code(std::errc::illegal_byte_sequence));
        }
    }

    if (!ignore_corrupt && !read_only) {
        int err = corrupt_file.Open(corrupt_file_path, Common::FS::FileAccessMode::Write);
        if (err != 0) {
            throw std::filesystem::filesystem_error(
                "Failed to open corrupted file", corrupt_file_path,
                std::make_error_code(std::errc::illegal_byte_sequence));
        }
    }

    max_blocks = static_cast<int>(GetMaxBlockFromSFO(param_sfo));

    g_mnt->Mount(save_path, mount_point, read_only);
    mounted = true;
    this->read_only = read_only;
}

void SaveInstance::Umount() {
    if (!mounted) {
        UNREACHABLE_MSG("Save instance is not mounted");
        return;
    }
    mounted = false;
    const bool ok = param_sfo.Encode(param_sfo_path);
    if (!ok) {
        throw std::filesystem::filesystem_error("Failed to write param.sfo", param_sfo_path,
                                                std::make_error_code(std::errc::permission_denied));
    }
    param_sfo = PSF();

    corrupt_file.Close();
    fs::remove(corrupt_file_path);
    g_mnt->Unmount(save_path, mount_point);
}

void SaveInstance::CreateFiles() {
    const auto sce_sys_dir = save_path / sce_sys;
    fs::create_directories(sce_sys_dir);

    SetupDefaultParamSFO(param_sfo, dir_name, game_serial);
    param_sfo.AddBinary(std::string{SaveParams::SAVEDATA_BLOCKS}, max_blocks, true);

    const bool ok = param_sfo.Encode(param_sfo_path);
    if (!ok) {
        throw std::filesystem::filesystem_error("Failed to write param.sfo", param_sfo_path,
                                                std::make_error_code(std::errc::permission_denied));
    }
}

} // namespace Libraries::SaveData