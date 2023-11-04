#include "fs.h"

#include <algorithm>

namespace Core::FileSys {
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
int HandleTable::createHandle() { return 0; }
void HandleTable::deleteHandle(int d) {}
File* HandleTable::getFile(int d) { return nullptr; }
File* HandleTable::getFile(const std::string& real_name) { return nullptr; }
}  // namespace Core::FileSys