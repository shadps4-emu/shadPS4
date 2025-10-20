// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class RegularFile final : public Inode {
    std::vector<char> data{};

public:
    RegularFile();
    ~RegularFile() = default;

    static file_ptr Create(void) {
        return std::make_shared<RegularFile>();
    }

    //
    // Working functions
    //
    s64 read(void* buf, size_t count) override;
    s64 write(const void* buf, size_t count) override;
    s64 pread(void* buf, size_t count, u64 offset) override;
    s64 pwrite(const void* buf, size_t count, u64 offset) override;
    s32 ftruncate(s64 length) override;

    //
    // Mock functions
    // Work normally, but don't write data to internal storage
    //
    s64 MockRead(u64 offset, void* buf, u64 count);
    s64 MockWrite(u64 offset, const void* buf, u64 count);
    int MockTruncate(u64 length);
};

} // namespace QuasiFS