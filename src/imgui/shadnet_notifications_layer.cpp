// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <ctime>
#include <deque>
#include <mutex>
#include <vector>

#include <imgui.h>

#include "common/types.h"
#include "imgui/imgui_layer.h"
#include "imgui/shadnet_notifications_layer.h"

namespace ImGui::ShadNetNotify {

constexpr float life_time = 6.0f;     // seconds a toast stays before fading out
constexpr float fade_in = 0.25f;      // fade-in duration
constexpr float fade_out = 0.5f;      // fade-out duration
constexpr std::size_t kMaxToasts = 6; // oldest dropped beyond this

struct Toast {
    Kind kind;
    std::string text;
    float age = 0.0f;
};

constexpr std::size_t max_history = 100;

std::mutex g_mutex;
std::deque<Toast> g_toasts;
std::deque<HistoryEntry> g_history;
std::size_t g_total = 0; // monotonic count of all pushes
std::size_t g_seen = 0;  // value of g_total at last MarkRead()

std::string NowHMS() {
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    if (std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm) == 0) {
        return {};
    }
    return buf;
}

class ShadNetNotificationsUI final : public ImGui::Layer {
public:
    void Draw() override;
};

ShadNetNotificationsUI g_layer;

KindDisplay DisplayOf(Kind kind) {
    switch (kind) {
    case Kind::FriendRequest:
        return {"Friend request", {0.45f, 0.70f, 1.00f, 1.0f}};
    case Kind::FriendNew:
        return {"Friend", {0.35f, 0.85f, 0.40f, 1.0f}};
    case Kind::FriendLost:
        return {"Friend", {0.85f, 0.55f, 0.35f, 1.0f}};
    case Kind::Online:
        return {"Online", {0.35f, 0.85f, 0.40f, 1.0f}};
    case Kind::Info:
    default:
        return {"shadNet", {0.75f, 0.75f, 0.78f, 1.0f}};
    }
}

void Push(Kind kind, std::string text) {
    std::lock_guard lock(g_mutex);
    g_toasts.push_back({kind, text, 0.0f});
    while (g_toasts.size() > kMaxToasts) {
        g_toasts.pop_front();
    }
    g_history.push_back({kind, std::move(text), NowHMS()});
    while (g_history.size() > max_history) {
        g_history.pop_front();
    }
    ++g_total;
}

std::vector<HistoryEntry> GetHistory() {
    std::lock_guard lock(g_mutex);
    return {g_history.begin(), g_history.end()};
}

void ClearHistory() {
    std::lock_guard lock(g_mutex);
    g_history.clear();
}

std::size_t UnreadCount() {
    std::lock_guard lock(g_mutex);
    return g_total - g_seen;
}

void MarkRead() {
    std::lock_guard lock(g_mutex);
    g_seen = g_total;
}

void Register() {
    ImGui::Layer::AddLayer(&g_layer);
}
void Unregister() {
    ImGui::Layer::RemoveLayer(&g_layer);
}

void ShadNetNotificationsUI::Draw() {
    auto& io = ImGui::GetIO();

    std::vector<Toast> snapshot;
    {
        std::lock_guard lock(g_mutex);
        for (auto& t : g_toasts) {
            t.age += io.DeltaTime;
        }
        while (!g_toasts.empty() && g_toasts.front().age >= life_time) {
            g_toasts.pop_front();
        }
        snapshot.assign(g_toasts.begin(), g_toasts.end());
    }
    if (snapshot.empty()) {
        return;
    }

    const float scale = io.DisplaySize.x / 1920.0f;
    const float margin = 20.0f * scale;
    const float width = 320.0f * scale;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - margin, margin), ImGuiCond_Always,
                            ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##shadnet_notifications", nullptr, flags)) {
        // Newest first.
        for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it) {
            float alpha = 1.0f;
            const float remaining = life_time - it->age;
            if (it->age < fade_in) {
                alpha = it->age / fade_in;
            } else if (remaining < fade_out) {
                alpha = std::max(0.0f, remaining / fade_out);
            }

            const KindDisplay ks = DisplayOf(it->kind);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            const ImVec4 col(ks.color[0], ks.color[1], ks.color[2], ks.color[3]);
            ImGui::TextColored(col, "[%s]", ks.tag);
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(it->text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleVar();

            if (it + 1 != snapshot.rend()) {
                ImGui::Separator();
            }
        }
    }
    ImGui::End();
}

} // namespace ImGui::ShadNetNotify
