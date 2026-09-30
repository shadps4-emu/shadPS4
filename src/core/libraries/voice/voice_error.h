// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Values below were recovered from the retail libSceVoice.sprx (control-flow
// and immediate values confirmed via static disassembly). Codes marked
// "observed only" are emitted by internal paths of the real module that are not
// reachable through the entry points implemented here; they are kept for
// documentation/completeness. Names for the "observed only" codes were
// independently corroborated (same numeric values) by @brad0demx in #5191.
constexpr int ORBIS_VOICE_ERROR_NOT_INIT = 0x804E0801;
constexpr int ORBIS_VOICE_ERROR_ALREADY_INIT = 0x804E0802;
constexpr int ORBIS_VOICE_ERROR_INVALID_PORT_ID = 0x804E0804;
constexpr int ORBIS_VOICE_ERROR_ARGUMENT_INVALID = 0x804E0805;
constexpr int ORBIS_VOICE_ERROR_NOT_ACTIVE =
    0x804E0807; // port_id in range but not created
// Observed only (internal codec/format negotiation paths not exercised by the
// public API below).
constexpr int ORBIS_VOICE_ERROR_GENERAL = 0x804E0803;
constexpr int ORBIS_VOICE_ERROR_CONTAINER_INVALID = 0x804E0806;
constexpr int ORBIS_VOICE_ERROR_RESOURCE_INSUFFICIENT = 0x804E0808;
