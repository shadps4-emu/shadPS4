# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

file(READ "${HEADER_FILE}" HEADER_CONTENT)
string(REPLACE "#pragma once" "#pragma once\n#include <cstdint>" HEADER_CONTENT "${HEADER_CONTENT}")
file(WRITE "${HEADER_FILE}" "${HEADER_CONTENT}")
