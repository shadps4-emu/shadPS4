# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

.global _runOnAnotherStack
_runOnAnotherStack:
  pushq %r12
  pushq %r13
#ifdef WIN32
  pushq %r14
  pushq %r15
  movq %gs:0x08, %r14 # save teb->stack_bottom
  movq %gs:0x10, %r15 # save teb->stack_top
  movq %rdx, %gs:0x08 # swap teb->stack_bottom
  movq %rcx, %gs:0x10 # swap teb->stack_top
#endif
  movq %rsp, %r12
  movq %rbp, %r13
  movq %rdx, %rsp
  movq %rdx, %rbp
  callq *%rsi
  movq %r13, %rbp
  movq %r12, %rsp
#ifdef WIN32
  movq %r15, %gs:0x10 # restore teb->stack_top
  movq %r14, %gs:0x08 # restore teb->stack_bottom
  popq %r15
  popq %r14
#endif
  popq %r13
  popq %r12
  ret
