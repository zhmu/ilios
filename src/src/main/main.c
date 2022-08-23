/*
 * main.c - ILIOS Kernel Main
 *
 * Here the fun begins!
 *
 */
#include <sys/types.h>
#include <sys/kmalloc.h>
#include <sys/network.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/irq.h>
#include <cli/cli.h>
#include <netipv4/ipv4.h>
#include <lib/lib.h>
#include <md/console.h>
#include <md/memory.h>
#include <md/init.h>
#include <md/interrupts.h>
#include <md/sio.h>
#include <net/dhcp.h>
#include <net/dns.h>
#include <config.h>
#include "../version.h"

char* autoexec[] = {
#if 0
"interface create lo0 lo",
"interface bind lo0 127.0.0.1",
#endif
"interface bind rl0 172.16.0.2 255.255.255.0",
"interface status",
NULL };

/* XXX */
void ep_probe();
void pci_init();

/*
 * kmain()
 *
 * This is the 'main' function for the kernel. It will be run in Ring 0.
 *
 */
void
kmain() {
	size_t total, avail;

	/* initialize the irq manager */
	irq_init();

	/* initialize the tty */
	tty_init();

	/* initialize the memory allocator and add all available memory */
	kmalloc_init();
	arch_add_all_memory();

	/* initialize machine dependant stuff */
	arch_init();

	/* initialize the network */
	network_init();

	/* initialize the IPv4 stack */
	ipv4_init();

	/* be verbose */
	kprintf (VERSION"\n\n");
	kmemstats (&total, &avail);
	kprintf ("Memory: %u KB total, %u KB available\n", (total / 1024), (avail / 1024));

	/* initialize the PCI bus */
	pci_init();

	/* 3com */
	ep_probe();

	/* serial ports! */
	arch_sio_init();

	/* DHCP stuff */
	dhcp_init();

	/* DNS stuff */
	dns_init();

	/* automatically execute stuff */
	cli_launch_script (autoexec);

	/* enable interrupts and goooo */
	arch_interrupts (ENABLE);
	cli_go();

	/* NOTREACHED */
}

/* vim:set ts=2 sw=2: */
