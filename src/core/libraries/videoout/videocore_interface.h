// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/equeue.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VideoCoreInterface {

struct OrbisVideoCoreInterfaceFrameBuffer {
    void* address[2];
};
static_assert(sizeof(OrbisVideoCoreInterfaceFrameBuffer) == 0x10);

using OrbisVideoCoreInterfaceFlushCallback = void(PS4_SYSV_ABI*)(s32 result);

s32 PS4_SYSV_ABI sceVideoCoreInterfaceInitializeInterface(void* param);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetFunctionTable();
s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetFunctionTableExt();
s32 PS4_SYSV_ABI sceVideoCoreInterfaceLoadModule();
s32 PS4_SYSV_ABI sceVideoCoreInterfaceUnloadModule();
s32 PS4_SYSV_ABI sceVideoCoreInterfaceMainLoop(const char* name);

s32 PS4_SYSV_ABI sceVideoCoreInterfaceCreateFrameBufferContext(void* param, const void* info,
                                                               void** frame_buffer_context);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceDestroyFrameBufferContext(void* frame_buffer_context);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceCreateCanvasContext(const void* info, void** canvas_context);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceDestroyCanvasContext(void* canvas_context);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetCanvasId(void* canvas_context, u32 canvas_id);

s32 PS4_SYSV_ABI sceVideoCoreInterfaceOpenCanvas(u32 canvas_id);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceCloseCanvas();
s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetCanvasVideoBufferAddress(u64* address);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetCanvasVideoBufferSize(u64* size);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceSubmitCanvases(const u32* canvas_ids, const u64* commands,
                                                     s32 count);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceWaitCanvasFlip();

s32 PS4_SYSV_ABI sceVideoCoreInterfaceAddVblankEvent(Kernel::OrbisKernelEqueue eq, void* udata);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceOpenVideoOut(void** video_out_context, s32* handle);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceCloseVideoOut(void* video_out_context);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetVideoOutBufferInfo(void* video_out_context,
                                                            s32 start_index, void* const* addresses,
                                                            s32 count);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceFlushVideoOutBuffer(
    void* video_out_context, OrbisVideoCoreInterfaceFlushCallback callback);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetRenderFrameBuffer(
    void* context, OrbisVideoCoreInterfaceFrameBuffer* frame_buffer);
s32 PS4_SYSV_ABI sceVideoCoreInterfaceFinishRendering(void* context);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::VideoCoreInterface
