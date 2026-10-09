# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(PREFERRED_BASE 0x710040000000)
add_library(shadps4_cpu_id SHARED client.c)
configure_DynamoRIO_client(shadps4_cpu_id)
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND LINKER_IS_LLVM_LLD)
    target_link_options(shadps4_cpu_id PRIVATE "-fuse-ld=${CMAKE_LINKER}")
endif()
use_DynamoRIO_extension(shadps4_cpu_id drmgr)
use_DynamoRIO_extension(shadps4_cpu_id drwrap)
