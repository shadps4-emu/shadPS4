// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/io_file.h"
#include "common/thread.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pm4_cmds.h"

namespace AmdGpu {

Liverpool::Liverpool() = default;

void Liverpool::ProcessCmdList(u32* cmdbuf, u32 size_in_bytes) {
    Common::SetCurrentThreadName("CommandProcessor_Gfx");

    auto* header = reinterpret_cast<PM4Header*>(cmdbuf);
    u32 processed_cmd_size = 0;

    while (processed_cmd_size < size_in_bytes) {
        PM4Header* next_header{};
        const u32 type = header->type;
        switch (type) {
        case 3: {
            const PM4ItOpcode opcode = header->type3.opcode;
            const u32 count = header->type3.NumWords();
            switch (opcode) {
            case PM4ItOpcode::Nop:
                break;
            case PM4ItOpcode::SetContextReg: {
                const auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[ContextRegWordOffset + set_data->reg_offset],
                            header + 2, (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::SetShReg: {
                const auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[ShRegWordOffset + set_data->reg_offset], header + 2,
                            (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::SetUconfigReg: {
                const auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[UconfigRegWordOffset + set_data->reg_offset],
                            header + 2, (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::IndexType: {
                const auto* index_type = reinterpret_cast<PM4CmdDrawIndexType*>(header);
                regs.index_buffer_type.raw = index_type->raw;
                break;
            }
            case PM4ItOpcode::DrawIndex2: {
                const auto* draw_index = reinterpret_cast<PM4CmdDrawIndex2*>(header);
                regs.max_index_size = draw_index->max_size;
                regs.index_base_address.base_addr_lo = draw_index->index_base_lo;
                regs.index_base_address.base_addr_hi.Assign(draw_index->index_base_hi);
                regs.num_indices = draw_index->index_count;
                regs.draw_initiator = draw_index->draw_initiator;
                // rasterizer->DrawIndex();
                break;
            }
            case PM4ItOpcode::DrawIndexAuto: {
                const auto* draw_index = reinterpret_cast<PM4CmdDrawIndexAuto*>(header);
                regs.num_indices = draw_index->index_count;
                regs.draw_initiator = draw_index->draw_initiator;
                // rasterizer->DrawIndex();
                break;
            }
            case PM4ItOpcode::DispatchDirect: {
                // const auto* dispatch_direct = reinterpret_cast<PM4CmdDispatchDirect*>(header);
                break;
            }
            case PM4ItOpcode::EventWriteEos: {
                const auto* event_eos = reinterpret_cast<PM4CmdEventWriteEos*>(header);
                event_eos->SignalFence();
                break;
            }
            case PM4ItOpcode::EventWriteEop: {
                const auto* event_eop = reinterpret_cast<PM4CmdEventWriteEop*>(header);
                event_eop->SignalFence();
                break;
            }
            case PM4ItOpcode::DmaData: {
                const auto* dma_data = reinterpret_cast<PM4DmaData*>(header);
                break;
            }
            case PM4ItOpcode::WriteData: {
                const auto* write_data = reinterpret_cast<PM4CmdWriteData*>(header);
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
                const auto* wait_reg_mem = reinterpret_cast<PM4CmdWaitRegMem*>(header);
                ASSERT(wait_reg_mem->engine.Value() == PM4CmdWaitRegMem::Engine::Me);
                while (!wait_reg_mem->Test()) {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(1ms);
                }
                break;
            }
            default:
                UNREACHABLE_MSG("Unknown PM4 type 3 opcode {:#x} with count {}",
                                static_cast<u32>(opcode), count);
            }
            next_header = header + header->type3.NumWords() + 1;
            break;
        }
        default:
            UNREACHABLE_MSG("Invalid PM4 type {}", type);
        }

        processed_cmd_size += uintptr_t(next_header) - uintptr_t(header);
        header = next_header;
    }
}

} // namespace AmdGpu
