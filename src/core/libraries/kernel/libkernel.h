// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <sys/types.h>
#include "common/types.h"
#include "core/libraries/error_codes.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(int result);
int ErrnoToSceKernelError(int e);
void SetPosixErrno(int e);

template <class F, F f>
struct WrapperImpl;

template <class R, class... Args, PS4_SYSV_ABI R (*f)(Args...)>
struct WrapperImpl<PS4_SYSV_ABI R (*)(Args...), f> {
    static R PS4_SYSV_ABI wrap(Args... args) {
        u32 ret = f(args...);
        if (ret != 0) {
            ret += SCE_KERNEL_ERROR_UNKNOWN;
        }
        return ret;
    }
};

template <class F, F f>
constexpr auto OrbisWrapper = WrapperImpl<F, f>::wrap;

#define ORBIS(func) OrbisWrapper<decltype(&func), func>

int* PS4_SYSV_ABI __Error();

void RegisterKernel(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
