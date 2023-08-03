#pragma once
#include <types.h>

// constants

constexpr u64 SCE_KERNEL_MAIN_DMEM_SIZE = 5376_MB;  // ~ 6GB

// memory types

enum MemoryTypes : u32 {
    SCE_KERNEL_WB_ONION = 0,   // write - back mode (Onion bus)
    SCE_KERNEL_WC_GARLIC = 3,  // write - combining mode (Garlic bus)
    SCE_KERNEL_WB_GARLIC = 10  // write - back mode (Garlic bus)
};