#pragma once

#include <types.h>
#include "printf.h"

namespace Core::Libraries::LibC::stdio {
int PS4_SYSV_ABI printf(VA_ARGS);
int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg);

}