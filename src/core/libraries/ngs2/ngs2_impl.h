// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2 {
public:
    s32 ReportInvalid(Ngs2Handle* handle, u32 handle_type) const;
    s32 HandleSetup(Ngs2Handle* handle, void* data, std::atomic<u32>* atomic, u32 type, u32 flags);
    s32 HandleCleanup(Ngs2Handle* handle, u32 hType, void* dataOut);
    s32 HandleEnter(Ngs2Handle* handle, u32 hType, Ngs2Handle* handleOut);
    s32 HandleLeave(Ngs2Handle* handle);
    s32 StackBufferOpen(StackBuffer* buf, void* base_addr, size_t size, void** stackTop,
                        bool verify);
    s32 StackBufferClose(StackBuffer* buf, size_t* usedSize);
    s32 SystemSetupCore(StackBuffer* buf, SystemOptions* options, Ngs2Handle** sysOut);

private:
};

} // namespace Libraries::Ngs2
