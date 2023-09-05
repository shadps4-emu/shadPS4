#pragma once
constexpr int SCE_OK = 0;

constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000c;        // Insufficient memory
constexpr int SCE_KERNEL_ERROR_EFAULT = 0x8002000e;        // Invalid address pointer
constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80020016;        // null or invalid states
constexpr int SCE_KERNEL_ERROR_EAGAIN = 0x80020023;        // Memory cannot be allocated
constexpr int SCE_KERNEL_ERROR_ENAMETOOLONG = 0x8002003f;  // character strings exceeds valid size

// videoOut
constexpr int SCE_VIDEO_OUT_ERROR_INVALID_VALUE = 0x80290001;        // invalid argument
constexpr int SCE_VIDEO_OUT_ERROR_RESOURCE_BUSY = 0x80290009;        // already opened
constexpr int SCE_VIDEO_OUT_ERROR_INVALID_HANDLE = 0x8029000B;       // invalid handle
constexpr int SCE_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE = 0x8029000C;  // Invalid event queue
