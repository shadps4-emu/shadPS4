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

    template <typename T, typename... Args>
    static file_ptr Create(Args&&... args) {
        if constexpr (std::is_base_of_v<QuasiFile, T>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        UNREACHABLE();
    }
};

} // namespace QuasiFS