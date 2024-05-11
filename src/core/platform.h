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
#include <optional>
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
    GfxEop = 0x40u
};

using IrqHandler = std::function<void(InterruptId)>;

struct IrqController {
    void RegisterOnce(IrqHandler handler) {
        std::unique_lock lock{m_lock};
        one_time_subscribers.emplace(handler);
    }

    void Register(IrqHandler handler) {
        ASSERT_MSG(!persistent_handler.has_value(),
                   "Too many persistent handlers"); // Add a slot map if so

        std::unique_lock lock{m_lock};
        persistent_handler.emplace(handler);
    }

    void Unregister() {
        std::unique_lock lock{m_lock};
        persistent_handler.reset();
    }

    void Signal(InterruptId irq) {
        std::unique_lock lock{m_lock};

        LOG_TRACE(Core, "IRQ signaled: {}", magic_enum::enum_name(irq));

        if (persistent_handler) {
            persistent_handler.value()(irq);
        }

        while (!one_time_subscribers.empty()) {
            const auto& h = one_time_subscribers.front();
            h(irq);

            one_time_subscribers.pop();
        }
    }

private:
    std::optional<IrqHandler> persistent_handler{};
    std::queue<IrqHandler> one_time_subscribers{};
    std::mutex m_lock{};
};

using IrqC = Common::Singleton<IrqController>;

} // namespace Platform
