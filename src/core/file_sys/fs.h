// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include "common/io_file.h"

namespace Core::FileSys {

class MntPoints {
public:
    struct MntPair {
        std::string host_path;
        std::string guest_path; // e.g /app0/
    };

    MntPoints() = default;
    virtual ~MntPoints() = default;

    void Mount(const std::filesystem::path& host_folder, const std::string& guest_folder);
    void Unmount(const std::string& path);
    void UnmountAll();
    std::string GetHostDirectory(const std::string& guest_directory);
    std::string GetHostFile(const std::string& guest_file);

private:
    std::vector<MntPair> m_mnt_pairs;
    std::mutex m_mutex;
};

struct DirEntry {
    std::string name;
    bool isFile;
};

struct File {
    std::atomic_bool is_opened{};
    std::atomic_bool is_directory{};
    std::string m_host_name;
    std::string m_guest_name;
    Common::FS::IOFile f;
    std::vector<DirEntry> dirents;
    u32 dirents_index;
    std::mutex m_mutex;
};

class HandleTable {
public:
    HandleTable() = default;
    virtual ~HandleTable() = default;

    int CreateHandle();
    void DeleteHandle(int d);
    File* GetFile(int d);
    File* getFile(const std::string& host_name);

private:
    std::vector<File*> m_files;
    std::mutex m_mutex;
};

} // namespace Core::FileSys
