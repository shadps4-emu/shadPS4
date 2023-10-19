#pragma once
#include <types.h>

#include <string>
#include <vector>

#include "generic_file_system.h"

namespace Emulator::Host::GenericFS {

class MetaFileSystem : public GenericHandleAllocator, AbstractFileSystem {
    struct System {
        std::string prefix;
        AbstractFileSystem *system;
    };

    u32 current;
    std::vector<System> fileSystems;
    std::string currentDirectory;
    std::vector<u32> handler;

  public:
    MetaFileSystem() : current(0) {}

    void mount(std::string prefix, AbstractFileSystem *system);
    void unMount(AbstractFileSystem *system);
    void unMountAll();
    AbstractFileSystem *getHandleOwner(u32 handle);
    bool mapFilePath(std::string inpath, std::string *outpath, AbstractFileSystem **system);
    u32 requestHandle() {
        handler.push_back(current);
        current++;
        return handler.back();
    }
    void releaseHandle(u32 handle) { handler.erase(std::remove(handler.begin(), handler.end(), handle), handler.end()); }
    bool ownsHandle(u32 handle) { return false; }
    u32 openFile(std::string filename, FileAccess access);
    void closeFile(u32 handle);
};
}  // namespace Emulator::Host::GenericFS