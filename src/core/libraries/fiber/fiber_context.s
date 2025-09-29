# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

.global _sceFiberSetJmp
_sceFiberSetJmp:
    movq %rax, 0x0(%rdi)

    movq (%rsp), %rdx
    movq %rdx, 0x10(%rdi)

    movq %rcx, 0x08(%rdi)
    movq %rbx, 0x18(%rdi)
    movq %rsp, 0x20(%rdi)
    movq %rbp, 0x28(%rdi)

    movq %r8,  0x30(%rdi)
    movq %r9,  0x38(%rdi)
    movq %r10, 0x40(%rdi)
    movq %r11, 0x48(%rdi)
    movq %r12, 0x50(%rdi)
    movq %r13, 0x58(%rdi)
    movq %r14, 0x60(%rdi)
    movq %r15, 0x68(%rdi)

    fnstcw  0x70(%rdi)
    stmxcsr 0x72(%rdi)

    xor %eax, %eax
    ret

.global _sceFiberLongJmp
_sceFiberLongJmp:
    # MXCSR = (MXCSR & 0x3f) ^ (ctx->mxcsr & ~0x3f)
    stmxcsr -0x4(%rsp)
    movl 0x72(%rdi), %eax
    andl $0xffffffc0, %eax
    movl -0x4(%rsp), %ecx
    andl $0x3f, %ecx
    xorl %eax, %ecx
    movl %ecx, -0x4(%rsp)
    ldmxcsr -0x4(%rsp)

    movq 0x00(%rdi), %rax
    movq 0x08(%rdi), %rcx
    movq 0x10(%rdi), %rdx
    movq 0x18(%rdi), %rbx
    movq 0x20(%rdi), %rsp
    movq 0x28(%rdi), %rbp

    movq 0x30(%rdi), %r8
    movq 0x38(%rdi), %r9
    movq 0x40(%rdi), %r10
    movq 0x48(%rdi), %r11
    movq 0x50(%rdi), %r12
    movq 0x58(%rdi), %r13
    movq 0x60(%rdi), %r14
    movq 0x68(%rdi), %r15

    fldcw 0x70(%rdi)

    # Make the jump and return 1
    movq %rdx, 0x00(%rsp)
    movl $0x1, %eax
    ret

.global _sceFiberSwitchEntry
_sceFiberSwitchEntry:
    mov %rdi, %r11

    # Set stack address to provided stack
    movq 0x18(%r11), %rsp
    xorl %ebp, %ebp

    movq 0x20(%r11), %r10 # data->state

    # Set previous fiber state to Idle
    test %r10, %r10
    jz .clear_regs
    movl $2, (%r10)

.clear_regs:
    test %esi, %esi
    jz .skip_fpu_regs

    ldmxcsr 0x2c(%r11)
    fldcw 0x28(%r11)

.skip_fpu_regs:
    movq 0x08(%r11), %rdi # data->arg_on_initialize
    movq 0x10(%r11), %rsi # data->arg_on_run_to
    movq 0x00(%r11), %r11 # data->entry

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
