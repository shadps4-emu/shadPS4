// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/io_file.h"
#include "common/thread.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pm4_cmds.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace AmdGpu {

std::array<u8, 48_KB> Liverpool::ConstantEngine::constants_heap;

Liverpool::Liverpool() {
    process_thread = std::jthread{std::bind_front(&Liverpool::Process, this)};
}

Liverpool::~Liverpool() {
    process_thread.request_stop();
    num_submits = -1;
    num_submits.notify_one();
    process_thread.join();
}

void Liverpool::Process(std::stop_token stoken) {
    Common::SetCurrentThreadName("GPU_CommandProcessor");

    while (!stoken.stop_requested()) {
        num_submits.wait(0);

        if (stoken.stop_requested()) {
            break;
        }

        int qid = -1;

        while (num_submits) {
            qid = (qid + 1) % NumTotalQueues;

            auto& queue = mapped_queues[qid];

            Task::Handle task{};
            {
                std::scoped_lock lock{queue.m_access};

                if (queue.submits.empty()) {
                    continue;
                }

                task = queue.submits.front();
            }
            task.resume();

            if (task.done()) {
                task.destroy();

                std::scoped_lock lock{queue.m_access};
                queue.submits.pop();

                --num_submits;
            }
        }

        if (submit_done) {
            num_submits.notify_all();
            submit_done = false;
        }
    }
}

void Liverpool::WaitGpuIdle() {
    while (const auto old = num_submits.load()) {
        num_submits.wait(old);
    }
}

Liverpool::Task Liverpool::ProcessCeUpdate(std::span<const u32> ccb) {
    while (!ccb.empty()) {
        const auto* header = reinterpret_cast<const PM4Header*>(ccb.data());
        const u32 type = header->type;
        if (type != 3) {
            // No other types of packets were spotted so far
            UNREACHABLE_MSG("Invalid PM4 type {}", type);
        }

        const PM4ItOpcode opcode = header->type3.opcode;
        const auto* it_body = reinterpret_cast<const u32*>(header) + 1;
        switch (opcode) {
        case PM4ItOpcode::Nop: {
            const auto* nop = reinterpret_cast<const PM4CmdNop*>(header);
            break;
        }
        case PM4ItOpcode::WriteConstRam: {
            const auto* write_const = reinterpret_cast<const PM4WriteConstRam*>(header);
            memcpy(cblock.constants_heap.data() + write_const->Offset(), &write_const->data,
                   write_const->Size());
            break;
        }
        case PM4ItOpcode::DumpConstRam: {
            const auto* dump_const = reinterpret_cast<const PM4DumpConstRam*>(header);
            memcpy(dump_const->Address<void*>(),
                   cblock.constants_heap.data() + dump_const->Offset(), dump_const->Size());
            break;
        }
        case PM4ItOpcode::IncrementCeCounter: {
            ++cblock.ce_count;
            break;
        }
        case PM4ItOpcode::WaitOnDeCounterDiff: {
            const auto diff = it_body[0];
            while ((cblock.de_count - cblock.ce_count) >= diff) {
                co_yield {};
            }
            break;
        }
        default:
            const u32 count = header->type3.NumWords();
            UNREACHABLE_MSG("Unknown PM4 type 3 opcode {:#x} with count {}",
                            static_cast<u32>(opcode), count);
        }
        ccb = ccb.subspan(header->type3.NumWords() + 1);
    }
}

Liverpool::Task Liverpool::ProcessGraphics(std::span<const u32> dcb, std::span<const u32> ccb) {
    cblock.Reset();

    // TODO: potentially, ASCs also can depend on CE and in this case the
    // CE task should be moved into more global scope
    Task ce_task{};

    if (!ccb.empty()) {
        // In case of CCB provided kick off CE asap to have the constant heap ready to use
        ce_task = ProcessCeUpdate(ccb);
        ce_task.handle.resume();
    }

    while (!dcb.empty()) {
        const auto* header = reinterpret_cast<const PM4Header*>(dcb.data());
        const u32 type = header->type;
        if (type != 3) {
            // No other types of packets were spotted so far
            UNREACHABLE_MSG("Invalid PM4 type {}", type);
        }

        const u32 count = header->type3.NumWords();
        const PM4ItOpcode opcode = header->type3.opcode;
        switch (opcode) {
        case PM4ItOpcode::Nop: {
            const auto* nop = reinterpret_cast<const PM4CmdNop*>(header);
            if (nop->header.count.Value() == 0) {
                break;
            }

            switch (nop->data_block[0]) {
            case PM4CmdNop::PayloadType::PatchedFlip: {
                // There is no evidence that GPU CP drives flip events by parsing
                // special NOP packets. For convenience lets assume that it does.
                Platform::IrqC::Instance()->Signal(Platform::InterruptId::GfxFlip);
                break;
            }
            default:
                break;
            }
            break;
        }
        case PM4ItOpcode::SetContextReg: {
            const auto* set_data = reinterpret_cast<const PM4CmdSetData*>(header);
            std::memcpy(&regs.reg_array[ContextRegWordOffset + set_data->reg_offset], header + 2,
                        (count - 1) * sizeof(u32));
            break;
        }
        case PM4ItOpcode::SetShReg: {
            const auto* set_data = reinterpret_cast<const PM4CmdSetData*>(header);
            std::memcpy(&regs.reg_array[ShRegWordOffset + set_data->reg_offset], header + 2,
                        (count - 1) * sizeof(u32));
            break;
        }
        case PM4ItOpcode::SetUconfigReg: {
            const auto* set_data = reinterpret_cast<const PM4CmdSetData*>(header);
            std::memcpy(&regs.reg_array[UconfigRegWordOffset + set_data->reg_offset], header + 2,
                        (count - 1) * sizeof(u32));
            break;
        }
        case PM4ItOpcode::IndexType: {
            const auto* index_type = reinterpret_cast<const PM4CmdDrawIndexType*>(header);
            regs.index_buffer_type.raw = index_type->raw;
            break;
        }
        case PM4ItOpcode::DrawIndex2: {
            const auto* draw_index = reinterpret_cast<const PM4CmdDrawIndex2*>(header);
            regs.max_index_size = draw_index->max_size;
            regs.index_base_address.base_addr_lo = draw_index->index_base_lo;
            regs.index_base_address.base_addr_hi.Assign(draw_index->index_base_hi);
            regs.num_indices = draw_index->index_count;
            regs.draw_initiator = draw_index->draw_initiator;
            if (rasterizer) {
                rasterizer->Draw(true);
            }
            break;
        }
        case PM4ItOpcode::DrawIndexAuto: {
            const auto* draw_index = reinterpret_cast<const PM4CmdDrawIndexAuto*>(header);
            regs.num_indices = draw_index->index_count;
            regs.draw_initiator = draw_index->draw_initiator;
            if (rasterizer) {
                rasterizer->Draw(false);
            }
            break;
        }
        case PM4ItOpcode::DispatchDirect: {
            const auto* dispatch_direct = reinterpret_cast<const PM4CmdDispatchDirect*>(header);
            regs.cs_program.dim_x = dispatch_direct->dim_x;
            regs.cs_program.dim_y = dispatch_direct->dim_y;
            regs.cs_program.dim_z = dispatch_direct->dim_z;
            regs.cs_program.dispatch_initiator = dispatch_direct->dispatch_initiator;
            if (rasterizer && (regs.cs_program.dispatch_initiator & 1)) {
                rasterizer->DispatchDirect();
            }
            break;
        }
        case PM4ItOpcode::EventWrite: {
            // const auto* event = reinterpret_cast<const PM4CmdEventWrite*>(header);
            break;
        }
        case PM4ItOpcode::EventWriteEos: {
            const auto* event_eos = reinterpret_cast<const PM4CmdEventWriteEos*>(header);
            event_eos->SignalFence();
            break;
        }
        case PM4ItOpcode::EventWriteEop: {
            const auto* event_eop = reinterpret_cast<const PM4CmdEventWriteEop*>(header);
            event_eop->SignalFence();
            break;
        }
        case PM4ItOpcode::DmaData: {
            const auto* dma_data = reinterpret_cast<const PM4DmaData*>(header);
            break;
        }
        case PM4ItOpcode::WriteData: {
            const auto* write_data = reinterpret_cast<const PM4CmdWriteData*>(header);
            ASSERT(write_data->dst_sel.Value() == 2 || write_data->dst_sel.Value() == 5);
            const u32 data_size = (header->type3.count.Value() - 2) * 4;
            if (!write_data->wr_one_addr.Value()) {
                std::memcpy(write_data->Address<void*>(), write_data->data, data_size);
            } else {
                UNREACHABLE();
            }
            break;
        }
        case PM4ItOpcode::AcquireMem: {
            // const auto* acquire_mem = reinterpret_cast<PM4CmdAcquireMem*>(header);
            break;
        }
        case PM4ItOpcode::WaitRegMem: {
            const auto* wait_reg_mem = reinterpret_cast<const PM4CmdWaitRegMem*>(header);
            ASSERT(wait_reg_mem->engine.Value() == PM4CmdWaitRegMem::Engine::Me);
            while (!wait_reg_mem->Test()) {
                co_yield {};
            }
            break;
        }
        case PM4ItOpcode::IncrementDeCounter: {
            ++cblock.de_count;
            break;
        }
        case PM4ItOpcode::WaitOnCeCounter: {
            while (cblock.ce_count <= cblock.de_count) {
                ce_task.handle.resume();
            }
            break;
        }
        default:
            UNREACHABLE_MSG("Unknown PM4 type 3 opcode {:#x} with count {}",
                            static_cast<u32>(opcode), count);
        }

        dcb = dcb.subspan(header->type3.NumWords() + 1);
    }

    if (ce_task.handle) {
        ASSERT_MSG(ce_task.handle.done(), "Partially processed CCB");
        ce_task.handle.destroy();
    }
}

Liverpool::Task Liverpool::ProcessCompute(std::span<const u32> acb) {
    while (!acb.empty()) {
        const auto* header = reinterpret_cast<const PM4Header*>(acb.data());
        const u32 type = header->type;
        if (type != 3) {
            // No other types of packets were spotted so far
            UNREACHABLE_MSG("Invalid PM4 type {}", type);
        }

        const u32 count = header->type3.NumWords();
        const PM4ItOpcode opcode = header->type3.opcode;
        const auto* it_body = reinterpret_cast<const u32*>(header) + 1;
        switch (opcode) {
        default:
            UNREACHABLE_MSG("Unknown PM4 type 3 opcode {:#x} with count {}",
                            static_cast<u32>(opcode), count);
        }

        acb = acb.subspan(header->type3.NumWords() + 1);
    }

    return {}; // Not a coroutine yet
}

void Liverpool::SubmitGfx(std::span<const u32> dcb, std::span<const u32> ccb) {
    static constexpr u32 GfxQueueId = 0u;
    auto& queue = mapped_queues[GfxQueueId];

    auto task = ProcessGraphics(dcb, ccb);
    {
        std::unique_lock lock{queue.m_access};
        queue.submits.emplace(task.handle);
    }

    ++num_submits;
    num_submits.notify_one();
}

void Liverpool::SubmitAsc(u32 vqid, std::span<const u32> acb) {
    ASSERT_MSG(vqid > 0 && vqid < NumTotalQueues, "Invalid virtual ASC queue index");
    auto& queue = mapped_queues[vqid];

    const auto& task = ProcessCompute(acb);
    {
        std::unique_lock lock{queue.m_access};
        queue.submits.emplace(task.handle);
    }

    ++num_submits;
    num_submits.notify_one();
}

} // namespace AmdGpu
