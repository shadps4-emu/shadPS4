# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

.global _runOnAnotherStack
_runOnAnotherStack:
  pushq %r12
  pushq %r13
  movq %rsp, %r12
  movq %rbp, %r13
  movq %rdx, %rsp
  movq %rdx, %rbp
  callq *%rsi
  movq %r13, %rbp
  movq %r12, %rsp
  popq %r13
  popq %r12
  ret
