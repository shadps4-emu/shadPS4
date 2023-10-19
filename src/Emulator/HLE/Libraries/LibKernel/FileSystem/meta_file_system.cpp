#include "meta_file_system.h"

namespace Emulator::Host::GenericFS {

void MetaFileSystem::mount(std::string prefix, AbstractFileSystem* system) {
    System x;
    x.prefix = prefix;
    x.system = system;
    fileSystems.push_back(x);
}

void MetaFileSystem::unMount(AbstractFileSystem* system) {}

void MetaFileSystem::unMountAll() { fileSystems.clear(); }

AbstractFileSystem* MetaFileSystem::getHandleOwner(u32 handle) { 
	for (u32 i = 0; i < fileSystems.size(); i++) {
        if (fileSystems[i].system->ownsHandle(handle)) return fileSystems[i].system;  // got it!
    }
	return nullptr; 

}

bool MetaFileSystem::mapFilePath(std::string inpath, std::string* outpath, AbstractFileSystem** system) { 
    for (unsigned int i = 0; i < fileSystems.size(); i++) {
        int prefLen = fileSystems[i].prefix.size();
        if (fileSystems[i].prefix == inpath.substr(0, prefLen))
        {
            *outpath = inpath.substr(prefLen);
            *system = fileSystems[i].system;
            return true;
        }
    }
    return false;
}

u32 MetaFileSystem::openFile(std::string filename, FileAccess access) { 
    AbstractFileSystem* system;
    std::string of;
    if (mapFilePath(filename, &of, &system)) {
        return system->openFile(of, access);
    }
    return 0;
}

void MetaFileSystem::closeFile(u32 handle) {
    AbstractFileSystem* sys = getHandleOwner(handle);
    if (sys) sys->closeFile(handle);
}


}  // namespace Emulator::Host::GenericFS