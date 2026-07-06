// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <imgui.h>

#include "common/types.h"
#include "core/libraries/np/np_handler.h"
#include "imgui/imgui_layer.h"
#include "imgui/invitation_prompt_layer.h"
#include "imgui/shadnet_notifications_layer.h"

namespace ImGui::InvitationPrompt {

namespace {

enum class State {
    Waiting,  // shown, awaiting user choice
    InFlight, // Accept/Decline running on a worker thread; buttons disabled
};

struct Prompt {
    s32 user_id;
    std::string invitation_id;
    std::string session_id;
    std::string from_npid;
    State state = State::Waiting;
};

constexpr std::size_t kMaxPrompts = 4; // oldest dropped beyond this

std::mutex g_mutex;
std::deque<Prompt> g_prompts;

class InvitationPromptUI final : public ImGui::Layer {
public:
    void Draw() override;
};

InvitationPromptUI g_layer;

void SetState(const std::string& invitation_id, State state) {
    std::lock_guard lock(g_mutex);
    for (auto& p : g_prompts) {
        if (p.invitation_id == invitation_id) {
            p.state = state;
        }
    }
}

void Remove(const std::string& invitation_id) {
    std::lock_guard lock(g_mutex);
    g_prompts.erase(
        std::remove_if(g_prompts.begin(), g_prompts.end(),
                       [&](const Prompt& p) { return p.invitation_id == invitation_id; }),
        g_prompts.end());
}

// Accept/Decline hit the shadNet server (blocking HTTP inside NpHandler), so they run on a
// detached worker thread rather than the render thread. The prompt is marked InFlight until the
// worker finishes, then removed,the outcome is surfaced as a shadNet toast.
void RunAccept(Prompt prompt) {
    SetState(prompt.invitation_id, State::InFlight);
    std::thread([prompt = std::move(prompt)] {
        const bool ok = Libraries::Np::NpHandler::GetInstance().AcceptSessionInvitation(
            prompt.user_id, prompt.invitation_id);
        Remove(prompt.invitation_id);
        if (ok) {
            ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::Info,
                                       "Accepted " + prompt.from_npid +
                                           "'s invitation, joining session");
        } else {
            ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::Info,
                                       "Could not accept " + prompt.from_npid +
                                           "'s invitation (expired or already used)");
        }
    }).detach();
}

void RunDecline(Prompt prompt) {
    SetState(prompt.invitation_id, State::InFlight);
    std::thread([prompt = std::move(prompt)] {
        Libraries::Np::NpHandler::GetInstance().DeclineSessionInvitation(prompt.user_id,
                                                                         prompt.invitation_id);
        Remove(prompt.invitation_id);
    }).detach();
}

} // namespace

void Push(s32 user_id, std::string invitation_id, std::string session_id, std::string from_npid) {
    std::lock_guard lock(g_mutex);
    // A re-sent invite replaces the older prompt rather than stacking a duplicate.
    g_prompts.erase(
        std::remove_if(g_prompts.begin(), g_prompts.end(),
                       [&](const Prompt& p) { return p.invitation_id == invitation_id; }),
        g_prompts.end());
    g_prompts.push_back(
        {user_id, std::move(invitation_id), std::move(session_id), std::move(from_npid)});
    while (g_prompts.size() > kMaxPrompts) {
        g_prompts.pop_front();
    }
}

void Dismiss(const std::string& invitation_id) {
    Remove(invitation_id);
}

void Register() {
    ImGui::Layer::AddLayer(&g_layer);
}

void Unregister() {
    ImGui::Layer::RemoveLayer(&g_layer);
}

void InvitationPromptUI::Draw() {
    std::vector<Prompt> snapshot;
    {
        std::lock_guard lock(g_mutex);
        if (g_prompts.empty()) {
            return;
        }
        snapshot.assign(g_prompts.begin(), g_prompts.end());
    }

    auto& io = ImGui::GetIO();
    const float scale = io.DisplaySize.x / 1920.0f;
    const float width = 420.0f * scale;
    const float margin = 20.0f * scale;

    // Top-center, below the top edge,the transient shadNet toasts live top-right so the two
    // don't collide. Mouse-interactive but NoNav so gamepad navigation stays with the game.
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2.0f, margin), ImGuiCond_Always,
                            ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##invitation_prompts", nullptr, flags)) {
        bool first = true;
        for (const auto& p : snapshot) {
            if (!first) {
                ImGui::Separator();
            }
            first = false;

            ImGui::TextColored(ImVec4(0.45f, 0.70f, 1.00f, 1.0f), "[Invitation]");
            ImGui::SameLine();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::Text("%s invited you to a session", p.from_npid.c_str());
            ImGui::PopTextWrapPos();

            const bool busy = p.state == State::InFlight;
            ImGui::BeginDisabled(busy);
            ImGui::PushID(p.invitation_id.c_str());
            if (ImGui::Button(busy ? "Joining..." : "Accept", ImVec2(120.0f * scale, 0.0f))) {
                RunAccept(p);
            }
            ImGui::SameLine();
            if (ImGui::Button("Decline", ImVec2(120.0f * scale, 0.0f))) {
                RunDecline(p);
            }
            ImGui::PopID();
            ImGui::EndDisabled();
        }
    }
    ImGui::End();
}

} // namespace ImGui::InvitationPrompt
