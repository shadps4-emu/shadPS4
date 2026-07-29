// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/types.h"

namespace ImGui::InvitationPrompt {

// Emulator-side stand-in for the PS4 system software UI ("ShellUI") invitation notification.
//
// ORBIS_SYSTEM_SERVICE_EVENT_SESSION_INVITATION fires only after the user
// "explicitly performs an action that joins from an invitation... using invitation dialog or the
// system software UI". Titles that don't open the invitation dialog (RECV) themselves rely
// entirely on the system software UI, which doesn't exist in the emulator. This layer fills that
// role: NpHandler queues a prompt when a sessionInvitation push arrives, the user Accepts or
// Declines here, and Accept routes through NpHandler::AcceptSessionInvitation - the same path the
// RECV dialog uses - which posts the join event and consumes the invite server-side.

// Queue an accept/decline prompt for a received session invitation. Thread-safe; deduplicates on
// invitation_id (a re-pushed invite replaces the older prompt).
void Push(s32 user_id, std::string invitation_id, std::string session_id, std::string from_npid);

// Drop a queued prompt (e.g. the invite was consumed through the RECV dialog instead).
// Thread-safe,no-op if not present.
void Dismiss(const std::string& invitation_id);

// Add/remove the overlay layer. Call Register() once after ImGui init and Unregister() on
// teardown.
void Register();
void Unregister();

} // namespace ImGui::InvitationPrompt
