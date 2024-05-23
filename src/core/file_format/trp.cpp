// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "trp.h"
#ifdef _WIN64
#include <Windows.h>
#elif
#include <limits.h>
#include <unistd.h>
#endif

TRP::TRP() = default;
TRP::~TRP() = default;

void TRP::GetNPcommID(std::filesystem::path trophyPath, int fileNbr) {
    std::filesystem::path trpPath = trophyPath / "sce_sys/npbind.dat";
    Common::FS::IOFile npbindFile(trpPath, Common::FS::FileAccessMode::Read);
    if (!npbindFile.IsOpen()) {
        return;
    }
    for (int i = 0; i < fileNbr; i++) {
        std::vector<u8> vec(12);
        npbindFile.Seek(0x84 + (i * 0x180));
        npbindFile.Read(vec);
        vec.resize(16);
        NPcommID.push_back(vec);
    }
    npbindFile.Close();
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
    for (const auto& entry : std::filesystem::directory_iterator(gameSysDir)) {
        if (entry.is_regular_file()) {
            fileList.push_back(entry.path());
        }
    }

    if (fileList.empty())
        return false;

    GetNPcommID(trophyPath, fileList.size());
    for (int nbr = 0; const std::filesystem::path& fileinfo : fileList) {
        std::string multiTrp = fileinfo.stem().string();

        Common::FS::IOFile file(fileinfo, Common::FS::FileAccessMode::Read);
        if (!file.IsOpen()) {
            return false;
        }

        trp_header header;
        file.ReadRaw<u8>(&header, sizeof(trp_header));

        if (header.magic != 0xDCA24D00)
            return false;

        s64 seekPos = sizeof(trp_header);

        char buf[1024];

#ifdef _WIN64
        GetModuleFileNameA(NULL, buf, 1024);
#elif
        if (getcwd(buffer, sizeof(buf)) == NULL) {
            // std::cerr << "Error getting current directory!" << std::endl;
            return false;
        }
#endif
        std::string appPath(buf);
        std::size_t found = appPath.find_last_of("/\\");
        if (found != std::string::npos) {
            appPath = appPath.substr(0, found);
        }
        trpFilesPath = std::filesystem::path(appPath + "/game_data/" + title);

        std::filesystem::path multiPath(trpFilesPath / "TrophyFiles");
        if (fileList.size() > 1) {
            multiPath = trpFilesPath / "TrophyFiles" / multiTrp;
        }
        std::filesystem::create_directories(multiPath / "Icons");

        for (int i = 0; i < header.entry_num; i++) {
            file.Seek(seekPos);
            seekPos += (s64)header.entry_size;
            trp_entry entry;
            file.ReadRaw<u8>(&entry, sizeof(trp_entry));
            std::string name(entry.entry_name);
            if (entry.flag == 0 && name.find("TROP") != std::string::npos) { // PNG
                file.Seek(entry.entry_pos);
                std::vector<u8> icon(entry.entry_len);
                file.Read(icon);

                Common::FS::IOFile out(multiPath / "Icons" / name,
                                       Common::FS::FileAccessMode::Write);
                out.Write(icon);
                out.Close();
            }

            if (entry.flag == 3 && NPcommID.at(nbr)[0] == 'N' &&
                NPcommID.at(nbr)[1] == 'P') { // ESFM, encrypted.
                file.Seek(entry.entry_pos);
                efsmIv.resize(16);
                file.Read(efsmIv); // get iv key.

                std::vector<u8> ESFM(entry.entry_len - 16);
                std::vector<u8> XML(entry.entry_len - 16);
                XML.reserve(entry.entry_len - 16);
                file.Seek(entry.entry_pos + 16);
                file.ReadRaw<u8>(ESFM.data(), entry.entry_len - 16);

                crypto.decryptEFSM(NPcommID.at(nbr), efsmIv, ESFM, XML); // decrypt
                ESFM.clear();
                removePadding(XML);
                std::string xml_name = name;
                size_t pos = xml_name.find("ESFM");
                if (pos != std::string::npos)
                    xml_name.replace(pos, xml_name.length(), "XML");
                Common::FS::IOFile out(multiPath / xml_name, Common::FS::FileAccessMode::Write);
                out.Write(XML);
                out.Close();
            }
        }
        file.Close();
        nbr++;
    }
    return true;
}