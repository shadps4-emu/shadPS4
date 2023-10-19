#pragma once
#include <types.h>

#include <string>

namespace Emulator::Host::GenericFS {

enum FileAccess {
    FILEACCESS_READ = 0,
    FILEACCESS_WRITE = 1,
    FILEACCESS_READWRITE = 2

};

class GenericHandleAllocator {
  public:
    virtual u32 requestHandle() = 0;
    virtual void releaseHandle(u32 handle) = 0;
};

class AbstractFileSystem {
  public:
    virtual bool ownsHandle(u32 handle) = 0;
    virtual u32 openFile(std::string filename, FileAccess access) = 0;
    virtual void closeFile(u32 handle) = 0;
};
}  // namespace Emulator::Host::GenericFS