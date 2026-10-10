// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Values confirmed via static disassembly of the retail libSceVoice.sprx.
constexpr int ORBIS_VOICE_ERROR_NOT_INIT = 0x804E0801;
constexpr int ORBIS_VOICE_ERROR_ALREADY_INIT = 0x804E0802;
constexpr int ORBIS_VOICE_ERROR_INVALID_PORT_ID = 0x804E0804;
constexpr int ORBIS_VOICE_ERROR_ARGUMENT_INVALID = 0x804E0805;
constexpr int ORBIS_VOICE_ERROR_NOT_ACTIVE = 0x804E0807; // port_id in range but not created

// Not returned by the functions implemented here; kept for completeness (names credited to
// @brad0demx, #5191).
constexpr int ORBIS_VOICE_ERROR_GENERAL = 0x804E0803;
constexpr int ORBIS_VOICE_ERROR_CONTAINER_INVALID = 0x804E0806;
constexpr int ORBIS_VOICE_ERROR_RESOURCE_INSUFFICIENT = 0x804E0808;
