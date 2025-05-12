// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(int result);
int ErrnoToSceKernelError(int e);
void SetPosixErrno(int e);
int* PS4_SYSV_ABI __Error();

template <class F, F f>
struct OrbisWrapperImpl;

template <class R, class... Args, PS4_SYSV_ABI R (*f)(Args...)>
struct OrbisWrapperImpl<PS4_SYSV_ABI R (*)(Args...), f> {
    static R PS4_SYSV_ABI wrap(Args... args) {
        u32 ret = f(args...);
        if (ret != 0) {
            ret += ORBIS_KERNEL_ERROR_UNKNOWN;
        }
        return ret;
    }
};

#define ORBIS(func) (Libraries::Kernel::OrbisWrapperImpl<decltype(&(func)), func>::wrap)

int* PS4_SYSV_ABI __Error();

void RegisterKernel(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
