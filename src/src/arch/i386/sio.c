/*
 * sio.c - XeOS Serial IO Module
 * (c) 2002 Rink Springer, BSD licensed
 *
 * This module handles all serial IO stuff.
 *
 */
#include <sys/types.h>
#include <sys/irq.h>
#include <sys/device.h>
#include <lib/lib.h>
#include <md/pio.h>
#include <md/sio.h>

#define SIO_BASE 0x3f8
#define SIO_IRQ 4

char* sio_type[] = { "8250", "16450", "16550", "16550A" };

/*
 * This will probe for a serial interface at [port]. It will return a 
 * SIO_TYPE_xxx on success or -1 on failure.
 */
int
arch_sio_probe (int port) {
	int x, old_data;

	/* check for UART presence */
	old_data = inb (port + SIO_MCR);
	outb (port + SIO_MCR, 0x10);
	if (inb (port + SIO_MSR) & 0xF0) return -1;
	outb (port + SIO_MCR, 0x1F);
	if ((inb (port + SIO_MSR) & 0xF0) != 0xF0) return -1;
	outb (port + SIO_MCR, old_data);

	/* check for a scatch register */
	old_data = inb (port + SIO_SCR);
	outb (port + SIO_SCR, 0x55);
	if (inb (port + SIO_SCR) != 0x55) return SIO_TYPE_8250;
	outb (port + SIO_SCR, 0xAA);
	if (inb (port + SIO_SCR) != 0xAA) return SIO_TYPE_8250;
	outb (port + SIO_SCR, old_data);

	/* check for the FIFO */
	outb (port + SIO_IIR, 1);
	x = inb (port + SIO_IIR);
	outb (port + SIO_IIR, 0);

	/* FIFO state is the key now */
	if ((x & 0x80) == 0) return SIO_TYPE_16450;
	if ((x & 0x40) == 0) return SIO_TYPE_16550;

	/* yay, the user got the real deal :) */;
	return SIO_TYPE_16550A;
}

/*
 * This will initialize the serial port at [port].
 */
void
arch_sio_initport (int port) {
	/* initialize the plain port */
	outb (port + SIO_LCR, SIO_LCR_DLAB);     /* set ldab */
	outw (port, 12);                         /* baud rate 9600bps */
	outb (port + SIO_LCR, 0x3);              /* 8n1 */
	outb (port + SIO_MCR, 3);                /* clear loopback, DTR/RTS on */

	/* initialize the interrupt */
	outb (port + SIO_MCR, inb (port + SIO_MCR) | SIO_MCR_OUT2);
	outb (port + SIO_IER, 0xc7 /* 0x80 | 0x40 | 0x4 | 0x2 | 0x1 */);
}

/*
 * This will send char [ch] to the other side.
 */
void
arch_sio_send (uint8_t ch) {
	while ((inb (SIO_BASE + SIO_LSR) & SIO_LSR_THRE) == 0);
	outb (SIO_BASE, ch);
}

/*
 * This will handle serial IO IRQ's.
 */
void
arch_sio_irq() {
	unsigned char stat, ch;
	int port = SIO_BASE;

	/* keep handling until the port says it's clear */
	for (;;) {
		/* fetch interrupt status */
		stat = inb (port + SIO_IIR);
		if (stat & 1)
			return;

		/* isolate anything but bit 1 and 2 */
		stat &= 6;

		/* mask out our bits */
		switch (stat) {
			case 0: /* modem status. we don't care, so just clear it */
			        ch = inb (port + SIO_MSR);
			        break;
			case 2: /* transfer interrupt */
			        outb (port + SIO_IER, 1);
			        break;
			case 4: /* incoming! */
			        ch = inb (port);
			        kprintf ("sio0: got char '%c'\n", ch);
			        break;
		  case 6: /* status info */
			        ch = inb (port + SIO_LSR);
			        break;
		}
	}
}

/*
 * This will initialize the serial driver.
 */
void
arch_sio_init() {
	int type;
	int port = SIO_BASE;

	/* probe for the UART */
	type = arch_sio_probe (port);
	if (type < 0) {
		/* this failed. complain */
		kprintf ("sio0: not found\n");
		return;
	}

	/* be verbose */
	kprintf ("sio0: type %s\n", sio_type[type]);

	/* register our irq */
	if (!irq_register (SIO_IRQ, (void*)&arch_sio_irq, NULL)) {
		kprintf ("sio0: cannot register IRQ %u\n", SIO_IRQ);
	}

	/* initialize the device */
	arch_sio_initport (port);
}

/* vim:set ts=2 sw=2: */
