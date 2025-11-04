// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>

#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/config.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "save_backup.h"
#include "save_instance.h"

constexpr auto OrbisSaveDataBlocksMin2 = 96;    // 3MiB
constexpr auto OrbisSaveDataBlocksMax = 32768;  // 1 GiB
constexpr std::string_view sce_sys = "sce_sys"; // system folder inside save

static Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

namespace fs = std::filesystem;

// clang-format off
static const std::unordered_map<int, std::string> default_title = {
    {0/*"ja_JP"*/, "セーブデータ"},
    {1/*"en_US"*/, "Saved Data"},
    {2/*"fr_FR"*/, "Données sauvegardées"},
    {3/*"es_ES"*/, "Datos guardados"},
    {4/*"de_DE"*/, "Gespeicherte Daten"},
    {5/*"it_IT"*/, "Dati salvati"},
    {6/*"nl_NL"*/, "Opgeslagen data"},
    {7/*"pt_PT"*/, "Dados guardados"},
    {8/*"ru_RU"*/, "Сохраненные данные"},
    {9/*"ko_KR"*/, "저장 데이터"},
    {10/*"zh_CN"*/, "保存数据"},
    {12/*"fi_FI"*/, "Tallennetut tiedot"},
    {13/*"sv_SE"*/, "Sparade data"},
    {14/*"da_DK"*/, "Gemte data"},
    {15/*"no_NO"*/, "Lagrede data"},
    {16/*"pl_PL"*/, "Zapisane dane"},
    {17/*"pt_BR"*/, "Dados salvos"},
    {19/*"tr_TR"*/, "Kayıtlı Veriler"},
};
// clang-format on

namespace Libraries::SaveData {

fs::path SaveInstance::MakeTitleSavePath(OrbisUserServiceUserId user_id,
                                         std::string_view game_serial) {
    return Config::GetSaveDataPath() / std::to_string(user_id) / game_serial;
}

fs::path SaveInstance::MakeDirSavePath(OrbisUserServiceUserId user_id, std::string_view game_serial,
                                       std::string_view dir_name) {
    return Config::GetSaveDataPath() / std::to_string(user_id) / game_serial / dir_name;
}

uint64_t SaveInstance::GetMaxBlockFromSFO(const PSF& psf) {
    const auto vec = psf.GetBinary(std::string{SaveParams::SAVEDATA_BLOCKS});
    if (!vec.has_value()) {
        return OrbisSaveDataBlocksMax;
    }
    auto value = vec.value();
    return *(uint64_t*)value.data();
}

fs::path SaveInstance::GetParamSFOPath(const fs::path& dir_path) {
    return dir_path / sce_sys / "param.sfo";
}

void SaveInstance::SetupDefaultParamSFO(PSF& param_sfo, std::string dir_name,
                                        std::string game_serial) {
    int locale = Config::GetLanguage();
    if (!default_title.contains(locale)) {
        locale = 1; // default to en_US if not found
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
    param_sfo = std::move(other.param_sfo);
    mount_point = std::move(other.mount_point);
    max_blocks = other.max_blocks;
    exists = other.exists;
    mounted = other.mounted;
    read_only = other.read_only;

    other.mounted = false;

    return *this;
}

void SaveInstance::SetupAndMount(bool read_only, bool copy_icon, bool ignore_corrupt,
                                 bool dont_restore_backup) {
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
        std::optional<fs::filesystem_error> err;
        if (!ignore_corrupt && fs::exists(corrupt_file_path)) {
            err = fs::filesystem_error("Corrupted save data", corrupt_file_path,
                                       std::make_error_code(std::errc::illegal_byte_sequence));
        } else if (!param_sfo.Open(param_sfo_path)) {
            err = fs::filesystem_error("Failed to read param.sfo", param_sfo_path,
                                       std::make_error_code(std::errc::illegal_byte_sequence));
        }
        if (err.has_value()) {
            if (dont_restore_backup) {
                throw err.value();
            }
            if (Backup::Restore(save_path)) {
                return SetupAndMount(read_only, copy_icon, ignore_corrupt, true);
            }
        }
    }

    if (!ignore_corrupt && !read_only) {
        Common::FS::IOFile f(corrupt_file_path, Common::FS::FileAccessMode::Create);
        f.Close();
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
        throw fs::filesystem_error("Failed to write param.sfo", param_sfo_path,
                                   std::make_error_code(std::errc::permission_denied));
    }
    param_sfo = PSF();

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
        throw fs::filesystem_error("Failed to write param.sfo", param_sfo_path,
                                   std::make_error_code(std::errc::permission_denied));
    }
}

} // namespace Libraries::SaveData
