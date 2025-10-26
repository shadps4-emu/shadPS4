// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Socket : public Inode {

public:
    Socket();
    ~Socket();

    template <typename T, typename... Args>
    static dev_ptr Create(Args&&... args) {
        if constexpr (std::is_base_of_v<Socket, T>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        UNREACHABLE();
    }
};

} // namespace QuasiFS