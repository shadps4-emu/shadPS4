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
    Compute0RelMem = 0u,
    Compute1RelMem = 1u,
    Compute2RelMem = 2u,
    Compute3RelMem = 3u,
    Compute4RelMem = 4u,
    Compute5RelMem = 5u,
    Compute6RelMem = 6u,
    GfxEop = 7u,
    GfxFlip = 8u,
    GpuIdle = 9u,
};

using IrqHandler = std::function<void(InterruptId)>;

struct IrqController {
    void RegisterOnce(InterruptId irq, IrqHandler handler) {
        ASSERT_MSG(static_cast<u32>(irq) < irq_contexts.size(), "Invalid IRQ number");
        auto& ctx = irq_contexts[static_cast<u32>(irq)];
        std::unique_lock lock{ctx.m_lock};
        ctx.one_time_subscribers.emplace(handler);
    }

    void Register(InterruptId irq, IrqHandler handler, void* uid) {
        ASSERT_MSG(static_cast<u32>(irq) < irq_contexts.size(), "Invalid IRQ number");
        auto& ctx = irq_contexts[static_cast<u32>(irq)];

        std::unique_lock lock{ctx.m_lock};
        ASSERT_MSG(ctx.persistent_handlers.find(uid) == ctx.persistent_handlers.cend(),
                   "The handler is already registered!");
        ctx.persistent_handlers.emplace(uid, handler);
    }

    void Unregister(InterruptId irq, void* uid) {
        ASSERT_MSG(static_cast<u32>(irq) < irq_contexts.size(), "Invalid IRQ number");
        auto& ctx = irq_contexts[static_cast<u32>(irq)];
        std::unique_lock lock{ctx.m_lock};
        ctx.persistent_handlers.erase(uid);
    }

    void Signal(InterruptId irq) {
        ASSERT_MSG(static_cast<u32>(irq) < irq_contexts.size(), "Unexpected IRQ signaled");
        auto& ctx = irq_contexts[static_cast<u32>(irq)];
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
    std::array<IrqContext, magic_enum::enum_count<InterruptId>()> irq_contexts{};
};

using IrqC = Common::Singleton<IrqController>;

} // namespace Platform
