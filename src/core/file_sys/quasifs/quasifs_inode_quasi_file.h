// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class QuasiFile : public Inode {

public:
    QuasiFile() {
        st.st_mode = 0000755 | QUASI_S_IFREG;
        st.st_nlink = 0;
    };
    ~QuasiFile() = default;

    static file_ptr Create() {
        return std::make_shared<QuasiFile>();
    }

    s64 pread(void* buf, size_t count, s64 offset) override;
    s64 pwrite(const void* buf, size_t count, s64 offset) override;

    s64 lseek(s64 current, s64 offset, s32 whence) override;

    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS