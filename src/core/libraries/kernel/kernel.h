// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/linker.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(s32 result);
s32 ErrnoToSceKernelError(s32 e);
void SetPosixErrno(s32 e);
s32* PS4_SYSV_ABI __Error();

extern Core::EntryParams entry_params;

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

#define CURRENT_FIRMWARE_VERSION 0x13020011

s32* PS4_SYSV_ABI __Error();

struct SwVersionStruct {
    u64 struct_size;
    char text_representation[0x1c];
    u32 hex_representation;
};

struct AuthInfoData {
    u64 paid;
    u64 caps[4];
    u64 attrs[4];
    u64 ucred[8];
};

struct OrbisKernelTitleWorkaround {
    s32 version;
    s32 align;
    u64 ids[2];
};

struct OrbisKernelAppInfo {
    s32 app_id;
    s32 mmap_flags;
    s32 attribute_exe;
    s32 attribute2;
    char cusa_name[10];
    u8 debug_level;
    u8 slv_flags;
    u8 mini_app_dmem_flags;
    u8 render_mode;
    u8 mdbg_out;
    u8 required_hdcp_type;
    u64 preload_prx_flags;
    s32 attribute1;
    s32 has_param_sfo;
    OrbisKernelTitleWorkaround title_workaround;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
