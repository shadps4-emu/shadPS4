// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/aes.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/file_format/trp.h"

static void DecryptEFSM(std::span<u8, 16> trophyKey, std::span<u8, 16> NPcommID,
                        std::span<u8, 16> efsmIv, std::span<u8> ciphertext,
                        std::span<u8> decrypted) {
    // Step 1: Encrypt NPcommID
    std::array<u8, 16> trophyIv{};
    std::array<u8, 16> trpKey;
    aes::encrypt_cbc(NPcommID.data(), NPcommID.size(), trophyKey.data(), trophyKey.size(),
                     trophyIv.data(), trpKey.data(), trpKey.size(), false);

    // Step 2: Decrypt EFSM
    aes::decrypt_cbc(ciphertext.data(), ciphertext.size(), trpKey.data(), trpKey.size(),
                     efsmIv.data(), decrypted.data(), decrypted.size(), nullptr);
}

TRP::TRP() = default;
TRP::~TRP() = default;

void TRP::GetNPcommID(const std::filesystem::path& trophyPath, int index) {
    std::filesystem::path trpPath = trophyPath / "sce_sys/npbind.dat";
    Common::FS::IOFile npbindFile(trpPath, Common::FS::FileAccessMode::Read);
    if (!npbindFile.IsOpen()) {
        LOG_CRITICAL(Common_Filesystem, "Failed to open npbind.dat file");
        return;
    }
    if (!npbindFile.Seek(0x84 + (index * 0x180))) {
        LOG_CRITICAL(Common_Filesystem, "Failed to seek to NPbind offset");
        return;
    }
    npbindFile.ReadRaw<u8>(np_comm_id.data(), 12);
    std::fill(np_comm_id.begin() + 12, np_comm_id.end(), 0); // fill with 0, we need 16 bytes.
}

static void removePadding(std::vector<u8>& vec) {
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        if (*it == '>') {
            size_t pos = std::distance(vec.begin(), it.base());
            vec.resize(pos);
            break;
        }
    }
}

static void hexToBytes(const char* hex, unsigned char* dst) {
    for (size_t i = 0; hex[i] != 0; i++) {
        const unsigned char value = (hex[i] < 0x3A) ? (hex[i] - 0x30) : (hex[i] - 0x37);
        dst[i / 2] |= ((i % 2) == 0) ? (value << 4) : (value);
    }
}

bool TRP::Extract(const std::filesystem::path& trophyPath, const std::string titleId) {
    std::filesystem::path gameSysDir = trophyPath / "sce_sys/trophy/";
    if (!std::filesystem::exists(gameSysDir)) {
        LOG_WARNING(Common_Filesystem, "Game trophy directory doesn't exist");
        return false;
    }

    const auto user_key_str = Config::getTrophyKey();
    if (user_key_str.size() != 32) {
        LOG_INFO(Common_Filesystem, "Trophy decryption key is not specified");
        return false;
    }

    std::array<u8, 16> user_key{};
    hexToBytes(user_key_str.c_str(), user_key.data());

    for (int index = 0; const auto& it : std::filesystem::directory_iterator(gameSysDir)) {
        if (it.is_regular_file()) {
            GetNPcommID(trophyPath, index);

            Common::FS::IOFile file(it.path(), Common::FS::FileAccessMode::Read);
            if (!file.IsOpen()) {
                LOG_CRITICAL(Common_Filesystem, "Unable to open trophy file for read");
                return false;
            }

            TrpHeader header;
            file.Read(header);
            if (header.magic != 0xDCA24D00) {
                LOG_CRITICAL(Common_Filesystem, "Wrong trophy magic number");
                return false;
            }

            s64 seekPos = sizeof(TrpHeader);
            std::filesystem::path trpFilesPath(
                Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / titleId /
                "TrophyFiles" / it.path().stem());
            std::filesystem::create_directories(trpFilesPath / "Icons");
            std::filesystem::create_directory(trpFilesPath / "Xml");

            for (int i = 0; i < header.entry_num; i++) {
                if (!file.Seek(seekPos)) {
                    LOG_CRITICAL(Common_Filesystem, "Failed to seek to TRP entry offset");
                    return false;
                }
                seekPos += (s64)header.entry_size;
                TrpEntry entry;
                file.Read(entry);
                std::string_view name(entry.entry_name);
                if (entry.flag == 0) { // PNG
                    if (!file.Seek(entry.entry_pos)) {
                        LOG_CRITICAL(Common_Filesystem, "Failed to seek to TRP entry offset");
                        return false;
                    }
                    std::vector<u8> icon(entry.entry_len);
                    file.Read(icon);
                    Common::FS::IOFile::WriteBytes(trpFilesPath / "Icons" / name, icon);
                }
                if (entry.flag == 3 && np_comm_id[0] == 'N' &&
                    np_comm_id[1] == 'P') { // ESFM, encrypted.
                    if (!file.Seek(entry.entry_pos)) {
                        LOG_CRITICAL(Common_Filesystem, "Failed to seek to TRP entry offset");
                        return false;
                    }
                    file.Read(esfmIv); // get iv key.
                    // Skip the first 16 bytes which are the iv key on every entry as we want a
                    // clean xml file.
                    std::vector<u8> ESFM(entry.entry_len - iv_len);
                    std::vector<u8> XML(entry.entry_len - iv_len);
                    if (!file.Seek(entry.entry_pos + iv_len)) {
                        LOG_CRITICAL(Common_Filesystem, "Failed to seek to TRP entry + iv offset");
                        return false;
                    }
                    file.Read(ESFM);
                    DecryptEFSM(user_key, np_comm_id, esfmIv, ESFM, XML); // decrypt
                    removePadding(XML);
                    std::string xml_name = entry.entry_name;
                    size_t pos = xml_name.find("ESFM");
                    if (pos != std::string::npos)
                        xml_name.replace(pos, xml_name.length(), "XML");
                    std::filesystem::path path = trpFilesPath / "Xml" / xml_name;
                    size_t written = Common::FS::IOFile::WriteBytes(path, XML);
                    if (written != XML.size()) {
                        LOG_CRITICAL(
                            Common_Filesystem,
                            "Trophy XML {} write failed, wanted to write {} bytes, wrote {}",
                            fmt::UTF(path.u8string()), XML.size(), written);
                    }
                }
            }
        }
        index++;
    }
    return true;
}
