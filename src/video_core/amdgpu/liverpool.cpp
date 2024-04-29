// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/io_file.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pm4_cmds.h"

namespace AmdGpu {

Liverpool::Liverpool() = default;

void Liverpool::ProcessCmdList(u32* cmdbuf, u32 size_in_bytes) {
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
                auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[ContextRegWordOffset + set_data->regOffset], header + 2,
                            (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::SetShReg: {
                auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[ShRegWordOffset + set_data->regOffset], header + 2,
                            (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::SetUconfigReg: {
                auto* set_data = reinterpret_cast<PM4CmdSetData*>(header);
                std::memcpy(&regs.reg_array[UconfigRegWordOffset + set_data->regOffset], header + 2,
                            (count - 1) * sizeof(u32));
                break;
            }
            case PM4ItOpcode::IndexType: {
                auto* index_type = reinterpret_cast<PM4CmdDrawIndexType*>(header);
                regs.index_buffer_type.raw = index_type->raw;
                break;
            }
            case PM4ItOpcode::DrawIndex2: {
                auto* draw_index = reinterpret_cast<PM4CmdDrawIndex2*>(header);
                regs.max_index_size = draw_index->maxSize;
                regs.index_base_address.base_addr_lo = draw_index->indexBaseLo;
                regs.index_base_address.base_addr_hi.Assign(draw_index->indexBaseHi);
                regs.num_indices = draw_index->indexCount;
                regs.draw_initiator = draw_index->drawInitiator;
                // rasterizer->DrawIndex();
                break;
            }
            case PM4ItOpcode::DrawIndexAuto: {
                auto* draw_index = reinterpret_cast<PM4CmdDrawIndexAuto*>(header);
                regs.num_indices = draw_index->index_count;
                regs.draw_initiator = draw_index->draw_initiator;
                // rasterizer->DrawIndex();
                break;
            }
            case PM4ItOpcode::EventWriteEop: {
                auto* event_write = reinterpret_cast<PM4CmdEventWriteEop*>(header);
                const InterruptSelect irq_sel = event_write->intSel;
                const DataSelect data_sel = event_write->dataSel;
                ASSERT(irq_sel == InterruptSelect::None && data_sel == DataSelect::Data64);
                *event_write->Address() = event_write->DataQWord();
                break;
            }
            case PM4ItOpcode::DmaData: {
                auto* dma_data = reinterpret_cast<PM4DmaData*>(header);
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
