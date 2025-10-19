#include <cstring>

#include "core/file_sys/quasifs/quasifs_inode_device.h"

class zero_device : public QuasiFS::Device {

public:
    zero_device() = default;
    ~zero_device() = default;

    virtual s64 read(u64 offset, void* buf, u64 count) {
        memset(buf, 0, count);
        return count;
    }

    virtual s64 write(u64 offset, const void* buf, u64 count) {
        return count;
    }
};
