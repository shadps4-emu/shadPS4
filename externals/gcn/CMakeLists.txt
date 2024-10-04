# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

project(gcn LANGUAGES CXX)

add_library(gcn dummy.cpp)

target_include_directories(gcn INTERFACE include)
