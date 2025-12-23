// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/ajm/ajm_batch.h"

namespace Libraries::Ajm {

class AjmInstanceStatistics {
public:
    void ExecuteJob(AjmJob& job);
    void Reset();

    static AjmInstanceStatistics& Getinstance();
};

} // namespace Libraries::Ajm
