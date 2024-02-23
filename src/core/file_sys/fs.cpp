#include "fs.h"

#include <algorithm>

namespace Core::FileSys {

constexpr int RESERVED_HANDLES = 3; // First 3 handles are stdin,stdout,stderr

void MntPoints::mount(const std::string& host_folder, const std::string& guest_folder) {
    std::scoped_lock lock{m_mutex};

    MntPair pair;
    pair.host_path = host_folder;
    pair.guest_path = guest_folder;

    m_mnt_pairs.push_back(pair);
}
void MntPoints::unmount(const std::string& path) {} // TODO!
void MntPoints::unmountAll() {
    std::scoped_lock lock{m_mutex};
    m_mnt_pairs.clear();
}
std::string MntPoints::getHostDirectory(const std::string& guest_directory) {
    std::scoped_lock lock{m_mutex};
    for (auto& pair : m_mnt_pairs) {
        if (pair.guest_path.starts_with(guest_directory)) {
            return pair.host_path + guest_directory;
        }
    }
    // hack for relative path , get app0 and assuming it goes from there
    for (auto& pair : m_mnt_pairs) {
        if (pair.guest_path.starts_with("/app0")) {
            std::replace(pair.host_path.begin(), pair.host_path.end(), '\\', '/');
            return pair.host_path + guest_directory;
        }
    }
    return "";
}
std::string MntPoints::getHostFile(const std::string& guest_file) {
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
int HandleTable::createHandle() {
    std::scoped_lock lock{m_mutex};
    auto* file = new File{};
    file->isDirectory = false;
    file->isOpened = false;

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
void HandleTable::deleteHandle(int d) {
    std::scoped_lock lock{m_mutex};
    delete m_files.at(d - RESERVED_HANDLES);
    m_files[d - RESERVED_HANDLES] = nullptr;
}

File* HandleTable::getFile(int d) {
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