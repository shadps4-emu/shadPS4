// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <span>

namespace Core::Startup {

enum class Stage { Inactive, Libraries, Executable, Modules, Launch, FirstFrame, Complete };

inline bool HasVisibleRgb8Content(std::span<const std::uint8_t> pixels) {
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
        if (pixels[i] > 4 || pixels[i + 1] > 4 || pixels[i + 2] > 4) {
            return true;
        }
    }
    return false;
}

class Progress {
public:
    using Clock = std::chrono::steady_clock;

    void Begin(Clock::time_point now = Clock::now()) {
        started_ms.store(Milliseconds(now), std::memory_order_relaxed);
        stage.store(Stage::Libraries, std::memory_order_release);
    }

    void SetStage(Stage next) {
        auto current = stage.load(std::memory_order_relaxed);
        while (current != Stage::Inactive && current != Stage::Complete) {
            if (stage.compare_exchange_weak(current, next, std::memory_order_release,
                                            std::memory_order_relaxed)) {
                return;
            }
        }
    }

    Stage GetStage() const {
        return stage.load(std::memory_order_acquire);
    }

    bool IsActive() const {
        const auto current = GetStage();
        return current != Stage::Inactive && current != Stage::Complete;
    }

    bool Presented(bool visible_content, bool success) {
        if (!visible_content || !success) {
            return false;
        }
        auto current = stage.load(std::memory_order_relaxed);
        while (current != Stage::Inactive && current != Stage::Complete) {
            if (stage.compare_exchange_weak(current, Stage::Complete, std::memory_order_release,
                                            std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    std::int64_t ElapsedMs(Clock::time_point now = Clock::now()) const {
        return Milliseconds(now) - started_ms.load(std::memory_order_relaxed);
    }

private:
    static std::int64_t Milliseconds(Clock::time_point time) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch())
            .count();
    }

    std::atomic<Stage> stage{Stage::Inactive};
    std::atomic<std::int64_t> started_ms{};
};

inline Progress progress;

} // namespace Core::Startup
