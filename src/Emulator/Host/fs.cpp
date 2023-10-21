#include "fs.h"

#include <algorithm>

namespace Emulator::Host::Fs {
void MntPoints::mount(const std::string& host_folder, const std::string& guest_folder) {
    Lib::LockMutexGuard lock(m_mutex);

    MntPair pair;
    pair.host_path = host_folder;
    pair.guest_path = guest_folder;

    m_mnt_pairs.push_back(pair);
}
void MntPoints::unMount(const std::string& path) {}
void MntPoints::unmountAll() {}
std::string MntPoints::getHostDirectory(const std::string& guest_directory) {
    Lib::LockMutexGuard lock(m_mutex);
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
}  // namespace Emulator::Host::Fs