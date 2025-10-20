// INAA License @marecl 2025

#pragma once

#include "common/assert.h"
#include "common/logging/log.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

#define DEVICE_STUB()                                                                              \
    {                                                                                              \
        LOG_ERROR(Kernel_Fs, "(STUBBED) called");                                                  \
        return -QUASI_ENOSYS;                                                                                  \
    }

namespace QuasiFS {

class Device : public Inode {

public:
    Device();
    ~Device();

    template <typename T, typename... Args>
    static dev_ptr Create(Args&&... args) {
        if constexpr (std::is_base_of_v<Device, T>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        UNREACHABLE();
    }
};

} // namespace QuasiFS