/*
 * interrupts.c - XeOS i386 interrupts code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is heavily based on Yoctix (src/arch/i386/idt.c), (c) 1999
 * Anders Gavare.
 *
 * This code will handle interrupts.
 *
 */
#include <md/interrupts.h>

/*
 * This will turn the interrupts on or off, and return the previous state.
 */
int
arch_interrupts (int state) {
	int flags;

	__asm__ ("pushf\npopl %%eax" : "=a" (flags));

	if (state == DISABLE)
		asm ("cli");
	else
		asm ("sti");

	return (flags & 0x200) ? ENABLE : DISABLE;
}

/* vim:set ts=2 sw=2: */
