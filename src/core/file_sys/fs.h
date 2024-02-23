#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <common/io_file.h>
#include "common/fs_file.h"

namespace Core::FileSys {

class MntPoints {
public:
    struct MntPair {
        std::string host_path;
        std::string guest_path; // e.g /app0/
    };

    MntPoints() = default;
    virtual ~MntPoints() = default;
    void mount(const std::string& host_folder, const std::string& guest_folder);
    void unmount(const std::string& path);
    void unmountAll();
    std::string getHostDirectory(const std::string& guest_directory);
    std::string getHostFile(const std::string& guest_file);

private:
    std::vector<MntPair> m_mnt_pairs;
    std::mutex m_mutex;
};

struct File {
    std::atomic_bool isOpened;
    std::atomic_bool isDirectory;
    std::string m_host_name;
    std::string m_guest_name;
    IOFile f;
    // std::vector<Common::FS::DirEntry> dirents;
    u32 dirents_index;
    std::mutex m_mutex;
};
class HandleTable {
public:
    HandleTable() {}
    virtual ~HandleTable() {}
    int createHandle();
    void deleteHandle(int d);
    File* getFile(int d);
    File* getFile(const std::string& host_name);

private:
    std::vector<File*> m_files;
    std::mutex m_mutex;
};

} // namespace Core::FileSys
