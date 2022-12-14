/*
 * tty.h - XeOS Console Driver
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This is a generic include file which describes a TTY driver.
 *
 */
#include <sys/types.h>
#include <sys/device.h>

#ifndef __TTY_H__
#define __TTY_H__

#ifdef __KERNEL

#define tty_init arch_console_init
#define tty_putchar arch_console_putchar
#define tty_puts arch_console_puts

void tty_init();
void tty_puts(const char*);
void tty_putchar(uint8_t);
uint8_t tty_getchar();
int	tty_setdevice (struct DEVICE*);
struct DEVICE* tty_getdevice();

extern struct DEVICE* tty_device;
#endif /* __KERNEL */

#endif

/* vim:set ts=2: */
