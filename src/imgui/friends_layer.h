// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace ImGui::Friends {

// Add/remove the Friends overlay layer. Call Register() once after ImGui is initialized
void Register();
void Unregister();

// Show/hide the Friends window. Bind Toggle() to a hotkey or a menu entry.
void Toggle();
bool IsOpen();

} // namespace ImGui::Friends
