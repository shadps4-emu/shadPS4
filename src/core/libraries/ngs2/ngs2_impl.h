// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2 {
public:
    s32 HandleReportInvalid(OrbisNgs2Handle* handle, u32 handle_type) const;

private:
};

} // namespace Libraries::Ngs2
