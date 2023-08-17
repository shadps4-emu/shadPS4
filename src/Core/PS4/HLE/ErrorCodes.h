#pragma once
constexpr int SCE_OK = 0;

constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000c;        // Insufficient memory
constexpr int SCE_KERNEL_ERROR_EFAULT = 0x8002000e;        // Invalid address pointer
constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80020016;        // null or invalid states
constexpr int SCE_KERNEL_ERROR_EAGAIN = 0x80020023;        // Memory cannot be allocated
constexpr int SCE_KERNEL_ERROR_ENAMETOOLONG = 0x8002003f;  // character strings exceeds valid size
