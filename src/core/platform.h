// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "common/types.h"
#include "magic_enum.hpp"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <queue>

namespace Platform {

enum class InterruptId : u32 {
    Compute0RelMem = 0x00,
    Compute1RelMem = 0x01,
    Compute2RelMem = 0x02,
    Compute3RelMem = 0x03,
    Compute4RelMem = 0x04,
    Compute5RelMem = 0x05,
    Compute6RelMem = 0x06,
    GfxEop = 0x40,
    GfxFlip = 0x08,
    GpuIdle = 0x09,

    InterruptIdMax = 0x40, ///< Max possible value (GfxEop)
};

using IrqHandler = std::function<void(InterruptId)>;

struct IrqController {
    void RegisterOnce(InterruptId irq, IrqHandler handler) {
        ASSERT_MSG(static_cast<u32>(irq) <= static_cast<u32>(InterruptId::InterruptIdMax),
                   "Invalid IRQ number");
        auto& ctx = irq_contexts.try_emplace(irq).first->second;
        std::unique_lock lock{ctx.m_lock};
        ctx.one_time_subscribers.emplace(handler);
    }

    void Register(InterruptId irq, IrqHandler handler, void* uid) {
        ASSERT_MSG(static_cast<u32>(irq) <= static_cast<u32>(InterruptId::InterruptIdMax),
                   "Invalid IRQ number");
        auto& ctx = irq_contexts.try_emplace(irq).first->second;

        std::unique_lock lock{ctx.m_lock};
        ASSERT_MSG(ctx.persistent_handlers.find(uid) == ctx.persistent_handlers.cend(),
                   "The handler is already registered!");
        ctx.persistent_handlers.emplace(uid, handler);
    }

    void Unregister(InterruptId irq, void* uid) {
        ASSERT_MSG(static_cast<u32>(irq) <= static_cast<u32>(InterruptId::InterruptIdMax),
                   "Invalid IRQ number");
        auto& ctx = irq_contexts.try_emplace(irq).first->second;
        std::unique_lock lock{ctx.m_lock};
        ctx.persistent_handlers.erase(uid);
    }

    void Signal(InterruptId irq) {
        ASSERT_MSG(static_cast<u32>(irq) <= static_cast<u32>(InterruptId::InterruptIdMax),
                   "Unexpected IRQ signaled");
        auto& ctx = irq_contexts.try_emplace(irq).first->second;
        std::unique_lock lock{ctx.m_lock};

        LOG_TRACE(Core, "IRQ signaled: {}", magic_enum::enum_name(irq));

        for (auto& [uid, h] : ctx.persistent_handlers) {
            h(irq);
        }

        if (!ctx.one_time_subscribers.empty()) {
            const auto& h = ctx.one_time_subscribers.front();
            h(irq);

            ctx.one_time_subscribers.pop();
        }
    }

private:
    struct IrqContext {
        std::unordered_map<void*, IrqHandler> persistent_handlers{};
        std::queue<IrqHandler> one_time_subscribers{};
        std::mutex m_lock{};
    };
    std::unordered_map<InterruptId, IrqContext> irq_contexts{};
};

using IrqC = Common::Singleton<IrqController>;

} // namespace Platform
