// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ImGui::ShadNetNotify {

// Kind drives the colored tag shown on the toast and in the history list.
enum class Kind {
    FriendRequest,
    FriendNew,
    FriendLost,
    Online,
    Info,
};

// A retained notification entry
struct HistoryEntry {
    Kind kind;
    std::string text;
    std::string time; // local "HH:MM:SS" captured when pushed
};

// Display style for a kind: short tag + RGBA color
struct KindDisplay {
    const char* tag;
    float color[4];
};
KindDisplay DisplayOf(Kind kind);

// Queue a shadNet notification: shows a transient toast AND appends to history.
void Push(Kind kind, std::string text);

// History access for UI (newest entries last)
std::vector<HistoryEntry> GetHistory();
void ClearHistory();

// Unread tracking for a UI badge: number of events pushed since the last MarkRead().
// Call MarkRead() when the user is actually viewing the notifications list.
std::size_t UnreadCount();
void MarkRead();

// Add/remove the toast overlay layer. Call Register() once after ImGui init and
// Unregister() on teardown.
void Register();
void Unregister();

} // namespace ImGui::ShadNetNotify
