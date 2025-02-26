// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"

namespace NetUtil {

class NetUtilInternal {
public:
    explicit NetUtilInternal() = default;
    ~NetUtilInternal() = default;

private:
    std::array<u8, 6> ether_address{};
    std::mutex m_mutex;

public:
    const std::array<u8, 6>& GetEthernetAddr() const;
    bool RetrieveEthernetAddr();
};
} // namespace NetUtil