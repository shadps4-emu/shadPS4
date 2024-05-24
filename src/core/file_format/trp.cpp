// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "trp.h"

TRP::TRP() = default;
TRP::~TRP() = default;

void TRP::GetNPcommID(std::filesystem::path trophyPath, int index) {
    std::filesystem::path trpPath = trophyPath / "sce_sys/npbind.dat";
    Common::FS::IOFile npbindFile(trpPath, Common::FS::FileAccessMode::Read);
    if (!npbindFile.IsOpen()) {
        return;
    }
    npbindFile.Seek(0x84 + (index * 0x180));
    npbindFile.Read(NPcommID);
    NPcommID.resize(16);
}

void TRP::removePadding(std::vector<u8>& vec) {
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        if (*it == '>') {
            size_t pos = std::distance(vec.begin(), it.base());
            vec.resize(pos);
            break;
        }
    }
}

bool TRP::Extract(std::filesystem::path trophyPath) {
    std::string title = trophyPath.filename().string();
    std::filesystem::path gameSysDir = trophyPath / "sce_sys/trophy/";
    if (!std::filesystem::exists(gameSysDir)) {
        return false;
    }
    std::vector<std::filesystem::path> fileList;
    for (int index = 0; const auto& it : std::filesystem::directory_iterator(gameSysDir)) {
        if (it.is_regular_file()) {
            GetNPcommID(trophyPath, index);

            Common::FS::IOFile file(it.path(), Common::FS::FileAccessMode::Read);
            if (!file.IsOpen()) {
                return false;
            }

            TrpHeader header;
            file.Read(header);
            if (header.magic != 0xDCA24D00)
                return false;

            s64 seekPos = sizeof(TrpHeader);
            std::filesystem::path trpFilesPath(std::filesystem::current_path() / "game_data" /
                                               title / "TrophyFiles" / it.path().stem());
            std::filesystem::create_directories(trpFilesPath / "Icons");
            std::filesystem::create_directory(trpFilesPath / "Xml");

            for (int i = 0; i < header.entry_num; i++) {
                file.Seek(seekPos);
                seekPos += (s64)header.entry_size;
                TrpEntry entry;
                file.Read(entry);
                std::string_view name(entry.entry_name);
                if (entry.flag == 0 && name.find("TROP") != std::string::npos) { // PNG
                    file.Seek(entry.entry_pos);
                    std::vector<u8> icon(entry.entry_len);
                    file.Read(icon);

                    Common::FS::IOFile out(trpFilesPath / "Icons" / name,
                                           Common::FS::FileAccessMode::Write);
                    out.Write(icon);
                    out.Close();
                }
                if (entry.flag == 3 && NPcommID[0] == 'N' &&
                    NPcommID[1] == 'P') { // ESFM, encrypted.
                    file.Seek(entry.entry_pos);
                    file.Read(efsmIv); // get iv key.
                    std::vector<u8> ESFM(entry.entry_len - 16);
                    std::vector<u8> XML(entry.entry_len - 16);
                    file.Seek(entry.entry_pos + 16);
                    file.Read(ESFM);
                    crypto.decryptEFSM(NPcommID, efsmIv, ESFM, XML); // decrypt
                    removePadding(XML);
                    std::string xml_name = entry.entry_name;
                    size_t pos = xml_name.find("ESFM");
                    if (pos != std::string::npos)
                        xml_name.replace(pos, xml_name.length(), "XML");
                    Common::FS::IOFile out(trpFilesPath / "Xml" / xml_name,
                                           Common::FS::FileAccessMode::Write);
                    out.Write(XML);
                }
            }
        }
        index++;
    }
    return true;
}