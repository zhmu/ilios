/*
 * exceptions.c - XeOS Exception Handler Code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is heavily based on Yoctix (src/arch/i386/idt.c), (c) 1999
 * Anders Gavare.
 *
 * This code will handle exceptions.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <lib/lib.h>

#define xSTACK_DUMP

/*
 * This will handle exception [no].
 */
void
exception_handler (uint32_t no, uint32_t ss, uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds, uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax, uint32_t errcode, uint32_t eip, uint32_t cs) {
	kprintf ("kernel exception #%x at %x:%x eax=%x ebx=%x ecx=%x edx=%x esi=%x edi=%x esp=%x ebp=%x ds=%x es=%x fs=%x gs=%x ss=%x\n", no, cs & 0xffff, eip, eax, ebx, ecx, edx, esi, edi, esp, ebp, ds & 0xffff, es & 0xffff, fs & 0xffff, gs & 0xffff, ss & 0xffff);

	/* this is serious */
	panic ("kernel exception");
}

/* vim:set ts=2 sw=2: */
