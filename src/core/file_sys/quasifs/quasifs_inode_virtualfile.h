// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode_quasi_file.h"

namespace QuasiFS {

class VirtualFile final : public QuasiFile {
    std::vector<char> data{};

public:
    VirtualFile() = default;
    ~VirtualFile() = default;

    //
    // Working functions
    //
    s64 read(void* buf, size_t count) override;
    s64 write(const void* buf, size_t count) override;
    s64 pread(void* buf, size_t count, u64 offset) override;
    s64 pwrite(const void* buf, size_t count, u64 offset) override;
    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS