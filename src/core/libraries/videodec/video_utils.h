// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AVFrame;

namespace Libraries::Videodec {

void CopyNV12Data(u8* dst, const AVFrame& src);

}
