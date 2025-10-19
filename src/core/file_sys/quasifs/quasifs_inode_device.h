// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Device : public Inode {

public:
    Device();
    ~Device() = default;
};

} // namespace QuasiFS