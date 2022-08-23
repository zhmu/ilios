/*
 * timer.c - ILIOS Timer Code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is heavily based on Yoctix, most residing in src/arch/i386/, (c)
 * 1999 Anders Gavare.
 *
 * This code will handle the i386 timer.
 *
 */
#include <sys/kmalloc.h>
#include <sys/irq.h>
#include <md/config.h>
#include <md/gdt.h>
#include <md/init.h>
#include <md/interrupts.h>
#include <md/pio.h>
#include <md/timer.h>
#include <lib/lib.h>
#include <netipv4/ipv4.h>
#include <config.h>

uint32_t timecnt = 0;
int tmr = 0;
int itick = 0;

/*
 * This is the actual timer interrupt.
 */
void
timer() {
#if 0
	/* ensure we do packets */
	network_handle_queue();
#endif

	/* one second passed? */
	if (++tmr != 36)
		/* no. leave */
		return;

	/* reset the timer and increment the time counter */
	tmr = 0;
	timecnt++;

	/* call IPv4 tick stuff every 10 ticks */
	if (++itick == 10) {
		itick = 0;
		ipv4_tick();
	}
}

/*
 * This will retrieve the number of seconds since bootup (well, sort of)
 */
uint32_t
arch_timer_get() {
	return timecnt;
}

/*
 * This will return the number of timer ticks.
 */
uint16_t
arch_timer_gettick() {
	uint8_t lo, hi;
	int oldints = arch_interrupts (DISABLE);

	/* Select counter 0 and latch it. */
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	lo = inb(TIMER_CNTR0);
	hi = inb(TIMER_CNTR0);
	arch_interrupts (oldints);

	return ((hi << 8) | lo);
}

/*
 * This will delay [wait] ms.
 */
void
arch_delay (int32_t wait) {
	int otick, tick, limit;

	/* fetch the old ticks */
	otick = arch_timer_gettick();
	wait -= 5;
	if (wait < 0)
		return;

	__asm __volatile("mul %2\n\tdiv %3"
	                 : "=a" (wait)
	                 : "0" (wait), "r" (TIMER_FREQ), "r" (1000000)
									 : "%edx", "cc");
	limit = TIMER_FREQ / HZ;
	while (wait > 0) {
		tick = arch_timer_gettick();
		if (tick > otick) {
			wait -= limit - (tick - otick);
		} else {
			wait -= otick - tick;
		}
		otick = tick;
	}
}

/* vim:set ts=2 sw=2: */
