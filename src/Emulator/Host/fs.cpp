#include "fs.h"

namespace Emulator::Host::Fs {
void MntPoints::mount(const std::string& host_folder, const std::string& guest_folder) { 
	Lib::LockMutexGuard lock(m_mutex);

	MntPair pair;
    pair.host_path = host_folder;
    pair.guest_path = guest_folder;

	m_mnt_pairs.push_back(pair);
}
void MntPoints::unMount(const std::string& path) {}
void MntPoints::unMountAll() {}
}  // namespace Emulator::Host::Fs