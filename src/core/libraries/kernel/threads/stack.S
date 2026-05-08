# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

.global _runOnAnotherStack
_runOnAnotherStack:
  pushq %r12
  pushq %r13
#ifdef _WIN32
  pushq %r14
  pushq %r15

  # save stack check registers
  movq %gs:0x08, %r14 # teb->stack_bottom
  movq %gs:0x10, %r15 # teb->stack_top

  # disable stack checks
  xorq %rcx, %rcx
  movq %rcx, %gs:0x08 # teb->stack_bottom = 0
  movq %rcx, %gs:0x10 # teb->stack_top = 0
#endif
  movq %rsp, %r12
  movq %rbp, %r13
  movq %rdx, %rsp
  movq %rdx, %rbp
  callq *%rsi
  movq %r13, %rbp
  movq %r12, %rsp
#ifdef _WIN32
  # restore stack check registers
  movq %r14, %gs:0x08 # teb->stack_bottom
  movq %r15, %gs:0x10 # teb->stack_top
  popq %r15
  popq %r14
#endif
  popq %r13
  popq %r12
  ret
