#include "meta_file_system.h"

namespace Emulator::Host::GenericFS {

void MetaFileSystem::mount(std::string prefix, AbstractFileSystem* system) {}

void MetaFileSystem::unMount(AbstractFileSystem* system) {}

void MetaFileSystem::unMountAll() {}

bool MetaFileSystem::mapFilePath(std::string inpath, std::string* outpath, AbstractFileSystem** system) { return false; }

void MetaFileSystem::releaseHandle(u32 handle) {}

u32 MetaFileSystem::openFile(std::string filename, FileAccess access) { return u32(); }

void MetaFileSystem::closeFile(u32 handle) {}


}  // namespace Emulator::Host::GenericFS