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

    static socket_ptr Create() {
        return std::make_shared<Socket>();
    }
};

} // namespace QuasiFS