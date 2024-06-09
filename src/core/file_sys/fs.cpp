// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "core/file_sys/fs.h"

namespace Core::FileSys {

constexpr int RESERVED_HANDLES = 3; // First 3 handles are stdin,stdout,stderr

void MntPoints::Mount(const std::filesystem::path& host_folder, const std::string& guest_folder) {
    std::scoped_lock lock{m_mutex};

    MntPair pair;
    pair.host_path = host_folder.string();
    pair.guest_path = guest_folder;

    m_mnt_pairs.push_back(pair);
}

void MntPoints::Unmount(const std::string& path) {} // TODO!

void MntPoints::UnmountAll() {
    std::scoped_lock lock{m_mutex};
    m_mnt_pairs.clear();
}

std::string MntPoints::GetHostDirectory(const std::string& guest_directory) {
    std::scoped_lock lock{m_mutex};
    for (auto& pair : m_mnt_pairs) {
        // horrible code but it works :D
        int find = guest_directory.find(pair.guest_path);
        if (find == 0) {
            std::string npath =
                guest_directory.substr(pair.guest_path.size(), guest_directory.size() - 1);
            std::replace(pair.host_path.begin(), pair.host_path.end(), '\\', '/');
            return pair.host_path + npath;
        }
    }
    return "";
}

std::string MntPoints::GetHostFile(const std::string& guest_file) {
    std::scoped_lock lock{m_mutex};

    for (auto& pair : m_mnt_pairs) {
        // horrible code but it works :D
        int find = guest_file.find(pair.guest_path);
        if (find == 0) {
            std::string npath = guest_file.substr(pair.guest_path.size(), guest_file.size() - 1);
            std::replace(pair.host_path.begin(), pair.host_path.end(), '\\', '/');
            return pair.host_path + npath;
        }
    }
    return "";
}

int HandleTable::CreateHandle() {
    std::scoped_lock lock{m_mutex};

    auto* file = new File{};
    file->is_directory = false;
    file->is_opened = false;

    int existingFilesNum = m_files.size();

    for (int index = 0; index < existingFilesNum; index++) {
        if (m_files.at(index) == nullptr) {
            m_files[index] = file;
            return index + RESERVED_HANDLES;
        }
    }

    m_files.push_back(file);
    return m_files.size() + RESERVED_HANDLES - 1;
}

void HandleTable::DeleteHandle(int d) {
    std::scoped_lock lock{m_mutex};
    delete m_files.at(d - RESERVED_HANDLES);
    m_files[d - RESERVED_HANDLES] = nullptr;
}

File* HandleTable::GetFile(int d) {
    std::scoped_lock lock{m_mutex};
    return m_files.at(d - RESERVED_HANDLES);
}

File* HandleTable::getFile(const std::string& host_name) {
    std::scoped_lock lock{m_mutex};
    for (auto* file : m_files) {
        if (file != nullptr && file->m_host_name == host_name) {
            return file;
        }
    }
    return nullptr;
}

} // namespace Core::FileSys
