# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

project(gcn LANGUAGES CXX)

add_library(gcn INTERFACE)
target_sources(gcn PRIVATE
  "include/gcn/si_ci_vi_merged_offset.h"
  "include/gcn/si_ci_vi_merged_pm4_it_opcodes.h"
)

target_include_directories(gcn INTERFACE include)
