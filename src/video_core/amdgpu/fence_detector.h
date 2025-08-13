// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "common/config.h"
#include "common/types.h"
#include "video_core/amdgpu/pm4_cmds.h"

namespace AmdGpu {

class FenceDetector {
public:
    explicit FenceDetector(std::span<const u32> cmd) {
        DetectFences(cmd);
    }

    bool IsFence(const PM4Header* header) const {
        return std::ranges::contains(fences, header, &LabelWrite::packet);
    }

private:
    static std::span<const u32> NextPacket(std::span<const u32> cmd, size_t offset) {
        if (offset > cmd.size()) {
            return {};
        }
        return cmd.subspan(offset);
    }

    void DetectFences(std::span<const u32> cmd) {
        if (!Config::readbacks() ||
            Config::readbackAccuracy() == Config::ReadbackAccuracy::Extreme) {
            return;
        }
        while (!cmd.empty()) {
            const auto* header = reinterpret_cast<const PM4Header*>(cmd.data());
            const u32 type = header->type;

            switch (type) {
            default:
                UNREACHABLE_MSG("Wrong PM4 type {}", type);
            case 0:
                return;
            case 2:
                cmd = NextPacket(cmd, 1);
                break;
            case 3:
                const PM4ItOpcode opcode = header->type3.opcode;
                switch (opcode) {
                case PM4ItOpcode::EventWriteEos: {
                    const auto* event_eos = reinterpret_cast<const PM4CmdEventWriteEos*>(header);
                    if (event_eos->command == PM4CmdEventWriteEos::Command::SignalFence) {
                        fences.emplace_back(header, event_eos->Address<VAddr>(),
                                            event_eos->DataDWord());
                    }
                    break;
                }
                case PM4ItOpcode::EventWriteEop: {
                    const auto* event_eop = reinterpret_cast<const PM4CmdEventWriteEop*>(header);
                    if (event_eop->int_sel != InterruptSelect::None) {
                        fences.emplace_back(header);
                    }
                    if (event_eop->data_sel == DataSelect::Data32Low) {
                        fences.emplace_back(header, event_eop->Address<VAddr>(),
                                            event_eop->DataDWord());
                    } else if (event_eop->data_sel == DataSelect::Data64) {
                        fences.emplace_back(header, event_eop->Address<VAddr>(),
                                            event_eop->DataQWord());
                    }
                    break;
                }
                case PM4ItOpcode::ReleaseMem: {
                    const auto* release_mem = reinterpret_cast<const PM4CmdReleaseMem*>(header);
                    if (release_mem->data_sel == DataSelect::Data32Low) {
                        fences.emplace_back(header, release_mem->Address<VAddr>(),
                                            release_mem->DataDWord());
                    } else if (release_mem->data_sel == DataSelect::Data64) {
                        fences.emplace_back(header, release_mem->Address<VAddr>(),
                                            release_mem->DataQWord());
                    }
                    break;
                }
                case PM4ItOpcode::WriteData: {
                    const auto* write_data = reinterpret_cast<const PM4CmdWriteData*>(header);
                    ASSERT(write_data->dst_sel.Value() == 2 || write_data->dst_sel.Value() == 5);
                    const u32 data_size = (header->type3.count.Value() - 2) * 4;
                    if (data_size <= sizeof(u64) && write_data->wr_confirm) {
                        u64 value{};
                        std::memcpy(&value, write_data->data, data_size);
                        fences.emplace_back(header, write_data->Address<VAddr>(), value);
                    }
                    break;
                }
                case PM4ItOpcode::WaitRegMem: {
                    const auto* wait_reg_mem = reinterpret_cast<const PM4CmdWaitRegMem*>(header);
                    if (wait_reg_mem->mem_space == PM4CmdWaitRegMem::MemSpace::Register) {
                        break;
                    }
                    const VAddr wait_addr = wait_reg_mem->Address<VAddr>();
                    using Function = PM4CmdWaitRegMem::Function;
                    const u32 mask = wait_reg_mem->mask;
                    const u32 ref = wait_reg_mem->ref;
                    std::erase_if(fences, [&](const LabelWrite& write) {
                        if (wait_addr != write.label) {
                            return false;
                        }
                        const u32 value = static_cast<u32>(write.value);
                        switch (wait_reg_mem->function.Value()) {
                        case Function::LessThan:
                            return (value & mask) < ref;
                        case Function::LessThanEqual:
                            return (value & mask) <= ref;
                        case Function::Equal:
                            return (value & mask) == ref;
                        case Function::NotEqual:
                            return (value & mask) != ref;
                        case Function::GreaterThanEqual:
                            return (value & mask) >= ref;
                        case Function::GreaterThan:
                            return (value & mask) > ref;
                        default:
                            UNREACHABLE();
                        }
                    });
                    break;
                }
                default:
                    break;
                }
                cmd = NextPacket(cmd, header->type3.NumWords() + 1);
                break;
            }
        }
    }

private:
    struct LabelWrite {
        const PM4Header* packet;
        VAddr label;
        u64 value;
    };
    std::vector<LabelWrite> fences;
};

} // namespace AmdGpu
