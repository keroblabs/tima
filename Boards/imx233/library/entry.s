/*
 * linux_prep entry code.
 *
 * Entered twice:
 *
 * - Before loading Linux kernel image to SDRAM to check
 *   if CPU is being taken out of standby mode and resume
 *   sleeping kernel.
 * 
 * - After loading Linux kernel image to prepare Linux boot
 *   paramteters and jump to the kernel
 *
 * Vladislav Buzov <vbuzov@embeddedalley.com>
 *
 * Copyright 2008 SigmaTel, Inc
 * Copyright 2008 Embedded Alley Solutions, Inc
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program  is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#include "platform.h"

#undef DEBUG

    .code 32
    .section ".start", "ax"

    .global _start
    .global _irq_handler_asm
    .global irq_handler
_start:

	/* enable IRQ */
	mrs	r0, cpsr
	bic r0, r0, #0xc0
	orr r0, r0, #0x40
	msr cpsr, r0

    // set system stack
	ldr r0, SysStackAddr
	mov sp, r0

	// clear frame pointer
	mov fp, #0
	mov a2, #0

   	bl	clear_bss

    /* Enter the main() - it should never return */
    bl hardware_setup

_main_loop:
    bl main
    b _main_loop

SysStackAddr:
			.word		_ebss + SYSTEM_STACK_USER_SIZE + SYSTEM_STACK_INTR_SIZE

IntrStackAddr:
			.word		_ebss + SYSTEM_STACK_INTR_SIZE

	.global _arm926_get_mode
_arm926_get_mode:

	mrs	r0, cpsr
	mov	pc, lr

_irq_handler_asm:
	sub		lr, lr, #4
    ldr		sp, IntrStackAddr
	push	{r0, r1, r2, r3, r4, r5, ip, lr}
	mov		r4, #0x80000000
	ldr		r3, [r4]
	str		r3, [r4]
	ldr		r3, [r4]
	ldr		r3, [r3]
	blx		r3
	mov		r3, #1
	str		r3, [r4, #16]
	ldm		sp!, {r0, r1, r2, r3, r4, r5, ip, pc}^

	//sub			lr, lr, #4
    //ldr			sp, IntrStackAddr
	//stmdb		sp!, { r0-r12,lr }

	//bl			irq_handler

	//ldmia		sp!, { r0-r12,pc }^


/*
 * This function clears out the .bss section.
 */
clear_bss:
	ldr	r0, =_bss_start
	ldr	r1, =_bss_end
	mov	r2, #0
1:	str	r2, [r0], #4
	cmp	r0, r1
	blo	1b
	mov	pc, lr

	.pool
/*
 * ARM EABI toolchain needs divide by zero handler to be implemented
 * externally.
 */
	.globl	__div0
__div0:
	mov	pc, lr
    .globl  raise
raise:
	mov	pc, lr
