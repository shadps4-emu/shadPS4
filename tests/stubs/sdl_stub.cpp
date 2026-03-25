// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_messagebox.h>

extern "C" {

	bool SDL_ShowMessageBox(const SDL_MessageBoxData* /* messageboxdata */, int* buttonid) {
		if (buttonid) *buttonid = 0; // "No",skip migration
		return true;
	}

	bool SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags /* flags */, const char* /* title */,
		const char* /* message */, SDL_Window* /* window */) {
		return true;
	}

} // extern "C"
