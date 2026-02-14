# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

.global _sceFiberSetJmp
_sceFiberSetJmp:
    movq %rsp, 0x00(%rdi)
    movq %rbp, 0x08(%rdi)

    movq (%rsp), %rax
    movq %rax, 0x10(%rdi)

    movq %rbx, 0x18(%rdi)
    movq %r12, 0x20(%rdi)
    movq %r13, 0x28(%rdi)
    movq %r14, 0x30(%rdi)
    movq %r15, 0x38(%rdi)

    fnstcw  0x40(%rdi)
    stmxcsr 0x44(%rdi)

    xor %eax, %eax
    ret

.global _sceFiberLongJmp
_sceFiberLongJmp:
    mov %rdi, %r11
    stmxcsr -4(%rsp)
    mov 0x44(%r11), %eax
    and $0xffffffc0, %eax
    mov -4(%rsp), %ecx
    and $0x3f, %ecx
    xor %ecx, %eax
    mov %eax, -4(%rsp)
    ldmxcsr -4(%rsp)

    movq 0x00(%r11), %rsp
    movq 0x08(%r11), %rbp
    movq 0x10(%r11), %rcx
    movq 0x18(%r11), %rbx
    movq 0x20(%r11), %r12
    movq 0x28(%r11), %r13
    movq 0x30(%r11), %r14
    movq 0x38(%r11), %r15

    fldcw 0x40(%r11)

    movq %rcx, 0x00(%rsp)
    movl $0x1, %eax
    ret

.global _sceFiberSwitchEntry
_sceFiberSwitchEntry:
    mov %rdi, %r11
    mov %esi, %r8d

    # Set stack address to provided stack
    movq 0x18(%r11), %rsp
    xorl %ebp, %ebp

    movq 0x28(%r11), %rdi # data->asan_fake_stack
    test %rdi, %rdi
    jz .skip_asan_finish
    dec %rdi
    xor %rsi, %rsi
    xor %rdx, %rdx
    push %r11
    push %r11
    call __sanitizer_finish_switch_fiber
    pop %r11
    pop %r11
.skip_asan_finish:

    movq 0x08(%r11), %rdi # data->arg_on_initialize
    movq 0x10(%r11), %rsi # data->arg_on_run_to
    movq 0x20(%r11), %r10 # data->state
    movq 0x00(%r11), %r11 # data->entry

    # Set previous fiber state to Idle
    test %r10, %r10
    jz .clear_regs
    movl $2, (%r10)

.clear_regs:
    test %r8d, %r8d
    jz .skip_fpu_regs

    ldmxcsr kFiberMxcsrDefault(%rip)
    fldcw kFiberFpucwDefault(%rip)

.skip_fpu_regs:
    xorl %eax, %eax
    xorl %ebx, %ebx
    xorl %ecx, %ecx
    xorl %edx, %edx
    xorq %r8, %r8
    xorq %r9, %r9
    xorq %r10, %r10
    xorq %r12, %r12
    xorq %r13, %r13
    xorq %r14, %r14
    xorq %r15, %r15
    pxor %mm0, %mm0
    pxor %mm1, %mm1
    pxor %mm2, %mm2
    pxor %mm3, %mm3
    pxor %mm4, %mm4
    pxor %mm5, %mm5
    pxor %mm6, %mm6
    pxor %mm7, %mm7
    emms
    vzeroall

    # Call the fiber's entry function: entry(arg_on_initialize, arg_on_run_to)
    call *%r11

    # Fiber returned, not good
    movl $1, %edi
    call _sceFiberForceQuit
    ret

    .align 4
kFiberMxcsrDefault:
    .long 0x00009fc0
    .align 2
kFiberFpucwDefault:
    .short 0x037f
