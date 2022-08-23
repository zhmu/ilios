/*
 * timer.h - ILIOS i386 Timer Code
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This is a generic include file which describes the timer interface.
 *
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#define TIMER_FREQ 1193182

#define IO_TIMER1				0x40
#define TIMER_CNTR0     (IO_TIMER1 + 0) /* timer 0 counter port */
#define TIMER_CNTR1     (IO_TIMER1 + 1) /* timer 1 counter port */
#define TIMER_CNTR2     (IO_TIMER1 + 2) /* timer 2 counter port */
#define TIMER_MODE      (IO_TIMER1 + 3) /* timer mode port */

#define	TIMER_SEL0      0x00    /* select counter 0 */
#define TIMER_LATCH     0x00    /* latch counter for reading */


void timer_asm();

uint32_t arch_timer_get();
void arch_delay (int32_t wait);

#endif

/* vim:set ts=2: */
