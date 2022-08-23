/*
 * lo.c - ILIOS Loopback Network Device
 * (c) 2003 Rink Springer, BSD licensed
 *
 * This module handles all loopback device stuff.
 *
 */
#include <sys/device.h>
#include <sys/network.h>
#include <sys/kmalloc.h>
#include <sys/irq.h>
#include <sys/types.h>
#include <lib/lib.h>
#include <md/pio.h>
#include "ne.h"
#include "ne_reg.h"

/*
 * This will handle 'transmission' on the loopback device.
 */
void
lo_xmit_frame(struct DEVICE* dev) {
	struct NETPACKET* pkt;

	/* keep going while we have data */
	for (;;) {
		/* fetch the packet */
		pkt = network_get_next_txbuf (dev);
		if (pkt == NULL)
			/* no packet. leave */
			return;

		/* update the statistics */
		dev->tx_frames++; dev->tx_bytes += pkt->len;
		dev->rx_frames++; dev->rx_bytes += pkt->len;

		/* queue the packet for handling */
		network_queue_packet (pkt);
	}
}

/*
 * This will initialize (yeah, right) the loopback device.
 */
int
lo_init (char* name, struct DEVICE_RESOURCES* res) {
	struct DEVICE dev;
	struct DEVICE* devptr;

	/* clear the device struct and set it up */
	kmemset (&dev, 0, sizeof (struct DEVICE));

	/* just register the device immediately */
	dev.name = name;
	dev.xmit = lo_xmit_frame;
	dev.addr_len = ETHER_ADDR_LEN;
	devptr = device_register (&dev);
	if (devptr == NULL) {
		/* this failed. complain */
		kprintf ("%s: unable to register device!\n", name);
		return 0;
	}

	/* fill out our device-specific data */
	kmemset (devptr->ether.hw_addr, 0, 6);

	/* all done */
	return 1;
}

/* vim:set ts=2 sw=2: */
