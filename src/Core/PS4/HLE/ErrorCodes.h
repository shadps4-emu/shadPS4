#pragma once
constexpr int SCE_OK = 0;

constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000c;//Insufficient memory
constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80020016;//null or invalid states
constexpr int SCE_KERNEL_ERROR_EAGAIN = 0x80020023;// Memory cannot be allocated
