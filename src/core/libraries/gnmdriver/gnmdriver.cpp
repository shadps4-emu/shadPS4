// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gnm_error.h"
#include "gnmdriver.h"

#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/slot_vector.h"
#include "core/address_space.h"
#include "core/debug_state.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/libs.h"
#include "core/libraries/videoout/video_out.h"
#include "core/platform.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pm4_cmds.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"

extern Frontend::WindowSDL* g_window;
std::unique_ptr<Vulkan::RendererVulkan> renderer;
std::unique_ptr<AmdGpu::Liverpool> liverpool;

namespace Libraries::GnmDriver {

using namespace AmdGpu;

enum GnmEventIdents : u64 {
    Compute0RelMem = 0x00,
    Compute1RelMem = 0x01,
    Compute2RelMem = 0x02,
    Compute3RelMem = 0x03,
    Compute4RelMem = 0x04,
    Compute5RelMem = 0x05,
    Compute6RelMem = 0x06,
    GfxEop = 0x40
};

enum ShaderStages : u32 {
    Cs,
    Ps,
    Vs,
    Gs,
    Es,
    Hs,
    Ls,

    Max
};

static constexpr std::array indirect_sgpr_offsets{0u, 0u, 0x4cu, 0u, 0xccu, 0u, 0x14cu};

static constexpr auto HwInitPacketSize = 0x100u;

// clang-format off
static constexpr std::array InitSequence{
    // A fake preamble to mimic context reset sent by FW
    0xc0001200u, 0u, // IT_CLEAR_STATE

    // Actual init state sequence
    0xc0017600u, 0x216u, 0xffffffffu,
    0xc0017600u, 0x217u, 0xffffffffu,
    0xc0017600u, 0x215u, 0u,
    0xc0016900u, 0x2f9u, 0x2du,
    0xc0016900u, 0x282u, 8u,
    0xc0016900u, 0x280u, 0x80008u,
    0xc0016900u, 0x281u, 0xffff0000u,
    0xc0016900u, 0x204u, 0u,
    0xc0016900u, 0x206u, 0x43fu,
    0xc0016900u, 0x83u,  0xffffu,
    0xc0016900u, 0x317u, 0x10u,
    0xc0016900u, 0x2fau, 0x3f800000u,
    0xc0016900u, 0x2fcu, 0x3f800000u,
    0xc0016900u, 0x2fbu, 0x3f800000u,
    0xc0016900u, 0x2fdu, 0x3f800000u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0016900u, 0x30eu, 0xffffffffu,
    0xc0016900u, 0x30fu, 0xffffffffu,
    0xc0002f00u, 1u,
    0xc0017600u, 7u,     0x1ffu,
    0xc0017600u, 0x46u,  0x1ffu,
    0xc0017600u, 0x87u,  0x1ffu,
    0xc0017600u, 0xc7u,  0x1ffu,
    0xc0017600u, 0x107u, 0u,
    0xc0017600u, 0x147u, 0x1ffu,
    0xc0016900u, 0x1b1u, 2u,
    0xc0016900u, 0x101u, 0u,
    0xc0016900u, 0x100u, 0xffffffffu,
    0xc0016900u, 0x103u, 0u,
    0xc0016900u, 0x284u, 0u,
    0xc0016900u, 0x290u, 0u,
    0xc0016900u, 0x2aeu, 0u,
    0xc0016900u, 0x292u, 0u,
    0xc0016900u, 0x293u, 0x6000000u,
    0xc0016900u, 0x2f8u, 0u,
    0xc0016900u, 0x2deu, 0x1e9u,
    0xc0036900u, 0x295u, 0x100u, 0x100u, 4u,
    0xc0017900u, 0x200u, 0xe0000000u,
};
static_assert(InitSequence.size() == 0x73 + 2);

static constexpr std::array InitSequence175{
    // A fake preamble to mimic context reset sent by FW
    0xc0001200u, 0u, // IT_CLEAR_STATE

    // Actual init state sequence 
    0xc0017600u, 0x216u, 0xffffffffu,
    0xc0017600u, 0x217u, 0xffffffffu,
    0xc0017600u, 0x215u, 0u,
    0xc0016900u, 0x2f9u, 0x2du,
    0xc0016900u, 0x282u, 8u,
    0xc0016900u, 0x280u, 0x80008u,
    0xc0016900u, 0x281u, 0xffff0000u,
    0xc0016900u, 0x204u, 0u,
    0xc0016900u, 0x206u, 0x43fu,
    0xc0016900u, 0x83u,  0xffffu,
    0xc0016900u, 0x317u, 0x10u,
    0xc0016900u, 0x2fau, 0x3f800000u,
    0xc0016900u, 0x2fcu, 0x3f800000u,
    0xc0016900u, 0x2fbu, 0x3f800000u,
    0xc0016900u, 0x2fdu, 0x3f800000u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0016900u, 0x30eu, 0xffffffffu,
    0xc0016900u, 0x30fu, 0xffffffffu,
    0xc0002f00u, 1u,
    0xc0017600u, 7u,     0x1ffu,
    0xc0017600u, 0x46u,  0x1ffu,
    0xc0017600u, 0x87u,  0x1ffu,
    0xc0017600u, 0xc7u,  0x1ffu,
    0xc0017600u, 0x107u, 0u,
    0xc0017600u, 0x147u, 0x1ffu,
    0xc0016900u, 0x1b1u, 2u,
    0xc0016900u, 0x101u, 0u,
    0xc0016900u, 0x100u, 0xffffffffu,
    0xc0016900u, 0x103u, 0u,
    0xc0016900u, 0x284u, 0u,
    0xc0016900u, 0x290u, 0u,
    0xc0016900u, 0x2aeu, 0u,
    0xc0016900u, 0x292u, 0u,
    0xc0016900u, 0x293u, 0x6020000u,
    0xc0016900u, 0x2f8u, 0u,
    0xc0016900u, 0x2deu, 0x1e9u,
    0xc0036900u, 0x295u, 0x100u, 0x100u, 4u,
    0xc0017900u, 0x200u, 0xe0000000u,
};
static_assert(InitSequence175.size() == 0x73 + 2);

static constexpr std::array InitSequence200{
    // A fake preamble to mimic context reset sent by FW
    0xc0001200u, 0u, // IT_CLEAR_STATE

    // Actual init state sequence    
    0xc0017600u, 0x216u, 0xffffffffu,
    0xc0017600u, 0x217u, 0xffffffffu,
    0xc0017600u, 0x215u, 0u,
    0xc0016900u, 0x2f9u, 0x2du,
    0xc0016900u, 0x282u, 8u,
    0xc0016900u, 0x280u, 0x80008u,
    0xc0016900u, 0x281u, 0xffff0000u,
    0xc0016900u, 0x204u, 0u,
    0xc0016900u, 0x206u, 0x43fu,
    0xc0016900u, 0x83u,  0xffffu,
    0xc0016900u, 0x317u, 0x10u,
    0xc0016900u, 0x2fau, 0x3f800000u,
    0xc0016900u, 0x2fcu, 0x3f800000u,
    0xc0016900u, 0x2fbu, 0x3f800000u,
    0xc0016900u, 0x2fdu, 0x3f800000u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0016900u, 0x30eu, 0xffffffffu,
    0xc0016900u, 0x30fu, 0xffffffffu,
    0xc0002f00u, 1u,
    0xc0017600u, 7u,     0x1701ffu,
    0xc0017600u, 0x46u,  0x1701fdu,
    0xc0017600u, 0x87u,  0x1701ffu,
    0xc0017600u, 0xc7u,  0x1701fdu,
    0xc0017600u, 0x107u, 0x17u,
    0xc0017600u, 0x147u, 0x1701fdu,
    0xc0017600u, 0x47u,  0x1cu,
    0xc0016900u, 0x1b1u, 2u,
    0xc0016900u, 0x101u, 0u,
    0xc0016900u, 0x100u, 0xffffffffu,
    0xc0016900u, 0x103u, 0u,
    0xc0016900u, 0x284u, 0u,
    0xc0016900u, 0x290u, 0u,
    0xc0016900u, 0x2aeu, 0u,
    0xc0016900u, 0x292u, 0u,
    0xc0016900u, 0x293u, 0x6020000u,
    0xc0016900u, 0x2f8u, 0u,
    0xc0016900u, 0x2deu, 0x1e9u,
    0xc0036900u, 0x295u, 0x100u, 0x100u, 4u,
    0xc0017900u, 0x200u, 0xe0000000u,
};
static_assert(InitSequence200.size() == 0x76 + 2);

static constexpr std::array InitSequence350{
    // A fake preamble to mimic context reset sent by FW
    0xc0001200u, 0u, // IT_CLEAR_STATE

    // Actual init state sequence    
    0xc0017600u, 0x216u, 0xffffffffu,
    0xc0017600u, 0x217u, 0xffffffffu,
    0xc0017600u, 0x215u, 0u,
    0xc0016900u, 0x2f9u, 0x2du,
    0xc0016900u, 0x282u, 8u,
    0xc0016900u, 0x280u, 0x80008u,
    0xc0016900u, 0x281u, 0xffff0000u,
    0xc0016900u, 0x204u, 0u,
    0xc0016900u, 0x206u, 0x43fu,
    0xc0016900u, 0x83u,  0xffffu,
    0xc0016900u, 0x317u, 0x10u,
    0xc0016900u, 0x2fau, 0x3f800000u,
    0xc0016900u, 0x2fcu, 0x3f800000u,
    0xc0016900u, 0x2fbu, 0x3f800000u,
    0xc0016900u, 0x2fdu, 0x3f800000u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0016900u, 0x30eu, 0xffffffffu,
    0xc0016900u, 0x30fu, 0xffffffffu,
    0xc0002f00u, 1u,
    0xc0017600u, 7u,    0x1701ffu,
    0xc0017600u, 0x46u,  0x1701fdu,
    0xc0017600u, 0x87u,  0x1701ffu,
    0xc0017600u, 0xc7u,  0x1701fdu,
    0xc0017600u, 0x107u, 0x17u,
    0xc0017600u, 0x147u, 0x1701fdu,
    0xc0017600u, 0x47u,  0x1cu,
    0xc0016900u, 0x1b1u, 2u,
    0xc0016900u, 0x101u, 0u,
    0xc0016900u, 0x100u, 0xffffffffu,
    0xc0016900u, 0x103u, 0u,
    0xc0016900u, 0x284u, 0u,
    0xc0016900u, 0x290u, 0u,
    0xc0016900u, 0x2aeu, 0u,
    0xc0016900u, 0x102u, 0u,
    0xc0016900u, 0x292u, 0u,
    0xc0016900u, 0x293u, 0x6020000u,
    0xc0016900u, 0x2f8u, 0u,
    0xc0016900u, 0x2deu, 0x1e9u,
    0xc0036900u, 0x295u, 0x100u, 0x100u, 4u,
    0xc0017900u, 0x200u, 0xe0000000u,
    0xc0016900u, 0x2aau, 0xffu,
};
static_assert(InitSequence350.size() == 0x7c + 2);

static constexpr std::array CtxInitSequence{
    0xc0012800u, 0x80000000u, 0x80000000u,
    0xc0001200u, 0u,
    0xc0002f00u, 1u,
    0xc0016900u, 0x102u, 0u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0111000u, 0u
};
static_assert(CtxInitSequence.size() == 0x0f);

static constexpr std::array CtxInitSequence400{
    0xc0012800u, 0x80000000u, 0x80000000u,
    0xc0001200u, 0u,
    0xc0016900u, 0x2f9u, 0x2du,
    0xc0016900u, 0x282u, 8u,
    0xc0016900u, 0x280u, 0x80008u,
    0xc0016900u, 0x281u, 0xffff0000u,
    0xc0016900u, 0x204u, 0u,
    0xc0016900u, 0x206u, 0x43fu,
    0xc0016900u, 0x83u,  0xffffu,
    0xc0016900u, 0x317u, 0x10u,
    0xc0016900u, 0x2fau, 0x3f800000u,
    0xc0016900u, 0x2fcu, 0x3f800000u,
    0xc0016900u, 0x2fbu, 0x3f800000u,
    0xc0016900u, 0x2fdu, 0x3f800000u,
    0xc0016900u, 0x202u, 0xcc0010u,
    0xc0016900u, 0x30eu, 0xffffffffu,
    0xc0016900u, 0x30fu, 0xffffffffu,
    0xc0002f00u, 1u,
    0xc0016900u, 0x1b1u, 2u,
    0xc0016900u, 0x101u, 0u,
    0xc0016900u, 0x100u, 0xffffffffu,
    0xc0016900u, 0x103u, 0u,
    0xc0016900u, 0x284u, 0u,
    0xc0016900u, 0x290u, 0u,
    0xc0016900u, 0x2aeu, 0u,
    0xc0016900u, 0x102u, 0u,
    0xc0016900u, 0x292u, 0u,
    0xc0016900u, 0x293u, 0x6020000u,
    0xc0016900u, 0x2f8u, 0u,
    0xc0016900u, 0x2deu, 0x1e9u,
    0xc0036900u, 0x295u, 0x100u, 0x100u, 4u,
    0xc0016900u, 0x2aau, 0xffu,
    0xc09e1000u,
};
static_assert(CtxInitSequence400.size() == 0x61);
// clang-format on

// In case if `submitDone` is issued we need to block submissions until GPU idle
static u32 submission_lock{};
std::condition_variable cv_lock{};
static std::mutex m_submission{};
static u64 frames_submitted{};      // frame counter
static bool send_init_packet{true}; // initialize HW state before first game's submit in a frame
static int sdk_version{0};

struct AscQueueInfo {
    VAddr map_addr;
    u32* read_addr;
    u32 ring_size_dw;
};
static Common::SlotVector<AscQueueInfo> asc_queues{};
static constexpr VAddr tessellation_factors_ring_addr = Core::SYSTEM_RESERVED_MAX - 0xFFFFFFF;

static void ResetSubmissionLock(Platform::InterruptId irq) {
    std::unique_lock lock{m_submission};
    submission_lock = 0;
    cv_lock.notify_all();
}

static void WaitGpuIdle() {
    HLE_TRACE;
    std::unique_lock lock{m_submission};
    cv_lock.wait(lock, [] { return submission_lock == 0; });
}

// Write a special ending NOP packet with N DWs data block
template <u32 data_block_size>
static inline u32* WriteTrailingNop(u32* cmdbuf) {
    auto* nop = reinterpret_cast<PM4CmdNop*>(cmdbuf);
    nop->header = PM4Type3Header{PM4ItOpcode::Nop, data_block_size - 1};
    nop->data_block[0] = 0u; // only one out of `data_block_size` is initialized
    return cmdbuf + data_block_size + 1 /* header */;
}

static inline u32* ClearContextState(u32* cmdbuf) {
    static constexpr std::array ClearStateSequence{
        0xc0012800u, 0x80000000u, 0x80000000u, 0xc0001200u, 0u, 0xc0055800u,
        0x2ec47fc0u, 0xffffffffu, 0u,          0u,          0u, 10u,
    };
    static_assert(ClearStateSequence.size() == 0xc);

    std::memcpy(cmdbuf, ClearStateSequence.data(), ClearStateSequence.size() * 4);
    return cmdbuf + ClearStateSequence.size();
}

s32 PS4_SYSV_ABI sceGnmAddEqEvent(SceKernelEqueue eq, u64 id, void* udata) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!eq) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    EqueueEvent kernel_event{};
    kernel_event.event.ident = id;
    kernel_event.event.filter = SceKernelEvent::Filter::GraphicsCore;
    // The library only sets EV_ADD but it is suspected the kernel driver forces EV_CLEAR
    kernel_event.event.flags = SceKernelEvent::Flags::Clear;
    kernel_event.event.fflags = 0;
    kernel_event.event.data = id;
    kernel_event.event.udata = udata;
    eq->AddEvent(kernel_event);

    Platform::IrqC::Instance()->Register(
        Platform::InterruptId::GfxEop,
        [=](Platform::InterruptId irq) {
            ASSERT_MSG(irq == Platform::InterruptId::GfxEop,
                       "An unexpected IRQ occured"); // We need to convert IRQ# to event id and do
                                                     // proper filtering in trigger function
            eq->TriggerEvent(GnmEventIdents::GfxEop, SceKernelEvent::Filter::GraphicsCore, nullptr);
        },
        eq);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmAreSubmitsAllowed() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return submission_lock == 0;
}

int PS4_SYSV_ABI sceGnmBeginWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmComputeWaitOnAddress(u32* cmdbuf, u32 size, uintptr_t addr, u32 mask,
                                            u32 cmp_func, u32 ref) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 0xe)) {
        cmdbuf = WriteHeader<PM4ItOpcode::Nop>(cmdbuf, 3);
        cmdbuf = WriteBody(cmdbuf, 0u);
        cmdbuf += 2;

        const u32 is_mem = addr > 0xffffu;
        const u32 addr_mask = is_mem ? 0xfffffffcu : 0xffffu;
        auto* wait_reg_mem = reinterpret_cast<PM4CmdWaitRegMem*>(cmdbuf);
        wait_reg_mem->header = PM4Type3Header{PM4ItOpcode::WaitRegMem, 5};
        wait_reg_mem->raw = (is_mem << 4u) | (cmp_func & 7u);
        wait_reg_mem->poll_addr_lo = u32(addr & addr_mask);
        wait_reg_mem->poll_addr_hi = u32(addr >> 32u);
        wait_reg_mem->ref = ref;
        wait_reg_mem->mask = mask;
        wait_reg_mem->poll_interval = 10u;

        WriteTrailingNop<2>(cmdbuf + 7);
        return ORBIS_OK;
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmComputeWaitSemaphore() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmCreateWorkloadStream() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerGetAddressWatch() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerHaltWavefront() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerReadGds() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerReadSqIndirectRegister() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerResumeWavefront() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerResumeWavefrontCreation() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerSetAddressWatch() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerWriteGds() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebuggerWriteSqIndirectRegister() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebugHardwareStatus() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmDeleteEqEvent(SceKernelEqueue eq, u64 id) {
    LOG_TRACE(Lib_GnmDriver, "called");
    ASSERT_MSG(id == GnmEventIdents::GfxEop);

    if (!eq) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    eq->RemoveEvent(id);

    Platform::IrqC::Instance()->Unregister(Platform::InterruptId::GfxEop, eq);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDestroyWorkloadStream() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceGnmDingDong(u32 gnm_vqid, u32 next_offs_dw) {
    LOG_DEBUG(Lib_GnmDriver, "vqid {}, offset_dw {}", gnm_vqid, next_offs_dw);

    if (gnm_vqid == 0) {
        return;
    }

    WaitGpuIdle();

    if (DebugState.ShouldPauseInSubmit()) {
        DebugState.PauseGuestThreads();
    }

    auto vqid = gnm_vqid - 1;
    auto& asc_queue = asc_queues[{vqid}];
    const auto* acb_ptr = reinterpret_cast<const u32*>(asc_queue.map_addr + *asc_queue.read_addr);
    const auto acb_size = next_offs_dw ? (next_offs_dw << 2u) - *asc_queue.read_addr
                                       : (asc_queue.ring_size_dw << 2u) - *asc_queue.read_addr;
    const std::span acb_span{acb_ptr, acb_size >> 2u};

    if (DebugState.DumpingCurrentFrame()) {
        static auto last_frame_num = -1LL;
        static u32 seq_num{};
        if (last_frame_num == frames_submitted) {
            ++seq_num;
        } else {
            last_frame_num = frames_submitted;
            seq_num = 0u;
        }

        // Up to this point, all ACB submissions have been stored in a secondary command buffer.
        // Dumping them using the current ring pointer would result in files containing only the
        // `IndirectBuffer` command. To access the actual command stream, we need to unwrap the IB.
        auto acb = acb_span;
        const auto* indirect_buffer =
            reinterpret_cast<const PM4CmdIndirectBuffer*>(acb_span.data());
        if (indirect_buffer->header.opcode == PM4ItOpcode::IndirectBuffer) {
            acb = {indirect_buffer->Address<const u32>(), indirect_buffer->ib_size};
        }

        using namespace DebugStateType;

        DebugState.PushQueueDump({
            .type = QueueType::acb,
            .submit_num = seq_num,
            .num2 = gnm_vqid,
            .data = {acb.begin(), acb.end()},
        });
    }

    liverpool->SubmitAsc(vqid, acb_span);

    *asc_queue.read_addr += acb_size;
    *asc_queue.read_addr %= asc_queue.ring_size_dw * 4;
}

int PS4_SYSV_ABI sceGnmDingDongForWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDisableMipStatsReport() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmDispatchDirect(u32* cmdbuf, u32 size, u32 threads_x, u32 threads_y,
                                      u32 threads_z, u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 9) && ((s32)(threads_x | threads_y | threads_z) > -1)) {
        const auto predicate = flags & 1 ? PM4Predicate::PredEnable : PM4Predicate::PredDisable;
        cmdbuf = WriteHeader<PM4ItOpcode::DispatchDirect>(cmdbuf, 4, PM4ShaderType::ShaderCompute,
                                                          predicate);
        cmdbuf = WriteBody(cmdbuf, threads_x, threads_y, threads_z);
        cmdbuf[0] = (flags & 0x18) + 1; // ordered append mode

        WriteTrailingNop<3>(cmdbuf + 1);
        return ORBIS_OK;
    }
    return -1;
}

s32 PS4_SYSV_ABI sceGnmDispatchIndirect(u32* cmdbuf, u32 size, u32 data_offset, u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 7)) {
        const auto predicate = flags & 1 ? PM4Predicate::PredEnable : PM4Predicate::PredDisable;
        cmdbuf = WriteHeader<PM4ItOpcode::DispatchIndirect>(cmdbuf, 2, PM4ShaderType::ShaderCompute,
                                                            predicate);
        cmdbuf[0] = data_offset;
        cmdbuf[1] = (flags & 0x18) + 1; // ordered append mode

        WriteTrailingNop<3>(cmdbuf + 2);
        return ORBIS_OK;
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmDispatchIndirectOnMec() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceGnmDispatchInitDefaultHardwareState(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size < HwInitPacketSize) {
        return 0;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x216u,
                                     0xffffffffu); // COMPUTE_STATIC_THREAD_MGMT_SE0
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x217u,
                                     0xffffffffu);            // COMPUTE_STATIC_THREAD_MGMT_SE1
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x215u, 0x170u); // COMPUTE_RESOURCE_LIMITS

    cmdbuf = WriteHeader<PM4ItOpcode::AcquireMem>(cmdbuf, 6);
    cmdbuf = WriteBody(cmdbuf, 0x28000000u, 0u, 0u, 0u, 0u, 0u);

    cmdbuf = WriteHeader<PM4ItOpcode::Nop>(cmdbuf, 0xef);
    cmdbuf = WriteBody(cmdbuf, 0xau, 0u);
    return HwInitPacketSize;
}

s32 PS4_SYSV_ABI sceGnmDrawIndex(u32* cmdbuf, u32 size, u32 index_count, uintptr_t index_addr,
                                 u32 flags, u32 type) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 10) && (index_addr != 0) && (index_addr & 1) == 0 &&
        (flags & 0x1ffffffe) == 0) { // no predication will be set in the packet
        auto* draw_index = reinterpret_cast<PM4CmdDrawIndex2*>(cmdbuf);
        draw_index->header =
            PM4Type3Header{PM4ItOpcode::DrawIndex2, 4, PM4ShaderType::ShaderGraphics};
        draw_index->max_size = index_count;
        draw_index->index_base_lo = u32(index_addr);
        draw_index->index_base_hi = u32(index_addr >> 32);
        draw_index->index_count = index_count;
        draw_index->draw_initiator = 0;

        WriteTrailingNop<3>(cmdbuf + 6);
        return ORBIS_OK;
    }
    return -1;
}

s32 PS4_SYSV_ABI sceGnmDrawIndexAuto(u32* cmdbuf, u32 size, u32 index_count, u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 7) &&
        (flags & 0x1ffffffe) == 0) { // no predication will be set in the packet
        cmdbuf = WritePacket<PM4ItOpcode::DrawIndexAuto>(cmdbuf, PM4ShaderType::ShaderGraphics,
                                                         index_count, 2u);
        WriteTrailingNop<3>(cmdbuf);
        return ORBIS_OK;
    }
    return -1;
}

s32 PS4_SYSV_ABI sceGnmDrawIndexIndirect(u32* cmdbuf, u32 size, u32 data_offset, u32 shader_stage,
                                         u32 vertex_sgpr_offset, u32 instance_sgpr_offset,
                                         u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 9) && (shader_stage < ShaderStages::Max) &&
        (vertex_sgpr_offset < 0x10u) && (instance_sgpr_offset < 0x10u)) {

        const auto predicate = flags & 1 ? PM4Predicate::PredEnable : PM4Predicate::PredDisable;
        cmdbuf = WriteHeader<PM4ItOpcode::DrawIndexIndirect>(
            cmdbuf, 4, PM4ShaderType::ShaderGraphics, predicate);

        const auto sgpr_offset = indirect_sgpr_offsets[shader_stage];

        cmdbuf[0] = data_offset;
        cmdbuf[1] = vertex_sgpr_offset == 0 ? 0 : (vertex_sgpr_offset & 0xffffu) + sgpr_offset;
        cmdbuf[2] = instance_sgpr_offset == 0 ? 0 : (instance_sgpr_offset & 0xffffu) + sgpr_offset;
        cmdbuf[3] = 0;

        cmdbuf += 4;
        WriteTrailingNop<3>(cmdbuf);
        return ORBIS_OK;
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmDrawIndexIndirectCountMulti() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDrawIndexIndirectMulti() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDrawIndexMultiInstanced() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmDrawIndexOffset(u32* cmdbuf, u32 size, u32 index_offset, u32 index_count,
                                       u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 9)) {
        const auto predicate = flags & 1 ? PM4Predicate::PredEnable : PM4Predicate::PredDisable;
        cmdbuf = WriteHeader<PM4ItOpcode::DrawIndexOffset2>(
            cmdbuf, 4, PM4ShaderType::ShaderGraphics, predicate);
        cmdbuf = WriteBody(cmdbuf, index_count, index_offset, index_count, 0u);

        WriteTrailingNop<3>(cmdbuf);
        return ORBIS_OK;
    }
    return -1;
}

s32 PS4_SYSV_ABI sceGnmDrawIndirect(u32* cmdbuf, u32 size, u32 data_offset, u32 shader_stage,
                                    u32 vertex_sgpr_offset, u32 instance_sgpr_offset, u32 flags) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 9) && (shader_stage < ShaderStages::Max) &&
        (vertex_sgpr_offset < 0x10u) && (instance_sgpr_offset < 0x10u)) {

        const auto predicate = flags & 1 ? PM4Predicate::PredEnable : PM4Predicate::PredDisable;
        cmdbuf = WriteHeader<PM4ItOpcode::DrawIndirect>(cmdbuf, 4, PM4ShaderType::ShaderGraphics,
                                                        predicate);

        const auto sgpr_offset = indirect_sgpr_offsets[shader_stage];

        cmdbuf[0] = data_offset;
        cmdbuf[1] = vertex_sgpr_offset == 0 ? 0 : (vertex_sgpr_offset & 0xffffu) + sgpr_offset;
        cmdbuf[2] = instance_sgpr_offset == 0 ? 0 : (instance_sgpr_offset & 0xffffu) + sgpr_offset;
        cmdbuf[3] = 2; // auto index

        cmdbuf += 4;
        WriteTrailingNop<3>(cmdbuf);
        return ORBIS_OK;
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmDrawIndirectCountMulti() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDrawIndirectMulti() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size < HwInitPacketSize) {
        return 0;
    }

    const auto& SetupContext = [](u32* cmdbuf, u32 size, bool clear_state) {
        if (clear_state) {
            cmdbuf = ClearContextState(cmdbuf);
        }

        std::memcpy(cmdbuf, &InitSequence[2], (InitSequence.size() - 2) * 4);
        cmdbuf += InitSequence.size() - 2;

        const auto cmdbuf_left =
            HwInitPacketSize - (InitSequence.size() - 2) - (clear_state ? 0xc : 0) - 1;
        cmdbuf = WriteHeader<PM4ItOpcode::Nop>(cmdbuf, cmdbuf_left);
        cmdbuf = WriteBody(cmdbuf, 0u);

        return HwInitPacketSize;
    };

    return SetupContext(cmdbuf, size, true);
}

u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState175(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size < HwInitPacketSize) {
        return 0;
    }

    cmdbuf = ClearContextState(cmdbuf);
    std::memcpy(cmdbuf, &InitSequence175[2], (InitSequence175.size() - 2) * 4);
    cmdbuf += InitSequence175.size() - 2;

    constexpr auto cmdbuf_left = HwInitPacketSize - (InitSequence175.size() - 2) - 0xc - 1;
    WriteTrailingNop<cmdbuf_left>(cmdbuf);

    return HwInitPacketSize;
}

u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState200(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size < HwInitPacketSize) {
        return 0;
    }

    const auto& SetupContext200 = [](u32* cmdbuf, u32 size, bool clear_state) {
        if (clear_state) {
            cmdbuf = ClearContextState(cmdbuf);
        }

        std::memcpy(cmdbuf, &InitSequence200[2], (InitSequence200.size() - 2) * 4);
        cmdbuf += InitSequence200.size() - 2;

        const auto cmdbuf_left =
            HwInitPacketSize - (InitSequence200.size() - 2) - (clear_state ? 0xc : 0) - 1;
        cmdbuf = WriteHeader<PM4ItOpcode::Nop>(cmdbuf, cmdbuf_left);
        cmdbuf = WriteBody(cmdbuf, 0u);

        return HwInitPacketSize;
    };

    return SetupContext200(cmdbuf, size, true);
}

u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState350(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size < HwInitPacketSize) {
        return 0;
    }

    const auto& SetupContext350 = [](u32* cmdbuf, u32 size, bool clear_state) {
        if (clear_state) {
            cmdbuf = ClearContextState(cmdbuf);
        }

        std::memcpy(cmdbuf, &InitSequence350[2], (InitSequence350.size() - 2) * 4);
        cmdbuf += InitSequence350.size() - 2;

        const auto cmdbuf_left =
            HwInitPacketSize - (InitSequence350.size() - 2) - (clear_state ? 0xc : 0) - 1;
        cmdbuf = WriteHeader<PM4ItOpcode::Nop>(cmdbuf, cmdbuf_left);
        cmdbuf = WriteBody(cmdbuf, 0u);

        return HwInitPacketSize;
    };

    return SetupContext350(cmdbuf, size, true);
}

u32 PS4_SYSV_ABI sceGnmDrawInitToDefaultContextState(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    constexpr auto CtxInitPacketSize = 0x20u;
    if (size != CtxInitPacketSize) {
        return 0;
    }

    std::memcpy(cmdbuf, CtxInitSequence.data(), CtxInitSequence.size() * 4);
    return CtxInitPacketSize;
}

u32 PS4_SYSV_ABI sceGnmDrawInitToDefaultContextState400(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    constexpr auto CtxInitPacketSize = 0x100u;
    if (size != CtxInitPacketSize) {
        return 0;
    }

    std::memcpy(cmdbuf, CtxInitSequence400.data(), CtxInitSequence400.size() * 4);
    return CtxInitPacketSize;
}

int PS4_SYSV_ABI sceGnmDrawOpaqueAuto() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

bool PS4_SYSV_ABI sceGnmDriverCaptureInProgress() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return false;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterface() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForGpuDebugger() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForGpuException() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForHDRScopes() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForReplay() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForResourceRegistration() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalRetrieveGnmInterfaceForValidation() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverInternalVirtualQuery() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverTraceInProgress() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDriverTriggerCapture() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmEndWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmFindResourcesPublic() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return ORBIS_GNM_ERROR_FAILURE; // not available in retail FW
}

void PS4_SYSV_ABI sceGnmFlushGarlic() {
    LOG_WARNING(Lib_GnmDriver, "(STUBBED) called");
}

int PS4_SYSV_ABI sceGnmGetCoredumpAddress() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetCoredumpMode() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetCoredumpProtectionFaultTimestamp() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetDbgGcHandle() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetDebugTimestamp() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetEqEventType() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetEqTimeStamp() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetGpuBlockStatus() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceGnmGetGpuCoreClockFrequency() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return Config::isNeoMode() ? 911'000'000 : 800'000'000;
}

int PS4_SYSV_ABI sceGnmGetGpuInfoStatus() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetLastWaitedAddress() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetNumTcaUnits() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetOffChipTessellationBufferSize() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetOwnerName() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetPhysicalCounterFromVirtualized() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetProtectionFaultTimeStamp() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceBaseAddressAndSizeInBytes() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceName() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceShaderGuid() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceType() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceUserData() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetShaderProgramBaseAddress() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetShaderStatus() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

VAddr PS4_SYSV_ABI sceGnmGetTheTessellationFactorRingBufferBaseAddress() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return tessellation_factors_ring_addr;
}

int PS4_SYSV_ABI sceGnmGpuPaDebugEnter() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGpuPaDebugLeave() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmInsertDingDongMarker() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmInsertPopMarker(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && (size == 6)) {
        cmdbuf =
            WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics,
                                          PM4CmdNop::PayloadType::DebugMarkerPop, 0u, 0u, 0u, 0u);
        return ORBIS_OK;
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmInsertPushColorMarker() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmInsertPushMarker(u32* cmdbuf, u32 size, const char* marker) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (cmdbuf && marker) {
        const auto len = std::strlen(marker);
        const u32 packet_size = ((len + 8) >> 2) + ((len + 0xc) >> 3) * 2;
        if (packet_size + 2 == size) {
            auto* nop = reinterpret_cast<PM4CmdNop*>(cmdbuf);
            nop->header =
                PM4Type3Header{PM4ItOpcode::Nop, packet_size, PM4ShaderType::ShaderGraphics};
            nop->data_block[0] = PM4CmdNop::PayloadType::DebugMarkerPush;
            const auto marker_len = len + 1;
            std::memcpy(&nop->data_block[1], marker, marker_len);
            std::memset(reinterpret_cast<u8*>(&nop->data_block[1]) + marker_len, 0,
                        packet_size * 4 - marker_len);
            return ORBIS_OK;
        }
    }
    return -1;
}

int PS4_SYSV_ABI sceGnmInsertSetColorMarker() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmInsertSetMarker() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmInsertThreadTraceMarker() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmInsertWaitFlipDone(u32* cmdbuf, u32 size, s32 vo_handle, u32 buf_idx) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (size != 7) {
        return -1;
    }

    uintptr_t label_addr{};
    VideoOut::sceVideoOutGetBufferLabelAddress(vo_handle, &label_addr);

    auto* wait_reg_mem = reinterpret_cast<PM4CmdWaitRegMem*>(cmdbuf);
    wait_reg_mem->header = PM4Type3Header{PM4ItOpcode::WaitRegMem, 5};
    wait_reg_mem->raw = 0x13u;
    *reinterpret_cast<uintptr_t*>(&wait_reg_mem->poll_addr_lo) =
        (label_addr + buf_idx * sizeof(uintptr_t)) & ~0x3ull;
    wait_reg_mem->ref = 0u;
    wait_reg_mem->mask = 0xffff'ffffu;
    wait_reg_mem->poll_interval = 10u;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmIsCoredumpValid() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmIsUserPaEnabled() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return 0; // PA Debug is always disabled in retail FW
}

int PS4_SYSV_ABI sceGnmLogicalCuIndexToPhysicalCuIndex() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmLogicalCuMaskToPhysicalCuMask() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmLogicalTcaUnitToPhysical() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmMapComputeQueue(u32 pipe_id, u32 queue_id, VAddr ring_base_addr,
                                       u32 ring_size_dw, u32* read_ptr_addr) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (pipe_id >= Liverpool::NumComputePipes) {
        return ORBIS_GNM_ERROR_COMPUTEQUEUE_INVALID_PIPE_ID;
    }

    if (queue_id >= Liverpool::NumQueuesPerPipe) {
        return ORBIS_GNM_ERROR_COMPUTEQUEUE_INVALID_QUEUE_ID;
    }

    if (VAddr(ring_base_addr) % 256 != 0) { // alignment check
        return ORBIS_GNM_ERROR_COMPUTEQUEUE_INVALID_RING_BASE_ADDR;
    }

    if (!std::has_single_bit(ring_size_dw)) {
        return ORBIS_GNM_ERROR_COMPUTEQUEUE_INVALID_RING_SIZE;
    }

    if (VAddr(read_ptr_addr) % 4 != 0) { // alignment check
        return ORBIS_GNM_ERROR_COMPUTEQUEUE_INVALID_READ_PTR_ADDR;
    }

    auto vqid = asc_queues.insert(VAddr(ring_base_addr), read_ptr_addr, ring_size_dw);
    // We need to offset index as `dingDong` assumes it to be from the range [1..64]
    const auto gnm_vqid = vqid.index + 1;
    LOG_INFO(Lib_GnmDriver, "ASC pipe {} queue {} mapped to vqueue {}", pipe_id, queue_id,
             gnm_vqid);

    return gnm_vqid;
}

int PS4_SYSV_ABI sceGnmMapComputeQueueWithPriority(u32 pipe_id, u32 queue_id, VAddr ring_base_addr,
                                                   u32 ring_size_dw, u32* read_ptr_addr,
                                                   u32 pipePriority) {
    LOG_TRACE(Lib_GnmDriver, "called");

    (void)pipePriority;
    return sceGnmMapComputeQueue(pipe_id, queue_id, ring_base_addr, ring_size_dw, read_ptr_addr);
}

int PS4_SYSV_ABI sceGnmPaDisableFlipCallbacks() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmPaEnableFlipCallbacks() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmPaHeartbeat() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmQueryResourceRegistrationUserMemoryRequirements() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRaiseUserExceptionEvent() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRegisterGdsResource() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRegisterGnmLiveCallbackConfig() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmRegisterOwner(void* handle, const char* name) {
    LOG_TRACE(Lib_GnmDriver, "called");
    return ORBIS_GNM_ERROR_FAILURE; // PA Debug is always disabled in retail FW
}

s32 PS4_SYSV_ABI sceGnmRegisterResource(void* res_handle, void* owner_handle, const void* addr,
                                        size_t size, const char* name, int res_type,
                                        u64 user_data) {
    LOG_TRACE(Lib_GnmDriver, "called");
    return ORBIS_GNM_ERROR_FAILURE; // PA Debug is always disabled in retail FW
}

int PS4_SYSV_ABI sceGnmRequestFlipAndSubmitDone() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRequestFlipAndSubmitDoneForWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRequestMipStatsReportAndReset() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmResetVgtControl(u32* cmdbuf, u32 size) {
    LOG_TRACE(Lib_GnmDriver, "called");
    if (cmdbuf == nullptr || size != 3) {
        return -1;
    }
    PM4CmdSetData::SetContextReg(cmdbuf, 0x2aau, 0xffu); // IA_MULTI_VGT_PARAM
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaClose() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaConstFill() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaCopyLinear() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaCopyTiled() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaCopyWindow() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaFlush() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaGetMinCmdSize() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSdmaOpen() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetCsShader(u32* cmdbuf, u32 size, const u32* cs_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x18) {
        return -1;
    }
    if (!cs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer in shader registers.");
        return -1;
    }
    if (cs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
        return -1;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x20cu, cs_regs[0],
                                     0u); // COMPUTE_PGM_LO/COMPUTE_PGM_HI
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x212u, cs_regs[2],
                                     cs_regs[3]); // COMPUTE_PGM_RSRC1/COMPUTE_PGM_RSRC2
    cmdbuf = PM4CmdSetData::SetShReg(
        cmdbuf, 0x207u, cs_regs[4], cs_regs[5],
        cs_regs[6]); // COMPUTE_NUM_THREAD_X/COMPUTE_NUM_THREAD_Y/COMPUTE_NUM_THREAD_Z

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetCsShaderWithModifier(u32* cmdbuf, u32 size, const u32* cs_regs,
                                               u32 modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x18) {
        return -1;
    }
    if (!cs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer in shader registers.");
        return -1;
    }
    if ((modifier & 0xfffffc3fu) != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid modifier mask.");
        return -1;
    }
    if (cs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
        return -1;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x20cu, cs_regs[0],
                                     0u); // COMPUTE_PGM_LO/COMPUTE_PGM_HI
    const u32 rsrc1 = modifier == 0 ? cs_regs[2] : (cs_regs[2] & 0xfffffc3fu) | modifier;
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x212u, rsrc1,
                                     cs_regs[3]); // COMPUTE_PGM_RSRC1/COMPUTE_PGM_RSRC2
    cmdbuf = PM4CmdSetData::SetShReg(
        cmdbuf, 0x207u, cs_regs[4], cs_regs[5],
        cs_regs[6]); // COMPUTE_NUM_THREAD_X/COMPUTE_NUM_THREAD_Y/COMPUTE_NUM_THREAD_Z

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetEmbeddedPsShader(u32* cmdbuf, u32 size, u32 shader_id,
                                           u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (shader_id > 1) {
        LOG_ERROR(Lib_GnmDriver, "Unknown shader id {}", shader_id);
        return ORBIS_GNM_ERROR_FAILURE;
    }

    // clang-format off
    constexpr static std::array ps0_code alignas(256) = {
        0xbeeb03ffu, 0x00000003u, // s_mov_b32     vcc_hi, $0x00000003
        0x7e000280u,              // v_mov_b32     v0, 0
        0x5e000100u,              // v_cvt_pkrtz_f16_f32 v0, v0, v0
        0xbf800000u,              // s_nop
        0xf8001c0fu, 0x00000000u, // exp           mrt0, v0, v0 compr vm done
        0xbf810000u,              // s_endpgm

        // Binary header
        0x5362724fu, 0x07726468u, 0x00002043u, 0u, 0xb0a45b2bu, 0x1d39766du, 0x72044b7bu, 0x0000000fu,
        // PS regs
        0x0fe000f0u, 0u, 0xc0000u, 4u, 0u, 4u, 2u, 2u, 0u, 0u, 0x10u, 0xfu, 0xcu, 0u, 0u, 0u,
    };

    const auto shader0_addr = uintptr_t(ps0_code.data()); // Original address is 0xfe000f00
    const static u32 ps0_regs[] = {
        u32(shader0_addr >> 8), u32(shader0_addr >> 40), 0xc0000u, 4u, 0u, 4u, 2u, 2u, 0u, 0u, 0x10u, 0xfu, 0xcu};

    constexpr static std::array ps1_code alignas(256) = {
        0xbeeb03ffu, 0x00000003u, // s_mov_b32     vcc_hi, $0x00000003
        0x7e040280u,              // v_mov_b32     v2, 0 
        0xf8001803u, 0x02020202u, // exp           mrt0, v2, v2, off, off vm done
        0xbf810000u,              // s_endpgm

        // Binary header
        0x5362724fu, 0x07726468u, 0x00001841u, 0x04080002u, 0x98b9cb94u, 0u, 0x6f130734u, 0x0000000fu,
        // PS regs
        0x0fe000f2u, 0u, 0x2000u, 0u, 0u, 2u, 2u, 2u, 0u, 0u, 0x10u, 3u, 0xcu,
    };

    const auto shader1_addr = uintptr_t(ps1_code.data()); // Original address is 0xfe000f20
    const static u32 ps1_regs[] = {
        u32(shader1_addr >> 8), u32(shader1_addr >> 40), 0x2000u, 0u, 0u, 2u, 2u, 2u, 0u, 0u, 0x10u, 3u, 0xcu};
    // clang-format on

    const auto ps_regs = shader_id == 0 ? ps0_regs : ps1_regs;

    // Normally the driver will do a call to `sceGnmSetPsShader350()`, but this function has
    // a check for zero in the upper part of shader address. In our case, the address is a
    // pointer to a stack memory, so the check will likely fail. To workaround it we will
    // repeat set shader functionality here as it is trivial.
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, ps_regs[0],
                                     0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 10u, ps_regs[2],
                                     ps_regs[3]); // SPI_SHADER_PGM_RSRC1_PS/SPI_SHADER_PGM_RSRC2_PS
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1c4u, ps_regs[4],
                                          ps_regs[5]); // SPI_SHADER_Z_FORMAT/SPI_SHADER_COL_FORMAT
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b3u, ps_regs[6],
                                          ps_regs[7]); // SPI_PS_INPUT_ENA/SPI_PS_INPUT_ADDR
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b6u, ps_regs[8]);  // SPI_PS_IN_CONTROL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b8u, ps_regs[9]);  // SPI_BARYC_CNTL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x203u, ps_regs[10]); // DB_SHADER_CONTROL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x8fu, ps_regs[11]);  // CB_SHADER_MASK

    WriteTrailingNop<11>(cmdbuf);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetEmbeddedVsShader(u32* cmdbuf, u32 size, u32 shader_id,
                                           u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (shader_id != 0) {
        LOG_ERROR(Lib_GnmDriver, "Unknown shader id {}", shader_id);
        return ORBIS_GNM_ERROR_FAILURE;
    }

    // A fullscreen triangle with one uv set
    // clang-format off
    constexpr static std::array shader_code alignas(256) = {
        0xbeeb03ffu, 0x00000007u, // s_mov_b32     vcc_hi, $0x00000007
        0x36020081u,              // v_and_b32     v1, 1, v0
        0x34020281u,              // v_lshlrev_b32 v1, 1, v1
        0x360000c2u,              // v_and_b32     v0, -2, v0
        0x4a0202c1u,              // v_add_i32     v1, vcc, -1, v1
        0x4a0000c1u,              // v_add_i32     v0, vcc, -1, v0
        0x7e020b01u,              // v_cvt_f32_i32 v1, v1
        0x7e000b00U,              // v_cvt_f32_i32 v0, v0
        0x7e040280u,              // v_mov_b32     v2, 0
        0x7e0602f2u,              // v_mov_b32     v3, 1.0
        0xf80008cfu, 0x03020001u, // exp           pos0, v1, v0, v2, v3 done
        0xf800020fu, 0x03030303u, // exp           param0, v3, v3, v3, v3
        0xbf810000u,              // s_endpgm

        // Binary header
        0x5362724fu, 0x07726468u, 0x00004047u, 0u, 0x47f8c29fu, 0x9b2da5cfu, 0xff7c5b7du, 0x00000017u,
        // VS regs
        0x0fe000f1u, 0u, 0x000c0000u, 4u, 0u, 4u, 0u, 7u,
    };
    // clang-format on

    const auto shader_addr = uintptr_t(shader_code.data()); // Original address is 0xfe000f10
    const static u32 vs_regs[] = {
        u32(shader_addr >> 8), u32(shader_addr >> 40), 0xc0000u, 4, 0, 4, 0, 7};

    // Normally the driver will do a call to `sceGnmSetVsShader()`, but this function has
    // a check for zero in the upper part of shader address. In our case, the address is a
    // pointer to a stack memory, so the check will likely fail. To workaround it we will
    // repeat set shader functionality here as it is trivial.
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x48u, vs_regs[0], vs_regs[1]); // SPI_SHADER_PGM_LO_VS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x4au, vs_regs[2],
                                     vs_regs[3]); // SPI_SHADER_PGM_RSRC1_VS/SPI_SHADER_PGM_RSRC2_VS
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x207u, vs_regs[6]); // PA_CL_VS_OUT_CNTL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b1u, vs_regs[4]); // SPI_VS_OUT_CONFIG
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1c3u, vs_regs[5]); // SPI_SHADER_POS_FORMAT

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetEsShader(u32* cmdbuf, u32 size, const u32* es_regs, u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size < 0x14) {
        return -1;
    }

    if (!es_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (shader_modifier & 0xfcfffc3f) {
        LOG_ERROR(Lib_GnmDriver, "Invalid modifier mask");
        return -1;
    }

    if (es_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    const u32 var =
        shader_modifier == 0 ? es_regs[2] : ((es_regs[2] & 0xfcfffc3f) | shader_modifier);
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0xc8u, es_regs[0], 0u);  // SPI_SHADER_PGM_LO_ES
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0xcau, var, es_regs[3]); // SPI_SHADER_PGM_RSRC1_ES

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetGsRingSizes() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetGsShader(u32* cmdbuf, u32 size, const u32* gs_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size < 0x1d) {
        return -1;
    }

    if (!gs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (gs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x88u, gs_regs[0], 0u); // SPI_SHADER_PGM_LO_GS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x8au, gs_regs[2],
                                     gs_regs[3]); // SPI_SHADER_PGM_RSRC1_GS/SPI_SHADER_PGM_RSRC2_GS
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x2e5u, gs_regs[4]); // VGT_STRMOUT_CONFIG
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x29bu, gs_regs[5]); // VGT_GS_OUT_PRIM_TYPE
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x2e4u, gs_regs[6]); // VGT_GS_INSTANCE_CNT

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetHsShader(u32* cmdbuf, u32 size, const u32* hs_regs, u32 param4) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size < 0x1E) {
        return -1;
    }

    if (!hs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (hs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x108u, hs_regs[0], 0u); // SPI_SHADER_PGM_LO_HS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x10au, hs_regs[2],
                                     hs_regs[3]); // SPI_SHADER_PGM_RSRC1_HS/SPI_SHADER_PGM_RSRC2_HS
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x286u, hs_regs[5],
                                          hs_regs[5]);                 // VGT_HOS_MAX_TESS_LEVEL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x2dbu, hs_regs[4]); // VGT_TF_PARAM
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x2d6u, param4);     // VGT_LS_HS_CONFIG

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetLsShader(u32* cmdbuf, u32 size, const u32* ls_regs, u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size < 0x17) {
        return -1;
    }

    if (!ls_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    const auto modifier_mask = ((shader_modifier & 0xfffffc3f) == 0) ? 0xfffffc3f : 0xfcfffc3f;
    if (shader_modifier & modifier_mask) {
        LOG_ERROR(Lib_GnmDriver, "Invalid modifier mask");
        return -1;
    }

    if (ls_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    const u32 var =
        shader_modifier == 0 ? ls_regs[2] : ((ls_regs[2] & modifier_mask) | shader_modifier);
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x148u, ls_regs[0], 0u);  // SPI_SHADER_PGM_LO_LS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x14bu, ls_regs[3]);      // SPI_SHADER_PGM_RSRC2_LS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x14au, var, ls_regs[3]); // SPI_SHADER_PGM_RSRC1_LS

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetPsShader(u32* cmdbuf, u32 size, const u32* ps_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x27) {
        return -1;
    }
    if (!ps_regs) {
        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, 0u,
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x203u, 0u); // DB_SHADER_CONTROL

        WriteTrailingNop<0x20>(cmdbuf);
    } else {
        if (ps_regs[1] != 0) {
            LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
            return -1;
        }

        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, ps_regs[0],
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf =
            PM4CmdSetData::SetShReg(cmdbuf, 10u, ps_regs[2],
                                    ps_regs[3]); // SPI_SHADER_PGM_RSRC1_PS/SPI_SHADER_PGM_RSRC2_PS
        cmdbuf = PM4CmdSetData::SetContextReg(
            cmdbuf, 0x1c4u, ps_regs[4], ps_regs[5]); // SPI_SHADER_Z_FORMAT/SPI_SHADER_COL_FORMAT
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b3u, ps_regs[6],
                                              ps_regs[7]); // SPI_PS_INPUT_ENA/SPI_PS_INPUT_ADDR
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b6u, ps_regs[8]);  // SPI_PS_IN_CONTROL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b8u, ps_regs[9]);  // SPI_BARYC_CNTL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x203u, ps_regs[10]); // DB_SHADER_CONTROL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x8fu, ps_regs[11]);  // CB_SHADER_MASK

        WriteTrailingNop<11>(cmdbuf);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetPsShader350(u32* cmdbuf, u32 size, const u32* ps_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x27) {
        return -1;
    }
    if (!ps_regs) {
        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, 0u,
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x203u, 0u);  // DB_SHADER_CONTROL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x8fu, 0xfu); // CB_SHADER_MASK

        WriteTrailingNop<0x1d>(cmdbuf);
    } else {
        if (ps_regs[1] != 0) {
            LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
            return -1;
        }

        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, ps_regs[0],
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf =
            PM4CmdSetData::SetShReg(cmdbuf, 10u, ps_regs[2],
                                    ps_regs[3]); // SPI_SHADER_PGM_RSRC1_PS/SPI_SHADER_PGM_RSRC2_PS
        cmdbuf = PM4CmdSetData::SetContextReg(
            cmdbuf, 0x1c4u, ps_regs[4], ps_regs[5]); // SPI_SHADER_Z_FORMAT/SPI_SHADER_COL_FORMAT
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b3u, ps_regs[6],
                                              ps_regs[7]); // SPI_PS_INPUT_ENA/SPI_PS_INPUT_ADDR
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b6u, ps_regs[8]);  // SPI_PS_IN_CONTROL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b8u, ps_regs[9]);  // SPI_BARYC_CNTL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x203u, ps_regs[10]); // DB_SHADER_CONTROL
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x8fu, ps_regs[11]);  // CB_SHADER_MASK

        WriteTrailingNop<11>(cmdbuf);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetResourceRegistrationUserMemory() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetResourceUserData() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetSpiEnableSqCounters() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetSpiEnableSqCountersForUnitInstance() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetupMipStatsReport() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetVgtControl(u32* cmdbuf, u32 size, u32 prim_group_sz_minus_one,
                                     u32 partial_vs_wave_mode, u32 wd_switch_only_on_eop_mode) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size != 3 || (prim_group_sz_minus_one >= 0x100) ||
        ((wd_switch_only_on_eop_mode | partial_vs_wave_mode) >= 2)) {
        return -1;
    }

    const u32 reg_value =
        ((partial_vs_wave_mode & 1) << 0x10) | (prim_group_sz_minus_one & 0xffffu);
    PM4CmdSetData::SetContextReg(cmdbuf, 0x2aau, reg_value); // IA_MULTI_VGT_PARAM
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSetVsShader(u32* cmdbuf, u32 size, const u32* vs_regs, u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x1c) {
        return -1;
    }

    if (!vs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (shader_modifier & 0xfcfffc3f) {
        LOG_ERROR(Lib_GnmDriver, "Invalid modifier mask");
        return -1;
    }

    if (vs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    const u32 var = shader_modifier == 0 ? vs_regs[2] : (vs_regs[2] & 0xfcfffc3f) | shader_modifier;
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x48u, vs_regs[0], 0u);   // SPI_SHADER_PGM_LO_VS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x4au, var, vs_regs[3]);  // SPI_SHADER_PGM_RSRC1_VS
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x207u, vs_regs[6]); // PA_CL_VS_OUT_CNTL
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1b1u, vs_regs[4]); // SPI_VS_OUT_CONFIG
    cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x1c3u, vs_regs[5]); // SPI_SHADER_POS_FORMAT

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetWaveLimitMultiplier() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSetWaveLimitMultipliers() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmEndSpm() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmInit() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmInit2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetDelay() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetMuxRam() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetMuxRam2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetSelectCounter() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetSpmSelects() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmSetSpmSelects2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSpmStartSpm() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttFini() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttFinishTrace() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetBcInfo() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetGpuClocks() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetHiWater() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetStatus() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetTraceCounter() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetTraceWptr() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetWrapCounts() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetWrapCounts2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttGetWritebackLabels() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttInit() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSelectMode() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSelectTarget() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSelectTokens() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetCuPerfMask() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetDceEventWrite() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetHiWater() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetTraceBuffer2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetTraceBuffers() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetUserData() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSetUserdataTimer() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttStartTrace() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttStopTrace() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSwitchTraceBuffer() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttSwitchTraceBuffer2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSqttWaitForEvent() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

static inline s32 PatchFlipRequest(u32* cmdbuf, u32 size, u32 vo_handle, u32 buf_idx, u32 flip_mode,
                                   u32 flip_arg, void* unk) {
    // check for `prepareFlip` packet
    cmdbuf += size - 64;
    ASSERT_MSG(cmdbuf[0] == 0xc03e1000, "Can't find `prepareFlip` packet");

    std::array<u32, 7> backup{};
    std::memcpy(backup.data(), cmdbuf, backup.size() * sizeof(decltype(backup)::value_type));

    ASSERT_MSG(((backup[2] & 3) == 0u) || (backup[1] != PM4CmdNop::PayloadType::PrepareFlipLabel),
               "Invalid flip packet");
    ASSERT_MSG(buf_idx != 0xffff'ffffu, "Invalid VO buffer index");

    const s32 flip_result = VideoOut::sceVideoOutSubmitEopFlip(vo_handle, buf_idx, flip_mode,
                                                               flip_arg, nullptr /*unk*/);
    if (flip_result != 0) {
        if (flip_result == 0x80290012) {
            LOG_ERROR(Lib_GnmDriver, "Flip queue is full");
            return 0x80d11081;
        } else {
            LOG_ERROR(Lib_GnmDriver, "Flip request failed");
            return flip_result;
        }
    }

    uintptr_t label_addr{};
    VideoOut::sceVideoOutGetBufferLabelAddress(vo_handle, &label_addr);

    // Write event to lock the VO surface
    auto* write_lock = reinterpret_cast<PM4CmdWriteData*>(cmdbuf);
    write_lock->header = PM4Type3Header{PM4ItOpcode::WriteData, 3};
    write_lock->raw = 0x500u;
    const auto addr = (label_addr + buf_idx * sizeof(label_addr)) & ~0x3ull;
    write_lock->Address<uintptr_t>(addr);
    write_lock->data[0] = 1;

    auto* nop = reinterpret_cast<PM4CmdNop*>(cmdbuf + 5);

    if (backup[1] == PM4CmdNop::PayloadType::PrepareFlip) {
        nop->header = PM4Type3Header{PM4ItOpcode::Nop, 0x39};
        nop->data_block[0] = PM4CmdNop::PayloadType::PatchedFlip;
    } else {
        if (backup[1] == PM4CmdNop::PayloadType::PrepareFlipLabel) {
            nop->header = PM4Type3Header{PM4ItOpcode::Nop, 0x34};
            nop->data_block[0] = PM4CmdNop::PayloadType::PatchedFlip;

            // Write event to update label
            auto* write_label = reinterpret_cast<PM4CmdWriteData*>(cmdbuf + 0x3b);
            write_label->header = PM4Type3Header{PM4ItOpcode::WriteData, 3};
            write_label->raw = 0x500u;
            write_label->dst_addr_lo = backup[2] & 0xffff'fffcu;
            write_label->dst_addr_hi = backup[3];
            write_label->data[0] = backup[4];
        }
        if (backup[1] == PM4CmdNop::PayloadType::PrepareFlipInterruptLabel) {
            nop->header = PM4Type3Header{PM4ItOpcode::Nop, 0x33};
            nop->data_block[0] = PM4CmdNop::PayloadType::PatchedFlip;

            auto* write_eop = reinterpret_cast<PM4CmdEventWriteEop*>(cmdbuf + 0x3a);
            write_eop->header = PM4Type3Header{PM4ItOpcode::EventWriteEop, 4};
            write_eop->event_control = (backup[5] & 0x3f) + 0x500u + (backup[6] & 0x3f) * 0x1000;
            write_eop->address_lo = backup[2] & 0xffff'fffcu;
            write_eop->data_control = (backup[3] & 0xffffu) | 0x2200'0000u;
            write_eop->data_lo = backup[4];
            write_eop->data_hi = 0u;
        }
        if (backup[1] == PM4CmdNop::PayloadType::PrepareFlipInterrupt) {
            nop->header = PM4Type3Header{PM4ItOpcode::Nop, 0x33};
            nop->data_block[0] = PM4CmdNop::PayloadType::PatchedFlip;

            auto* write_eop = reinterpret_cast<PM4CmdEventWriteEop*>(cmdbuf + 0x3a);
            write_eop->header = PM4Type3Header{PM4ItOpcode::EventWriteEop, 4};
            write_eop->event_control = (backup[5] & 0x3f) + 0x500u + (backup[6] & 0x3f) * 0x1000;
            write_eop->address_lo = 0u;
            write_eop->data_control = 0x100'0000u;
            write_eop->data_lo = 0u;
            write_eop->data_hi = 0u;
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSubmitAndFlipCommandBuffers(u32 count, u32* dcb_gpu_addrs[],
                                                   u32* dcb_sizes_in_bytes, u32* ccb_gpu_addrs[],
                                                   u32* ccb_sizes_in_bytes, u32 vo_handle,
                                                   u32 buf_idx, u32 flip_mode, u32 flip_arg) {
    LOG_DEBUG(Lib_GnmDriver, "called [buf = {}]", buf_idx);

    auto* cmdbuf = dcb_gpu_addrs[count - 1];
    const auto size_dw = dcb_sizes_in_bytes[count - 1] / 4;

    const s32 patch_result =
        PatchFlipRequest(cmdbuf, size_dw, vo_handle, buf_idx, flip_mode, flip_arg, nullptr /*unk*/);
    if (patch_result != ORBIS_OK) {
        return patch_result;
    }

    return sceGnmSubmitCommandBuffers(count, const_cast<const u32**>(dcb_gpu_addrs),
                                      dcb_sizes_in_bytes, const_cast<const u32**>(ccb_gpu_addrs),
                                      ccb_sizes_in_bytes);
}

int PS4_SYSV_ABI sceGnmSubmitAndFlipCommandBuffersForWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmSubmitCommandBuffers(u32 count, const u32* dcb_gpu_addrs[],
                                            u32* dcb_sizes_in_bytes, const u32* ccb_gpu_addrs[],
                                            u32* ccb_sizes_in_bytes) {
    LOG_DEBUG(Lib_GnmDriver, "called");

    if (!dcb_gpu_addrs || !dcb_sizes_in_bytes) {
        LOG_ERROR(Lib_GnmDriver, "dcbGpuAddrs and dcbSizesInBytes must not be NULL");
        return 0x80d11000;
    }

    for (u32 i = 0; i < count; i++) {
        if (dcb_sizes_in_bytes[i] == 0) {
            LOG_ERROR(Lib_GnmDriver, "Submitting a null DCB {}", i);
            return 0x80d11000;
        }
        if (dcb_sizes_in_bytes[i] > 0x3ffffc) {
            LOG_ERROR(Lib_GnmDriver, "dcbSizesInBytes[{}] ({}) is limited to (2*20)-1 DWORDS", i,
                      dcb_sizes_in_bytes[i]);
            return 0x80d11000;
        }
        if (ccb_sizes_in_bytes && ccb_sizes_in_bytes[i] > 0x3ffffc) {
            LOG_ERROR(Lib_GnmDriver, "ccbSizesInBytes[{}] ({}) is limited to (2*20)-1 DWORDS", i,
                      ccb_sizes_in_bytes[i]);
            return 0x80d11000;
        }
    }

    WaitGpuIdle();

    if (DebugState.ShouldPauseInSubmit()) {
        DebugState.PauseGuestThreads();
    }

    if (send_init_packet) {
        if (sdk_version <= 0x1ffffffu) {
            liverpool->SubmitGfx(InitSequence, {});
        } else if (sdk_version <= 0x3ffffffu) {
            liverpool->SubmitGfx(InitSequence200, {});
        } else {
            liverpool->SubmitGfx(InitSequence350, {});
        }
        send_init_packet = false;
    }

    for (auto cbpair = 0u; cbpair < count; ++cbpair) {
        const auto* ccb = ccb_gpu_addrs ? ccb_gpu_addrs[cbpair] : nullptr;
        const auto ccb_size_in_bytes = ccb_sizes_in_bytes ? ccb_sizes_in_bytes[cbpair] : 0;

        const auto dcb_size_dw = dcb_sizes_in_bytes[cbpair] >> 2;
        const auto ccb_size_dw = ccb_size_in_bytes >> 2;

        const auto& dcb_span = std::span{dcb_gpu_addrs[cbpair], dcb_size_dw};
        const auto& ccb_span = std::span{ccb, ccb_size_dw};

        if (DebugState.DumpingCurrentFrame()) {
            static auto last_frame_num = -1LL;
            static u32 seq_num{};
            if (last_frame_num == frames_submitted && cbpair == 0) {
                ++seq_num;
            } else {
                last_frame_num = frames_submitted;
                seq_num = 0u;
            }

            using DebugStateType::QueueType;

            DebugState.PushQueueDump({
                .type = QueueType::dcb,
                .submit_num = seq_num,
                .num2 = cbpair,
                .data = {dcb_span.begin(), dcb_span.end()},
            });
            DebugState.PushQueueDump({
                .type = QueueType::ccb,
                .submit_num = seq_num,
                .num2 = cbpair,
                .data = {ccb_span.begin(), ccb_span.end()},
            });
        }

        liverpool->SubmitGfx(dcb_span, ccb_span);
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSubmitCommandBuffersForWorkload() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmSubmitDone() {
    LOG_DEBUG(Lib_GnmDriver, "called");
    WaitGpuIdle();
    if (!liverpool->IsGpuIdle()) {
        submission_lock = true;
    }
    liverpool->SubmitDone();
    send_init_packet = true;
    ++frames_submitted;
    DebugState.IncGnmFrameNum();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmUnmapComputeQueue() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmUnregisterAllResourcesForOwner() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmUnregisterOwnerAndResources() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmUnregisterResource() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmUpdateGsShader(u32* cmdbuf, u32 size, const u32* gs_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size < 0x1d) {
        return -1;
    }

    if (!gs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (gs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x88u, gs_regs[0], 0u); // SPI_SHADER_PGM_LO_GS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x8au, gs_regs[2],
                                     gs_regs[3]); // SPI_SHADER_PGM_RSRC1_GS/SPI_SHADER_PGM_RSRC2_GS
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e02e5u,
                                           gs_regs[4]);
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e029bu,
                                           gs_regs[5]);
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e02e4u,
                                           gs_regs[6]);

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmUpdateHsShader() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmUpdatePsShader(u32* cmdbuf, u32 size, const u32* ps_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x27) {
        return -1;
    }
    if (!ps_regs) {
        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, 0u,
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e0203u,
                                               0u); // DB_SHADER_CONTROL update
        WriteTrailingNop<0x20>(cmdbuf);
    } else {
        if (ps_regs[1] != 0) {
            LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
            return -1;
        }

        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, ps_regs[0],
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf =
            PM4CmdSetData::SetShReg(cmdbuf, 10u, ps_regs[2],
                                    ps_regs[3]); // SPI_SHADER_PGM_RSRC1_PS/SPI_SHADER_PGM_RSRC2_PS
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(
            cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01c4u, ps_regs[4],
            ps_regs[5]); // SPI_SHADER_Z_FORMAT/SPI_SHADER_COL_FORMAT update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(
            cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b3u, ps_regs[6],
            ps_regs[7]); // SPI_PS_INPUT_ENA/SPI_PS_INPUT_ADDR update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b6u,
                                               ps_regs[8]); // SPI_PS_IN_CONTROL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b8u,
                                               ps_regs[9]); // SPI_BARYC_CNTL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e0203u,
                                               ps_regs[10]); // DB_SHADER_CONTROL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e008fu,
                                               ps_regs[11]); // CB_SHADER_MASK update

        WriteTrailingNop<11>(cmdbuf);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmUpdatePsShader350(u32* cmdbuf, u32 size, const u32* ps_regs) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x27) {
        return -1;
    }
    if (!ps_regs) {
        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, 0u,
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e0203u,
                                               0u);                 // DB_SHADER_CONTROL update
        cmdbuf = PM4CmdSetData::SetContextReg(cmdbuf, 0x8fu, 0xfu); // CB_SHADER_MASK

        WriteTrailingNop<0x1d>(cmdbuf);
    } else {
        if (ps_regs[1] != 0) {
            LOG_ERROR(Lib_GnmDriver, "Invalid shader address.");
            return -1;
        }

        cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 8u, ps_regs[0],
                                         0u); // SPI_SHADER_PGM_LO_PS/SPI_SHADER_PGM_HI_PS
        cmdbuf =
            PM4CmdSetData::SetShReg(cmdbuf, 10u, ps_regs[2],
                                    ps_regs[3]); // SPI_SHADER_PGM_RSRC1_PS/SPI_SHADER_PGM_RSRC2_PS
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(
            cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01c4u, ps_regs[4],
            ps_regs[5]); // SPI_SHADER_Z_FORMAT/SPI_SHADER_COL_FORMAT update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(
            cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b3u, ps_regs[6],
            ps_regs[7]); // SPI_PS_INPUT_ENA/SPI_PS_INPUT_ADDR update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b6u,
                                               ps_regs[8]); // SPI_PS_IN_CONTROL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b8u,
                                               ps_regs[9]); // SPI_BARYC_CNTL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e0203u,
                                               ps_regs[10]); // DB_SHADER_CONTROL update
        cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e008fu,
                                               ps_regs[11]); // CB_SHADER_MASK update

        WriteTrailingNop<11>(cmdbuf);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmUpdateVsShader(u32* cmdbuf, u32 size, const u32* vs_regs,
                                      u32 shader_modifier) {
    LOG_TRACE(Lib_GnmDriver, "called");

    if (!cmdbuf || size <= 0x1c) {
        return -1;
    }

    if (!vs_regs) {
        LOG_ERROR(Lib_GnmDriver, "Null pointer passed as argument");
        return -1;
    }

    if (shader_modifier & 0xfcfffc3f) {
        LOG_ERROR(Lib_GnmDriver, "Invalid modifier mask");
        return -1;
    }

    if (vs_regs[1] != 0) {
        LOG_ERROR(Lib_GnmDriver, "Invalid shader address");
        return -1;
    }

    const u32 var =
        shader_modifier == 0 ? vs_regs[2] : ((vs_regs[2] & 0xfcfffc3f) | shader_modifier);
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x48u, vs_regs[0], 0u);  // SPI_SHADER_PGM_LO_VS
    cmdbuf = PM4CmdSetData::SetShReg(cmdbuf, 0x4au, var, vs_regs[3]); // SPI_SHADER_PGM_RSRC1_VS
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e0207u,
                                           vs_regs[6]); // PA_CL_VS_OUT_CNTL update
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01b1u,
                                           vs_regs[4]); // PA_CL_VS_OUT_CNTL update
    cmdbuf = WritePacket<PM4ItOpcode::Nop>(cmdbuf, PM4ShaderType::ShaderGraphics, 0xc01e01c3u,
                                           vs_regs[5]); // PA_CL_VS_OUT_CNTL update

    WriteTrailingNop<11>(cmdbuf);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceGnmValidateCommandBuffers() {
    LOG_TRACE(Lib_GnmDriver, "called");
    return ORBIS_GNM_ERROR_VALIDATION_NOT_ENABLED; // not available in retail FW;
}

int PS4_SYSV_ABI sceGnmValidateDisableDiagnostics() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateDisableDiagnostics2() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateDispatchCommandBuffers() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateDrawCommandBuffers() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateGetDiagnosticInfo() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateGetDiagnostics() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateGetVersion() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateOnSubmitEnabled() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidateResetState() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmValidationRegisterMemoryCheckCallback() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRazorCaptureCommandBuffersOnlyImmediate() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRazorCaptureCommandBuffersOnlySinceLastFlip() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRazorCaptureImmediate() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRazorCaptureSinceLastFlip() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRazorIsLoaded() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_063D065A2D6359C3() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0CABACAFB258429D() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_150CF336FC2E99A3() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_17CA687F9EE52D49() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1870B89F759C6B45() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_26F9029EF68A955E() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_301E3DBBAB092DB0() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_30BAFE172AF17FEF() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3E6A3E8203D95317() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_40FEEF0C6534C434() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_416B9079DE4CBACE() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4774D83BB4DDBF9A() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_50678F1CCEEB9A00() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_54A2EC5FA4C62413() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5A9C52C83138AE6B() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5D22193A31EA1142() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_725A36DEBB60948D() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8021A502FA61B9BB() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9D002FE0FA40F0E6() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9D297F36A7028B71() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A2D7EC7A7BCF79B3() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AA12A3CB8990854A() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ADC8DDC005020BC6() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B0A8688B679CB42D() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B489020B5157A5FF() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BADE7B4C199140DD() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D1511B9DCFFB3DD9() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D53446649B02E58E() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D8B6E8E28E1EF0A3() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D93D733A19DD7454() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DE995443BC2A8317() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DF6E9528150C23FF() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ECB4C6BA41FE3350() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebugModuleReset() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDebugReset() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C4C328B7CF3B4171() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDrawInitToDefaultContextStateInternalCommand() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmDrawInitToDefaultContextStateInternalSize() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmFindResources() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmGetResourceRegistrationBuffers() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceGnmRegisterOwnerForSystem() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1C43886B16EE5530() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_81037019ECCD0E01() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BFB41C057478F0BF() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E51D44DB8151238C() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F916890425496553() {
    LOG_ERROR(Lib_GnmDriver, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceGnmDriver(Core::Loader::SymbolsResolver* sym) {
    LOG_INFO(Lib_GnmDriver, "Initializing renderer");
    liverpool = std::make_unique<AmdGpu::Liverpool>();
    renderer = std::make_unique<Vulkan::RendererVulkan>(*g_window, liverpool.get());

    const int result = sceKernelGetCompiledSdkVersion(&sdk_version);
    if (result != ORBIS_OK) {
        sdk_version = 0;
    }

    if (Config::copyGPUCmdBuffers()) {
        liverpool->reserveCopyBufferSpace();
    }

    Platform::IrqC::Instance()->Register(Platform::InterruptId::GpuIdle, ResetSubmissionLock,
                                         nullptr);

    LIB_FUNCTION("b0xyllnVY-I", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmAddEqEvent);
    LIB_FUNCTION("b08AgtPlHPg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmAreSubmitsAllowed);
    LIB_FUNCTION("ihxrbsoSKWc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmBeginWorkload);
    LIB_FUNCTION("ffrNQOshows", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmComputeWaitOnAddress);
    LIB_FUNCTION("EJapNl2+pgU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmComputeWaitSemaphore);
    LIB_FUNCTION("5udAm+6boVg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmCreateWorkloadStream);
    LIB_FUNCTION("jwCEzr7uEP4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerGetAddressWatch);
    LIB_FUNCTION("PNf0G7gvFHQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerHaltWavefront);
    LIB_FUNCTION("nO-tMnaxJiE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerReadGds);
    LIB_FUNCTION("t0HIQWnvK9E", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerReadSqIndirectRegister);
    LIB_FUNCTION("HsLtF4jKe48", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerResumeWavefront);
    LIB_FUNCTION("JRKSSV0YzwA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerResumeWavefrontCreation);
    LIB_FUNCTION("jpTMyYB8UBI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerSetAddressWatch);
    LIB_FUNCTION("MJG69Q7ti+s", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerWriteGds);
    LIB_FUNCTION("PaFw9w6f808", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebuggerWriteSqIndirectRegister);
    LIB_FUNCTION("qpGITzPE+Zc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebugHardwareStatus);
    LIB_FUNCTION("PVT+fuoS9gU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDeleteEqEvent);
    LIB_FUNCTION("UtObDRQiGbs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDestroyWorkloadStream);
    LIB_FUNCTION("bX5IbRvECXk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDingDong);
    LIB_FUNCTION("byXlqupd8cE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDingDongForWorkload);
    LIB_FUNCTION("HHo1BAljZO8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDisableMipStatsReport);
    LIB_FUNCTION("0BzLGljcwBo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDispatchDirect);
    LIB_FUNCTION("Z43vKp5k7r0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDispatchIndirect);
    LIB_FUNCTION("wED4ZXCFJT0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDispatchIndirectOnMec);
    LIB_FUNCTION("nF6bFRUBRAU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDispatchInitDefaultHardwareState);
    LIB_FUNCTION("HlTPoZ-oY7Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDrawIndex);
    LIB_FUNCTION("GGsn7jMTxw4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDrawIndexAuto);
    LIB_FUNCTION("ED9-Fjr8Ta4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndexIndirect);
    LIB_FUNCTION("thbPcG7E7qk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndexIndirectCountMulti);
    LIB_FUNCTION("5q95ravnueg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndexIndirectMulti);
    LIB_FUNCTION("jHdPvIzlpKc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndexMultiInstanced);
    LIB_FUNCTION("oYM+YzfCm2Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndexOffset);
    LIB_FUNCTION("4v+otIIdjqg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDrawIndirect);
    LIB_FUNCTION("cUCo8OvArrw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndirectCountMulti);
    LIB_FUNCTION("f5QQLp9rzGk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawIndirectMulti);
    LIB_FUNCTION("Idffwf3yh8s", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitDefaultHardwareState);
    LIB_FUNCTION("QhnyReteJ1M", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitDefaultHardwareState175);
    LIB_FUNCTION("0H2vBYbTLHI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitDefaultHardwareState200);
    LIB_FUNCTION("yb2cRhagD1I", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitDefaultHardwareState350);
    LIB_FUNCTION("8lH54sfjfmU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitToDefaultContextState);
    LIB_FUNCTION("im2ZuItabu4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitToDefaultContextState400);
    LIB_FUNCTION("stDSYW2SBVM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawOpaqueAuto);
    LIB_FUNCTION("TLV4mswiZ4A", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverCaptureInProgress);
    LIB_FUNCTION("ODEeJ1GfDtE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterface);
    LIB_FUNCTION("4LSXsEKPTsE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForGpuDebugger);
    LIB_FUNCTION("MpncRjHNYRE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForGpuException);
    LIB_FUNCTION("EwjWGcIOgeM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForHDRScopes);
    LIB_FUNCTION("3EXdrVC7WFk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForReplay);
    LIB_FUNCTION("P9iKqxAGeck", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForResourceRegistration);
    LIB_FUNCTION("t-vIc5cTEzg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalRetrieveGnmInterfaceForValidation);
    LIB_FUNCTION("BvvO8Up88Zc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverInternalVirtualQuery);
    LIB_FUNCTION("R6z1xM3pW-w", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverTraceInProgress);
    LIB_FUNCTION("d88anrgNoKY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDriverTriggerCapture);
    LIB_FUNCTION("Fa3x75OOLRA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmEndWorkload);
    LIB_FUNCTION("4Mv9OXypBG8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmFindResourcesPublic);
    LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmFlushGarlic);
    LIB_FUNCTION("GviyYfFQIkc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetCoredumpAddress);
    LIB_FUNCTION("meiO-5ZCVIE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetCoredumpMode);
    LIB_FUNCTION("O-7nHKgcNSQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetCoredumpProtectionFaultTimestamp);
    LIB_FUNCTION("bSJFzejYrJI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetDbgGcHandle);
    LIB_FUNCTION("pd4C7da6sEg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetDebugTimestamp);
    LIB_FUNCTION("UoYY0DWMC0U", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetEqEventType);
    LIB_FUNCTION("H7-fgvEutM0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetEqTimeStamp);
    LIB_FUNCTION("oL4hGI1PMpw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetGpuBlockStatus);
    LIB_FUNCTION("Fwvh++m9IQI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetGpuCoreClockFrequency);
    LIB_FUNCTION("tZCSL5ulnB4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetGpuInfoStatus);
    LIB_FUNCTION("iFirFzgYsvw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetLastWaitedAddress);
    LIB_FUNCTION("KnldROUkWJY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetNumTcaUnits);
    LIB_FUNCTION("FFVZcCu3zWU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetOffChipTessellationBufferSize);
    LIB_FUNCTION("QJjPjlmPAL0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetOwnerName);
    LIB_FUNCTION("dewXw5roLs0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetPhysicalCounterFromVirtualized);
    LIB_FUNCTION("fzJdEihTFV4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetProtectionFaultTimeStamp);
    LIB_FUNCTION("4PKnYXOhcx4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceBaseAddressAndSizeInBytes);
    LIB_FUNCTION("O0S96YnD04U", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceName);
    LIB_FUNCTION("UBv7FkVfzcQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceShaderGuid);
    LIB_FUNCTION("bdqdvIkLPIU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceType);
    LIB_FUNCTION("UoBuWAhKk7U", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceUserData);
    LIB_FUNCTION("nEyFbYUloIM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetShaderProgramBaseAddress);
    LIB_FUNCTION("k7iGTvDQPLQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetShaderStatus);
    LIB_FUNCTION("ln33zjBrfjk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetTheTessellationFactorRingBufferBaseAddress);
    LIB_FUNCTION("QLdG7G-PBZo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGpuPaDebugEnter);
    LIB_FUNCTION("tVEdZe3wlbY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGpuPaDebugLeave);
    LIB_FUNCTION("NfvOrNzy6sk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertDingDongMarker);
    LIB_FUNCTION("7qZVNgEu+SY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertPopMarker);
    LIB_FUNCTION("aPIZJTXC+cU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertPushColorMarker);
    LIB_FUNCTION("W1Etj-jlW7Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertPushMarker);
    LIB_FUNCTION("aj3L-iaFmyk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertSetColorMarker);
    LIB_FUNCTION("jiItzS6+22g", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertSetMarker);
    LIB_FUNCTION("URDgJcXhQOs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertThreadTraceMarker);
    LIB_FUNCTION("1qXLHIpROPE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmInsertWaitFlipDone);
    LIB_FUNCTION("HRyNHoAjb6E", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmIsCoredumpValid);
    LIB_FUNCTION("jg33rEKLfVs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmIsUserPaEnabled);
    LIB_FUNCTION("26PM5Mzl8zc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmLogicalCuIndexToPhysicalCuIndex);
    LIB_FUNCTION("RU74kek-N0c", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmLogicalCuMaskToPhysicalCuMask);
    LIB_FUNCTION("Kl0Z3LH07QI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmLogicalTcaUnitToPhysical);
    LIB_FUNCTION("29oKvKXzEZo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmMapComputeQueue);
    LIB_FUNCTION("A+uGq+3KFtQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmMapComputeQueueWithPriority);
    LIB_FUNCTION("+N+wrSYBLIw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmPaDisableFlipCallbacks);
    LIB_FUNCTION("8WDA9RiXLaw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmPaEnableFlipCallbacks);
    LIB_FUNCTION("tNuT48mApTc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmPaHeartbeat);
    LIB_FUNCTION("6IMbpR7nTzA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmQueryResourceRegistrationUserMemoryRequirements);
    LIB_FUNCTION("+rJnw2e9O+0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRaiseUserExceptionEvent);
    LIB_FUNCTION("9Mv61HaMhfA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRegisterGdsResource);
    LIB_FUNCTION("t7-VbMosbR4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRegisterGnmLiveCallbackConfig);
    LIB_FUNCTION("ZFqKFl23aMc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmRegisterOwner);
    LIB_FUNCTION("nvEwfYAImTs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRegisterResource);
    LIB_FUNCTION("gObODli-OH8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRequestFlipAndSubmitDone);
    LIB_FUNCTION("6YRHhh5mHCs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRequestFlipAndSubmitDoneForWorkload);
    LIB_FUNCTION("f85orjx7qts", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRequestMipStatsReportAndReset);
    LIB_FUNCTION("MYRtYhojKdA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmResetVgtControl);
    LIB_FUNCTION("hS0MKPRdNr0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSdmaClose);
    LIB_FUNCTION("31G6PB2oRYQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSdmaConstFill);
    LIB_FUNCTION("Lg2isla2XeQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSdmaCopyLinear);
    LIB_FUNCTION("-Se2FY+UTsI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSdmaCopyTiled);
    LIB_FUNCTION("OlFgKnBsALE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSdmaCopyWindow);
    LIB_FUNCTION("LQQN0SwQv8c", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSdmaFlush);
    LIB_FUNCTION("suUlSjWr7CE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSdmaGetMinCmdSize);
    LIB_FUNCTION("5AtqyMgO7fM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSdmaOpen);
    LIB_FUNCTION("KXltnCwEJHQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetCsShader);
    LIB_FUNCTION("Kx-h-nWQJ8A", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetCsShaderWithModifier);
    LIB_FUNCTION("X9Omw9dwv5M", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetEmbeddedPsShader);
    LIB_FUNCTION("+AFvOEXrKJk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetEmbeddedVsShader);
    LIB_FUNCTION("FUHG8sQ3R58", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetEsShader);
    LIB_FUNCTION("jtkqXpAOY6w", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetGsRingSizes);
    LIB_FUNCTION("UJwNuMBcUAk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetGsShader);
    LIB_FUNCTION("VJNjFtqiF5w", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetHsShader);
    LIB_FUNCTION("vckdzbQ46SI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetLsShader);
    LIB_FUNCTION("bQVd5YzCal0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetPsShader);
    LIB_FUNCTION("5uFKckiJYRM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetPsShader350);
    LIB_FUNCTION("q-qhDxP67Hg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetResourceRegistrationUserMemory);
    LIB_FUNCTION("K3BKBBYKUSE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetResourceUserData);
    LIB_FUNCTION("0O3xxFaiObw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetSpiEnableSqCounters);
    LIB_FUNCTION("lN7Gk-p9u78", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetSpiEnableSqCountersForUnitInstance);
    LIB_FUNCTION("+xuDhxlWRPg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetupMipStatsReport);
    LIB_FUNCTION("cFCp0NX8wf0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetVgtControl);
    LIB_FUNCTION("gAhCn6UiU4Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetVsShader);
    LIB_FUNCTION("y+iI2lkX+qI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetWaveLimitMultiplier);
    LIB_FUNCTION("XiyzNZ9J4nQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSetWaveLimitMultipliers);
    LIB_FUNCTION("kkn+iy-mhyg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmEndSpm);
    LIB_FUNCTION("aqhuK2Mj4X4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmInit);
    LIB_FUNCTION("KHpZ9hJo1c0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmInit2);
    LIB_FUNCTION("QEsMC+M3yjE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmSetDelay);
    LIB_FUNCTION("hljMAxTLNF0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmSetMuxRam);
    LIB_FUNCTION("bioGsp74SLM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmSetMuxRam2);
    LIB_FUNCTION("cMWWYeqQQlM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSpmSetSelectCounter);
    LIB_FUNCTION("-zJi8Vb4Du4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSpmSetSpmSelects);
    LIB_FUNCTION("xTsOqp-1bE4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSpmSetSpmSelects2);
    LIB_FUNCTION("AmmYLcJGTl0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSpmStartSpm);
    LIB_FUNCTION("UHDiSFDxNao", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSqttFini);
    LIB_FUNCTION("a3tLC56vwug", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttFinishTrace);
    LIB_FUNCTION("L-owl1dSKKg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSqttGetBcInfo);
    LIB_FUNCTION("LQtzqghKQm4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetGpuClocks);
    LIB_FUNCTION("wYN5mmv6Ya8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetHiWater);
    LIB_FUNCTION("9X4SkENMS0M", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSqttGetStatus);
    LIB_FUNCTION("lbMccQM2iqc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetTraceCounter);
    LIB_FUNCTION("DYAC6JUeZvM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetTraceWptr);
    LIB_FUNCTION("pS2tjBxzJr4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetWrapCounts);
    LIB_FUNCTION("rXV8az6X+fM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetWrapCounts2);
    LIB_FUNCTION("ARS+TNLopyk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttGetWritebackLabels);
    LIB_FUNCTION("X6yCBYPP7HA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSqttInit);
    LIB_FUNCTION("2IJhUyK8moE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSelectMode);
    LIB_FUNCTION("QA5h6Gh3r60", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSelectTarget);
    LIB_FUNCTION("F5XJY1XHa3Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSelectTokens);
    LIB_FUNCTION("wJtaTpNZfH4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetCuPerfMask);
    LIB_FUNCTION("kY4dsQh+SH4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetDceEventWrite);
    LIB_FUNCTION("7XRH1CIfNpI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetHiWater);
    LIB_FUNCTION("05YzC2r3hHo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetTraceBuffer2);
    LIB_FUNCTION("ASUric-2EnI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetTraceBuffers);
    LIB_FUNCTION("gPxYzPp2wlo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetUserData);
    LIB_FUNCTION("d-YcZX7SIQA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSetUserdataTimer);
    LIB_FUNCTION("ru8cb4he6O8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttStartTrace);
    LIB_FUNCTION("gVuGo1nBnG8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSqttStopTrace);
    LIB_FUNCTION("OpyolX6RwS0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSwitchTraceBuffer);
    LIB_FUNCTION("dl5u5eGBgNk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttSwitchTraceBuffer2);
    LIB_FUNCTION("QLzOwOF0t+A", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSqttWaitForEvent);
    LIB_FUNCTION("xbxNatawohc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSubmitAndFlipCommandBuffers);
    LIB_FUNCTION("Ga6r7H6Y0RI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSubmitAndFlipCommandBuffersForWorkload);
    LIB_FUNCTION("zwY0YV91TTI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSubmitCommandBuffers);
    LIB_FUNCTION("jRcI8VcgTz4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmSubmitCommandBuffersForWorkload);
    LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitDone);
    LIB_FUNCTION("ArSg-TGinhk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUnmapComputeQueue);
    LIB_FUNCTION("yhFCnaz5daw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUnregisterAllResourcesForOwner);
    LIB_FUNCTION("fhKwCVVj9nk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUnregisterOwnerAndResources);
    LIB_FUNCTION("k8EXkhIP+lM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUnregisterResource);
    LIB_FUNCTION("nLM2i2+65hA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdateGsShader);
    LIB_FUNCTION("GNlx+y7xPdE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdateHsShader);
    LIB_FUNCTION("4MgRw-bVNQU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdatePsShader);
    LIB_FUNCTION("mLVL7N7BVBg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdatePsShader350);
    LIB_FUNCTION("V31V01UiScY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdateVsShader);
    LIB_FUNCTION("iCO804ZgzdA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateCommandBuffers);
    LIB_FUNCTION("SXw4dZEkgpA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateDisableDiagnostics);
    LIB_FUNCTION("BgM3t3LvcNk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateDisableDiagnostics2);
    LIB_FUNCTION("qGP74T5OWJc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateDispatchCommandBuffers);
    LIB_FUNCTION("hsZPf1lON7E", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateDrawCommandBuffers);
    LIB_FUNCTION("RX7XCNSaL6I", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateGetDiagnosticInfo);
    LIB_FUNCTION("5SHGNwLXBV4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateGetDiagnostics);
    LIB_FUNCTION("HzMN7ANqYEc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateGetVersion);
    LIB_FUNCTION("rTIV11nMQuM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateOnSubmitEnabled);
    LIB_FUNCTION("MBMa6EFu4Ko", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidateResetState);
    LIB_FUNCTION("Q7t4VEYLafI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmValidationRegisterMemoryCheckCallback);
    LIB_FUNCTION("xeTLfxVIQO4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceRazorCaptureCommandBuffersOnlyImmediate);
    LIB_FUNCTION("9thMn+uB1is", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceRazorCaptureCommandBuffersOnlySinceLastFlip);
    LIB_FUNCTION("u9YKpRRHe-M", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceRazorCaptureImmediate);
    LIB_FUNCTION("4UFagYlfuAM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceRazorCaptureSinceLastFlip);
    LIB_FUNCTION("f33OrruQYbM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceRazorIsLoaded);
    LIB_FUNCTION("Bj0GWi1jWcM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_063D065A2D6359C3);
    LIB_FUNCTION("DKusr7JYQp0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_0CABACAFB258429D);
    LIB_FUNCTION("FQzzNvwumaM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_150CF336FC2E99A3);
    LIB_FUNCTION("F8pof57lLUk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_17CA687F9EE52D49);
    LIB_FUNCTION("GHC4n3Wca0U", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_1870B89F759C6B45);
    LIB_FUNCTION("JvkCnvaKlV4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_26F9029EF68A955E);
    LIB_FUNCTION("MB49u6sJLbA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_301E3DBBAB092DB0);
    LIB_FUNCTION("MLr+Fyrxf+8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_30BAFE172AF17FEF);
    LIB_FUNCTION("Pmo+ggPZUxc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_3E6A3E8203D95317);
    LIB_FUNCTION("QP7vDGU0xDQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_40FEEF0C6534C434);
    LIB_FUNCTION("QWuQed5Mus4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_416B9079DE4CBACE);
    LIB_FUNCTION("R3TYO7Tdv5o", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_4774D83BB4DDBF9A);
    LIB_FUNCTION("UGePHM7rmgA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_50678F1CCEEB9A00);
    LIB_FUNCTION("VKLsX6TGJBM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_54A2EC5FA4C62413);
    LIB_FUNCTION("WpxSyDE4rms", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_5A9C52C83138AE6B);
    LIB_FUNCTION("XSIZOjHqEUI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_5D22193A31EA1142);
    LIB_FUNCTION("clo23rtglI0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_725A36DEBB60948D);
    LIB_FUNCTION("gCGlAvphubs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_8021A502FA61B9BB);
    LIB_FUNCTION("nQAv4PpA8OY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_9D002FE0FA40F0E6);
    LIB_FUNCTION("nSl-NqcCi3E", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_9D297F36A7028B71);
    LIB_FUNCTION("otfsenvPebM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_A2D7EC7A7BCF79B3);
    LIB_FUNCTION("qhKjy4mQhUo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_AA12A3CB8990854A);
    LIB_FUNCTION("rcjdwAUCC8Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_ADC8DDC005020BC6);
    LIB_FUNCTION("sKhoi2ectC0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_B0A8688B679CB42D);
    LIB_FUNCTION("tIkCC1FXpf8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_B489020B5157A5FF);
    LIB_FUNCTION("ut57TBmRQN0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_BADE7B4C199140DD);
    LIB_FUNCTION("0VEbnc-7Pdk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_D1511B9DCFFB3DD9);
    LIB_FUNCTION("1TRGZJsC5Y4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_D53446649B02E58E);
    LIB_FUNCTION("2Lbo4o4e8KM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_D8B6E8E28E1EF0A3);
    LIB_FUNCTION("2T1zOhnddFQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_D93D733A19DD7454);
    LIB_FUNCTION("3plUQ7wqgxc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_DE995443BC2A8317);
    LIB_FUNCTION("326VKBUMI-8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_DF6E9528150C23FF);
    LIB_FUNCTION("7LTGukH+M1A", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 Func_ECB4C6BA41FE3350);
    LIB_FUNCTION("dqPBvjFVpTA", "libSceGnmDebugModuleReset", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebugModuleReset);
    LIB_FUNCTION("RNPAItiMLIg", "libSceGnmDebugReset", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDebugReset);
    LIB_FUNCTION("xMMot887QXE", "libSceGnmDebugReset", 1, "libSceGnmDriver", 1, 1,
                 Func_C4C328B7CF3B4171);
    LIB_FUNCTION("pF1HQjbmQJ0", "libSceGnmDriverCompat", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitToDefaultContextStateInternalCommand);
    LIB_FUNCTION("jajhf-Gi3AI", "libSceGnmDriverCompat", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitToDefaultContextStateInternalSize);
    LIB_FUNCTION("vbcR4Ken6AA", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 sceGnmFindResources);
    LIB_FUNCTION("eLQbNsKeTkU", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetResourceRegistrationBuffers);
    LIB_FUNCTION("j6mSQs3UgaY", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 sceGnmRegisterOwnerForSystem);
    LIB_FUNCTION("HEOIaxbuVTA", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 Func_1C43886B16EE5530);
    LIB_FUNCTION("gQNwGezNDgE", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 Func_81037019ECCD0E01);
    LIB_FUNCTION("v7QcBXR48L8", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 Func_BFB41C057478F0BF);
    LIB_FUNCTION("5R1E24FRI4w", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 Func_E51D44DB8151238C);
    LIB_FUNCTION("+RaJBCVJZVM", "libSceGnmDriverResourceRegistration", 1, "libSceGnmDriver", 1, 1,
                 Func_F916890425496553);
    LIB_FUNCTION("Fwvh++m9IQI", "libSceGnmGetGpuCoreClockFrequency", 1, "libSceGnmDriver", 1, 1,
                 sceGnmGetGpuCoreClockFrequency);
    LIB_FUNCTION("R3TYO7Tdv5o", "libSceGnmWaitFreeSubmit", 1, "libSceGnmDriver", 1, 1,
                 Func_4774D83BB4DDBF9A);
    LIB_FUNCTION("ut57TBmRQN0", "libSceGnmWaitFreeSubmit", 1, "libSceGnmDriver", 1, 1,
                 Func_BADE7B4C199140DD);
};

} // namespace Libraries::GnmDriver
