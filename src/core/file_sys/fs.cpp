#include "fs.h"

#include <algorithm>

namespace Core::FileSys {

constexpr int RESERVED_HANDLES = 3; //first 3 handles are stdin,stdout,stderr

void MntPoints::mount(const std::string& host_folder, const std::string& guest_folder) {
    std::unique_lock lock{m_mutex};

    MntPair pair;
    pair.host_path = host_folder;
    pair.guest_path = guest_folder;

    m_mnt_pairs.push_back(pair);
}
void MntPoints::unmount(const std::string& path) {}  // TODO!
void MntPoints::unmountAll() {
    std::unique_lock lock{m_mutex};
    m_mnt_pairs.clear();
}
std::string MntPoints::getHostDirectory(const std::string& guest_directory) {
    std::unique_lock lock{m_mutex};
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
int HandleTable::createHandle() {
    std::unique_lock lock{m_mutex};
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

    return existingFilesNum + RESERVED_HANDLES  - 1;
}
void HandleTable::deleteHandle(int d) {
    std::unique_lock lock{m_mutex};
    delete m_files.at(d - RESERVED_HANDLES);
    m_files[d - RESERVED_HANDLES] = nullptr;
}
File* HandleTable::getFile(int d) {
    std::unique_lock lock{m_mutex};
    return m_files.at(d - RESERVED_HANDLES);
}
File* HandleTable::getFile(const std::string& real_name) {
    std::unique_lock lock{m_mutex};
    for (auto* file : m_files) {
        if (file != nullptr && file->m_real_name == real_name) {
            return file;
        }
    }
    return nullptr;
}
}  // namespace Core::FileSys