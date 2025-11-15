// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Device : public Inode {

public:
    Device();
    ~Device();

    static dev_ptr Create() {
        return std::make_shared<Device>();
    }

    virtual s64 read(void* buf, u64 count);
    virtual s64 write(const void* buf, u64 count);

    s64 pread(void* buf, u64 count, s64 offset) final override;
    s64 pwrite(const void* buf, u64 count, s64 offset) final override;
};

} // namespace QuasiFS